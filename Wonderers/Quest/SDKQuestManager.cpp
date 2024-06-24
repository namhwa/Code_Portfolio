// Fill out your copyright notice in the Description page of Project Settings.

#include "Quest/SDKQuestManager.h"
#include "Engine/World.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

#include "Character/SDKMyInfo.h"
#include "Character/SDKHUD.h"
#include "Character/SDKCharacter.h"
#include "Character/SDKPlayerCharacter.h"
#include "Character/SDKPlayerController.h"

#include "Engine/SDKAssetManager.h"
#include "Engine/SDKGameInstance.h"
#include "GameMode/SDKGameState.h"
#include "GameMode/SDKRpgGameMode.h"
#include "GameMode/SDKRpgState.h"

#include "Manager/SDKTableManager.h"
#include "Manager/SDKLobbyServerManager.h"

#include "Object/SDKQuest.h"
#include "Share/SDKHelper.h"
#include "Share/SDKDataTable.h"

#include "UI/SDKUserWidget.h"
#include "UI/SDKUIOpenWorldWidget.h"
#include "UI/SDKTopBarWidget.h"


DEFINE_LOG_CATEGORY(LogQuest)


USDKQuestManager::USDKQuestManager()
{
	ClearQuestManager();
}

USDKQuestManager::~USDKQuestManager()
{
}

class UWorld* USDKQuestManager::GetWorld() const
{
	if (IsValid(GEngine) && IsValid(GEngine->GameViewport))
	{
		return GEngine->GameViewport->GetWorld();
	}

	return nullptr;
}

void USDKQuestManager::CheckActiveConditionQuestMissionList(int32 InHeroID, int32 InChapter, int32 Step, TMap<int32, FQuestCondition>& OutmapData)
{
	TMap<int32, TArray<int32>> mapDataByHero;
	TMap<int32, FQuestCondition> mapActiveList;

	TArray<int32> QuestMissionIDs;
	mapQuestMissionData.GenerateKeyArray(QuestMissionIDs);
	QuestMissionIDs += StartableMissionIDList;
	QuestMissionIDs.Sort([](int32 A, int32 B) { return A < B; });

	if (QuestMissionIDs.Num() > 0)
	{
		for (auto& itID : QuestMissionIDs)
		{
			if (mapQuestMissionData.Contains(itID) && mapQuestMissionData[itID].GetIsReceivedReward() == true)
			{
				continue;
			}

			FS_QuestMission* QMissionTable = USDKTableManager::Get()->FindTableRow<FS_QuestMission>(ETableType::tb_QuestMission, FString::FromInt(itID));
			if (QMissionTable != nullptr)
			{
				bool bAllCharacter = QMissionTable->RequireCharacter.Num() <= 0;

				bool bAdd = bAllCharacter || QMissionTable->RequireCharacter.Contains(InHeroID);
				bAdd &= QMissionTable->Chapter <= 0 || QMissionTable->Chapter == InChapter;
				bAdd &= Step >= QMissionTable->StepMax - 1;
				bAdd &= QMissionTable->LevelID.IsNone() == false;

				if (bAdd == true )
				{
					if (mapActiveList.Contains(itID))
					{
						mapActiveList.Add(itID, FQuestCondition(QMissionTable->Chapter, QMissionTable->LevelID, QMissionTable->RequireCharacter, QMissionTable->StepMin, QMissionTable->StepMax, QMissionTable->ApplyAllColumn, QMissionTable->InvisibleQuestMark));
					}

					if (bAllCharacter)
					{
						if (!mapDataByHero.Contains(0))
						{
							mapDataByHero.Add(0, { itID });
						}
						else
						{
							mapDataByHero[0].AddUnique(itID);
						}
					}
					else
					{
						if (!mapDataByHero.Contains(InHeroID))
						{
							mapDataByHero.Add(InHeroID, { itID });
						}
						else
						{
							mapDataByHero[InHeroID].AddUnique(itID);
						}
					}
				}
			}
		}
	}

	if(mapDataByHero.Num() > 0)
	{
		// 영웅ID에 따른 소팅 : 플레이중인 영웅 / 전체 영웅
		mapDataByHero.KeySort([&](int32 A, int32 B)
		{
			if (A == InHeroID)
			{
				return true;
			}
			else if (B == InHeroID)
			{
				return false;
			}

			return A < B;
		});

		int32 MaxCount = 0;
		FS_GlobalDefine* GDTable = USDKTableManager::Get()->FindTableGlobalDefine(EGlobalDefine::MaxProgressQuestCount);
		if (GDTable != nullptr && GDTable->Value.Num() > 0)
		{
			MaxCount = FSDKHelpers::StringToInt32(GDTable->Value[0]);
		}

		auto itHero = mapDataByHero.CreateIterator();
		for (int32 i = 0; i < MaxCount; ++i)
		{
			if (itHero->Key == InHeroID && itHero->Value.Num() > 0)
			{
				int32 TableID = itHero->Value[0];
				OutmapData.Add(TableID, mapActiveList[TableID]);

				++itHero;
				continue;
			}

			if (itHero->Key == 0 && itHero->Value.Num() > 0)
			{
				int32 Index = 0;
				if (OutmapData.Num() > 0 && itHero->Value.Num() > i)
				{
					Index = i;
				}

				if(itHero->Value.IsValidIndex(Index))
				{
					int32 TableID = itHero->Value[Index];
					if (!OutmapData.Contains(TableID))
					{
						OutmapData.Add(TableID, mapActiveList[TableID]);
					}
				}
			}
		}
	}
}

CQuestMissionData USDKQuestManager::GetQuestMissionData(int32 QuestMissionID)
{
	if (mapQuestMissionData.Contains(QuestMissionID))
	{
		return mapQuestMissionData[QuestMissionID];
	}

	return CQuestMissionData();
}

int32 USDKQuestManager::GetActivedQuestActorCount() const
{
	return mapQuestActor.Num() + mapQuestMissionActor.Num();
}

void USDKQuestManager::SetQuestDataList(const TArray<CQuestData>& Data)
{
	if (Data.Num() <= 0)
	{
		return;
	}

	for (auto& itData : Data)
	{
		SetQuestData(itData);
		SetUnlockContentsByEndQuest(itData.GetQuestID());
	}
}

void USDKQuestManager::SetQuestDataList(const TArray<int32>& QuestIDs, const TArray<CQuestMissionData>& Data)
{
	if (QuestIDs.Num() <= 0)
	{
		return;
	}

	for (auto& itID : QuestIDs)
	{
		UE_LOG(LogQuest, Log, TEXT("Setted QuestID: %d"), itID);
	}
	

	for (auto& itID : QuestIDs)
	{
		if (!mapQuestData.Contains(itID)
		{
			SetQuestData(CQuestData(itID));
		}
	}

	if (Data.Num() > 0)
	{
		for (auto const& itData : Data)
		{
			UE_LOG(LogQuest, Log, TEXT("Setted QuestMissionID: %d"), itData.GetQuestMissionID());
		}
		
		for (auto& itData : Data)
		{
			SetQuestMissionData(itData);
		}

		SortQuestMissioIDListByQuestType(ProgressingMissionIDList);
		SortQuestMissioIDListByQuestType(CompletableMissionIDList);

		SetQuestCurrentIndex(Data);
		SetStartableQuestMissionList();

		ActiveQuestActorList();

		UpdateQuestBoardUI();
	}
	else
	{
		for (auto& itID : QuestIDs)
		{
			if (mapQuestData.Contains(itID) && mapQuestData[itID].GetCurrentMissionIndex() < 0)
			{
				SetStartableQuestMissionList(itID);
			}	
		}
	}
}

void USDKQuestManager::SetQuestCurrentIndex(const TArray<CQuestMissionData>& Data)
{
	for (auto& itData : Data)
	{
		int32 QuestID = itData.GetQuestID();
		auto pQuestData = mapQuestData.Find(QuestID);
		if (pQuestData != nullptr && pQuestData->GetQuestID() <= 0)
		{
			auto QuestTable = USDKTableManager::Get()->FindTableRow<FS_Quest>(ETableType::tb_Quest, FString::FromInt(QuestID));
			if (QuestTable != nullptr)
			{
				SetQuestData(CQuestData(QuestID, QuestTable->MissionIDList.Find(itData.GetQuestMissionID()), false, false));
			}
		}
	}
}

void USDKQuestManager::EndQuestData(int32 EndID)
{
	UE_LOG(LogQuest, Log, TEXT("EndQuestData: %d"), EndID);
	
	CQuestData* pQuestData = mapQuestData.Find(EndID);
	if (pQuestData != nullptr)
	{
		pQuestData->SetEndQuest();

		SetQuestData(*pQuestData);
		SetUnlockContentsByEndQuest(EndID);

		if (EndQuestIDList.Num() > 0)
		{
			SetActiveQuestActors(EQuestState::End, EndQuestIDList);
		}
	}
}

void USDKQuestManager::SetQuestMissionDataList(const TArray<CQuestMissionData>& Data)
{
	if (Data.Num() <= 0)
	{
		return;
	}

	for (auto& itData : Data)
	{
		SetQuestMissionData(itData);
	}

	SortQuestMissioIDListByQuestType(ProgressingMissionIDList);
	SortQuestMissioIDListByQuestType(CompletableMissionIDList);

	SetStartableQuestMissionList();

	ActiveQuestActorList();

	// 퀘스트 보드 갱신
	UpdateQuestBoardUI();
}

void USDKQuestManager::SetStartableQuestMissionList(int32 QuestID, int32 Index /*= 0*/)
{
	auto QuestTable = USDKTableManager::Get()->FindTableRow<FS_Quest>(ETableType::tb_Quest, FString::FromInt(QuestID));
	if (QuestTable != nullptr && QuestTable->MissionCount > 0 && QuestTable->MissionCount > Index)
	{
		StartableMissionIDList.AddUnique(QuestTable->MissionIDList[Index]);
	}
}

void USDKQuestManager::SetStartableQuestMissionList()
{
	if (ProgressingQuestIDList.Num() <= 0)
	{
		return;
	}

	for (auto& itID : ProgressingQuestIDList)
	{
		auto pQuestData = mapQuestData.Find(itID);
		if (pQuestData != nullptr)
		{
			int32 CurIdx = pQuestData->GetCurrentMissionIndex();
			if (CurIdx < 0)
			{
				SetStartableQuestMissionList(itID);
			}
			else
			{
				auto QuestTable = USDKTableManager::Get()->FindTableRow<FS_Quest>(ETableType::tb_Quest, FString::FromInt(itID));
				if (QuestTable == nullptr || QuestTable->MissionCount <= CurIdx)
				{
					continue;
				}

				int32 MissionID = QuestTable->MissionIDList[CurIdx];
				auto pQMissionData = mapQuestMissionData.Find(MissionID);
				if (pQMissionData == nullptr)
				{
					continue;
				}

				if (!pQMissionData->GetIsReceivedReward())
				{
					continue;
				}

				int32 NextIdx = CurIdx + 1;
				if (NextIdx >= QuestTable->MissionCount)
				{
					continue;
				}

				int32 NextID = QuestTable->MissionIDList[NextIdx];
				if (!StartableMissionIDList.Contains(NextID) && !ProgressingMissionIDList.Contains(NextID))
				{
					SetStartableQuestMissionList(itID, NextIdx);
				}
			}
		}
	}
}

void USDKQuestManager::SetStartableQuestMissionList(const TArray<CQuestMissionData>& InList)
{
	if (InList.Num() <= 0)
	{
		return;
	}
	
	for(auto& itData : InList)
	{
		if (itData.GetQuestMissionID() <= 0)
		{
			continue;
		}

		if (!mapQuestData.Contains(itData.GetQuestID()))
		{
			SetQuestData(CQuestData(itData.GetQuestID()));
		}

		SetQuestMissionData(itData);

		StartableMissionIDList.AddUnique(itData.GetQuestMissionID());
	}


	if (StartableMissionIDList.Num() > 0)
	{
		SetActiveQuestMissionActors(EQuestState::Available, StartableMissionIDList);
	}

	// 퀘스트 보드 갱신
	UpdateQuestBoardUI();
}

void USDKQuestManager::SetCompletableQuestMissionList(const TArray<int32>& IDList)
{
	CompletableMissionIDList = IDList;

	// 퀘스트 액터 활성화 설정
	bool bHaveOpenAttribute = false;
	if(IDList.Num() > 0)
	{
		USDKGameInstance* SDKGameInstance = GetWorld()->GetGameInstance<USDKGameInstance>();
		if(IsValid(SDKGameInstance))
		{
			for (auto itID : IDList)
			{
				auto pQuestMissionData = mapQuestMissionData.Find(itID);
				if (pQuestMissionData != nullptr)
				{
					pQuestMissionData->SetMissionCompleted();

					SetQuestMissionData(*pQuestMissionData);
				}

				SortQuestMissioIDListByQuestType(ProgressingMissionIDList);
				SortQuestMissioIDListByQuestType(CompletableMissionIDList);

				auto QuestMissionTable = USDKTableManager::Get()->FindTableRow<FS_QuestMission>(ETableType::tb_QuestMission, FString::FromInt(itID));
				if (QuestMissionTable != nullptr)
				{
					if (QuestMissionTable->CompleteQuestActor.IsNull())
					{
						UE_LOG(LogQuest, Log, TEXT("USDKQuestManager::SetCompletableQuestMissionList() - CQ_QuestMissionEnd"));
						SDKGameInstance->LobbyServerManager->CQ_QuestMissionEnd(itID);
					}
				}
			}
		}

		SetActiveQuestMissionActors(EQuestState::Complete, IDList);

		if (FSDKHelpers::GetGameMode(GetWorld()) == EGameMode::Rpg)
		{
			ASDKRpgState* SDKRpgState = GetWorld()->GetGameState<ASDKRpgState>();
			if(IsValid(SDKRpgState) && SDKRpgState->GetCurrentMapType() == FName("Boss"))
			{
				if (mapQuestMissionActor.Num() > 0)
				{
					for (auto& itID : IDList)
					{
						if (!mapQuestMissionActor.Contains(itID))
						{
							continue;
						}

						AddDelegateHandle(itID, EndQuestActorEventCallback.AddWeakLambda(this, [&]
							{
								ASDKRpgState* BindRpgState = GetWorld()->GetGameState<ASDKRpgState>();
								if (IsValid(BindRpgState))
								{
									BindRpgState->CheckRpgGame(true);
								}
							}));
					}
				}
			}
		}
	}
}

void USDKQuestManager::CheckProgressingQuestMissionData(EQuestMissionType MissionType, int32 TargetID, int32 Count/* = 1*/)
{
	if (ProgressingQuestIDList.Num() <= 0 && ProgressingMissionIDList.Num() <= 0)
	{
		return;
	}

	bool bFindMission = false;
	for (auto& itID : ProgressingMissionIDList)
	{
		auto pQuestMissionData = mapQuestMissionData.Find(itID);
		if (pQuestMissionData == nullptr)
		{
			continue;
		}

		// 미션 타입이 안맞거나 미션이 없는 경우
		if (pQuestMissionData->GetQuestMissionType() != MissionType || pQuestMissionData->GetMissionList().Num() <= 0)
		{
			continue;
		}
		
		auto QuestMissionTable = USDKTableManager::Get()->FindTableRow<FS_QuestMission>(ETableType::tb_QuestMission, FString::FromInt(itID));
		if (QuestMissionTable != nullptr)
		{
			// 로그라이크 : 몬스터 잡는 퀘스트 미션인 경우, 레벨 아이디 비교
			if (CurrentMode == EGameMode::Rpg && MissionType == EQuestMissionType::KillMonster)
			{
				TArray<FString> LevelIDs;
				auto SDKRpgMode = GetWorld()->GetAuthGameMode<ASDKRpgGameMode>();
				if (IsValid(SDKRpgMode))
				{
					LevelIDs = SDKRpgMode->GetLoadedLevelIDs();
				}

				if (!QuestMissionTable->LevelID.IsNone() && !LevelIDs.Contains(QuestMissionTable->LevelID.ToString()))
				{
					continue;
				}
			}

			ASDKPlayerCharacter* PlayerCharacter = Cast<ASDKPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
			if (IsValid(PlayerCharacter))
			{
				if (QuestMissionTable->RequireCharacter.Num() > 0 && QuestMissionTable->RequireCharacter.Contains(PlayerCharacter->GetTableID()) == false)
				{
					continue;
				}
			}
		}

		for (auto itMission : pQuestMissionData->GetMissionList())
		{
			if (itMission.GetMissionID() != TargetID)
			{
				continue;
			}

			bFindMission = true;
			break;
		}

		if (bFindMission)
		{
			break;
		}
	}

	if (bFindMission == true)
	{
		// 로그라이크 중이고, 
		if (CurrentMode == EGameMode::Rpg)
		{
			auto SDKRpgState = GetWorld()->GetGameState<ASDKRpgState>();
			if (IsValid(SDKRpgState) && SDKRpgState->GetPlayingRoomPhase())
			{
				if (!mapSavedMissions.Contains(MissionType))
				{
					mapSavedMissions.Add(MissionType);
					mapSavedMissions[MissionType].Add(TargetID, Count);
				}
				else
				{
					if (!mapSavedMissions[MissionType].Contains(TargetID))
					{
						mapSavedMissions[MissionType].Add(TargetID, Count);
					}
					else
					{
						mapSavedMissions[MissionType][TargetID] += Count;
					}
				}

				return;
			}
		}

		auto SDKGameInstance = GetWorld()->GetGameInstance<USDKGameInstance>();
		if (IsValid(SDKGameInstance))
		{
			SDKGameInstance->LobbyServerManager->CQ_QuestMissionUpdateCount(MissionType, TargetID, Count);
		}
	}
}

void USDKQuestManager::SendFinishPhaseQuestMissionCount()
{
	if (mapSavedMissions.Num() <= 0)
	{
		return;
	}

	for (auto itType : mapSavedMissions)
	{
		if (itType.Value.Num() <= 0)
		{
			continue;
		}

		for(auto itCount : itType.Value)
		{
			auto SDKGameInstance = GetWorld()->GetGameInstance<USDKGameInstance>();
			if (IsValid(SDKGameInstance))
			{
				SDKGameInstance->LobbyServerManager->CQ_QuestMissionUpdateCount(itType.Key, itCount.Key, itCount.Value);
			}
		}
	}
	
	mapSavedMissions.Empty();
}

void USDKQuestManager::UpdateStartedQuestMissionData(const CQuestMissionData& Data, bool bUpdateActor /*= true*/)
{
	UE_LOG(LogQuest, Log, TEXT("UpdateStartedQuestMissionData : %d"), Data.GetQuestMissionID());
	SetQuestMissionData(Data);

	SortQuestMissioIDListByQuestType(ProgressingMissionIDList);
	SortQuestMissioIDListByQuestType(CompletableMissionIDList);

	if (!Data.GetIsReceivedReward())
	{
		auto QuestTable = USDKTableManager::Get()->FindTableRow<FS_Quest>(ETableType::tb_Quest, FString::FromInt(Data.GetQuestID()));
		if (QuestTable != nullptr && QuestTable->MissionCount > 0)
		{
			auto pQuestData = mapQuestData.Find(Data.GetQuestID());
			if (pQuestData != nullptr)
			{
				pQuestData->SetCurrentIndex(QuestTable->MissionIDList.Find(Data.GetQuestMissionID()));
			}
		}

		// 퀘스트 액터 활성화 설정
		ActiveQuestActorList();
	}

	// 퀘스트 보드 갱신
	UpdateQuestBoardUI(true);
}

void USDKQuestManager::UpdateProgressingQuestMissionData(const TArray<CQuestMissionData>& Data)
{
	if (Data.Num() <= 0)
	{
		return;
	}

	for (auto& itData : Data)
	{
		int32 ID = itData.GetQuestMissionID();
		if (mapQuestMissionData.Contains(itData.GetQuestMissionID()))
		{
			mapQuestMissionData[ID] = itData;
		}
		else
		{
			mapQuestMissionData.Add(ID, itData);
		}
	}

	// 퀘스트 보드 갱신
	UpdateQuestBoardUI();
}

void USDKQuestManager::EndQuestMissionData(int32 QuestMissionID, bool bIsLast, const CQuestMissionData& Data)
{
	UE_LOG(LogQuest, Log, TEXT("EndQuestMissionData : %d"), QuestMissionID);
	
	CQuestMissionData* pEndMissionData = mapQuestMissionData.Find(QuestMissionID);
	if (pEndMissionData != nullptr)
	{
		pEndMissionData->SetEndQuestMission();
		SetQuestMissionData(*pEndMissionData);
	}
	
	// 미션 타입에 대한 퀘스트 정보 정리
	TArray<int32> EndIDs = { QuestMissionID };
	SetActiveQuestMissionActors(EQuestState::End, EndIDs);

	SortQuestMissioIDListByQuestType(ProgressingMissionIDList);
	SortQuestMissioIDListByQuestType(CompletableMissionIDList);

	if (bIsLast)
	{
		// 마지막 퀘스트 미션이였을 경우, 퀘스트 종료
		USDKGameInstance* SDKGameInstance = Cast<USDKGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
		if (IsValid(SDKGameInstance))
		{
			SDKGameInstance->LobbyServerManager->CQ_QuestEnd(pEndMissionData->GetQuestID());
		}
	}
	else
	{
		if (Data.GetQuestMissionID() > 0)
		{
			// 자동 수락일 경우,
			UpdateStartedQuestMissionData(Data);
		}
		else
		{
			CQuestData* pQuestData = mapQuestData.Find(pEndMissionData->GetQuestID());
			if(pQuestData != nullptr)
			{
				// 수동 수락일 경우,
				FS_Quest* QuestTable = USDKTableManager::Get()->FindTableRow<FS_Quest>(ETableType::tb_Quest, FString::FromInt(pQuestData->GetQuestID()));
				if (QuestTable != nullptr && QuestTable->MissionCount > 1)
				{
					int32 CurIndex = QuestTable->MissionIDList.Find(pEndMissionData->GetQuestMissionID());
					if (CurIndex > INDEX_NONE)
					{
						// 시작 가능한 퀘스트 미션에 추가
						SetStartableQuestMissionList(pQuestData->GetQuestID(), CurIndex + 1);
					}
				}
			}
		}
	}

	if (mapQuestMissionActor.Contains(QuestMissionID) && mapQuestMissionActor[QuestMissionID].IsValid())
	{
		AddDelegateHandle(QuestMissionID, EndQuestActorEventCallback.AddWeakLambda(this, [&] { CompletedEndQuestActorEvent(QuestMissionID); }));
	}
	else
	{
		CompletedEndQuestActorEvent(QuestMissionID);
	}
}

void USDKQuestManager::SetProgressingQuestMissionOperateUIType(int32 QuestMissionID, EUI InUI)
{
	// 미완 작업
	ASDKPlayerController* PlayerController = Cast<ASDKPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(PlayerController))
	{
		ASDKHUD* SDKHUD = PlayerController->GetHUD<ASDKHUD>();
		if (IsValid(SDKHUD))
		{
			USDKUserWidget* OperatedWidget = SDKHUD->OpenUI(InUI);
			if (IsValid(OperatedWidget) == true)
			{
				OperatedWidget->TutorialUserWidget();
			}
		}
	}
}

void USDKQuestManager::CompletedEndQuestActorEvent(int32 InQuestMissionID)
{
	// 퀘스트 델리게이트 제거
	RemoveDelegateHandle(InQuestMissionID);
	
	// 지역 이동이 필요한 경우
	FS_QuestMission* QMissionTable = USDKTableManager::Get()->FindTableRow<FS_QuestMission>(ETableType::tb_QuestMission, FString::FromInt(InQuestMissionID));
	if (QMissionTable != nullptr && QMissionTable->TargetMapPath.IsNone() == false && QMissionTable->IsForcedMove)
	{
		USDKGameInstance* SDKGameInstance = GetWorld()->GetGameInstance<USDKGameInstance>();
		if (IsValid(SDKGameInstance))
		{
			if (QMissionTable->TargetMapPath == FName("OpenWorld"))
			{
				SDKGameInstance->OnEnterAdventure();
			}
		}
	}
	
	// 퀘스트 보드 갱신
	UpdateQuestBoardUI();
}

void USDKQuestManager::ActiveQuestActorList()
{
	if (CurrentMode == EGameMode::None || CurrentMode == EGameMode::Lobby)
	{
		return;
	}

	if (StartableMissionIDList.Num() > 0)
	{
		SetActiveQuestMissionActors(EQuestState::Available, StartableMissionIDList);
	}

	if (ProgressingMissionIDList.Num() > 0)
	{
		SetActiveQuestMissionActors(EQuestState::Progressing, ProgressingMissionIDList);
		SetActiveQuestMissionActors(EQuestState::Condition, ProgressingMissionIDList);
	}

	if (CompletableMissionIDList.Num() > 0)
	{
		SetActiveQuestMissionActors(EQuestState::Complete, CompletableMissionIDList);
	}

	if (CurrentMode < EGameMode::Beg_Ingame)
	{
		if (EndQuestIDList.Num() > 0)
		{
			SetActiveQuestActors(EQuestState::End, EndQuestIDList);
		}
	}

	/*// 로그라이크에서 보스 몬스터 이후, 진행되아햐는 퀘스트가 있는지에 대한 확인
	if (FSDKHelpers::GetGameMode(GetWorld()) == EGameMode::Rpg)
	{
		ASDKRpgState* SDKRpgState = GetWorld()->GetGameState<ASDKRpgState>();
		if (IsValid(SDKRpgState) && SDKRpgState->GetCurrentMapType() == FName("Boss") && GetActivedQuestActorCount() <= 0)
		{
			EndQuestActorEventCallback.ExecuteIfBound();
			EndQuestActorEventCallback.Unbind();
		}
	}*/
}

void USDKQuestManager::RemoveQuestActor(ASDKQuest* Actor)
{
	UE_LOG(LogQuest, Log, TEXT("RemoveQuestActor : %s"), *Actor->GetName());

	if (mapQuestMissionActor.Num() > 0)
	{
		if (mapQuestMissionActor.FindKey(Actor) != nullptr)
		{
			int32 QuestMissionID = *mapQuestMissionActor.FindKey(Actor);
			mapQuestMissionActor.Remove(QuestMissionID);

			
			RemoveDelegateHandle(QuestMissionID);
		}
	}

	if (mapQuestActor.Num() > 0)
	{
		if (mapQuestActor.FindKey(Actor) != nullptr)
		{
			mapQuestActor.Remove(*mapQuestActor.FindKey(Actor));
		}
	}
}

void USDKQuestManager::ResetQuestActorList()
{
	if (mapQuestActor.Num() > 0)
	{
		for (auto itQuest : mapQuestActor)
		{
			if (itQuest.Value.IsValid())
			{
				itQuest.Value->SetLifeSpan(0.01f);
			}
		}

		mapQuestActor.Empty();
	}

	if (mapQuestMissionActor.Num() > 0)
	{
		for (auto itMission : mapQuestMissionActor)
		{
			if (itMission.Value.IsValid())
			{
				itMission.Value->SetLifeSpan(0.01f);
			}
		}

		mapQuestMissionActor.Empty();
	}

	CurrentMode = FSDKHelpers::GetGameMode(GetWorld());

	// 이벤트 초기화
	EndQuestActorEventCallback.Clear();
}

void USDKQuestManager::UpdateQuestBoardUI(bool bNew /*= false*/)
{
	ASDKPlayerController* SDKPlayerController = Cast<ASDKPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKPlayerController))
	{
		SDKPlayerController->ClientUpdateQuestBoardUI(false);
	}
}

void USDKQuestManager::AddDelegateHandle(int32 InID, const FDelegateHandle& InHandle)
{
	if (DelegateHandleList.Contains(InID))
	{
		EndQuestActorEventCallback.Remove(DelegateHandleList[InID]);
		DelegateHandleList[InID] = InHandle;
	}
	else
	{
		DelegateHandleList.Add(InID, InHandle);
	}
}

void USDKQuestManager::RemoveDelegateHandle(int32 InID)
{
	if (DelegateHandleList.Contains(InID))
	{
		EndQuestActorEventCallback.Remove(DelegateHandleList[InID]);
		DelegateHandleList.Remove(InID);
	}
}

void USDKQuestManager::ClearQuestManager()
{
	// 퀘스트 액터 제거
	ResetQuestActorList();

	mapSavedMissions.Empty();

	StartableMissionIDList.Empty();

	ProgressingQuestIDList.Empty();
	ProgressingMissionIDList.Empty();

	CompletableQuestIDList.Empty();
	CompletableMissionIDList.Empty();

	EndQuestIDList.Empty();
	EndMissionIDList.Empty();

	mapQuestData.Empty();
	mapQuestMissionData.Empty();

	CurrentMode = EGameMode::None;
}

void USDKQuestManager::EDITOR_ResetCheatClearQuest()
{
	if (mapQuestActor.Num() > 0)
	{
		for (auto itQuest : mapQuestActor)
		{
			if (itQuest.Value.IsValid())
			{
				itQuest.Value->SetLifeSpan(0.01f);
			}
		}
	}

	if (mapQuestMissionActor.Num() > 0)
	{
		for (auto itMission : mapQuestActor)
		{
			if (itMission.Value.IsValid())
			{
				itMission.Value->SetLifeSpan(0.01f);
			}
		}
	}
}

void USDKQuestManager::SetQuestData(const CQuestData& Data)
{
	if (Data.GetQuestID() <= 0)
	{
		return;
	}

	UE_LOG(LogQuest, Log, TEXT("SetQuestData : %d"), Data.GetQuestID());

	int32 QuestID = Data.GetQuestID();
	if (!mapQuestData.Contains(QuestID))
	{
		mapQuestData.Add(QuestID, Data);
	}
	else
	{
		mapQuestData[QuestID] = Data;
	}

	// 완료 가능 OR 진행 중인 퀘스트 인지 & 상태에 따라서 이전 상태 리스트에서 제거
	if (Data.GetIsCompletedQuest())
	{
		if (!Data.GetIsReceivedReward())
		{
			CompletableQuestIDList.AddUnique(QuestID);

			if (ProgressingQuestIDList.Contains(QuestID))
			{
				ProgressingQuestIDList.Remove(QuestID);
			}
		}
		else
		{
			EndQuestIDList.AddUnique(QuestID);

			if (CompletableQuestIDList.Contains(QuestID))
			{
				CompletableQuestIDList.Remove(QuestID);
			}
		}
	}
	else
	{
		// 진행중인 퀘스트 ID
		if(!Data.GetIsReceivedReward())
		{
			ProgressingQuestIDList.AddUnique(QuestID);
		}
	}
}

void USDKQuestManager::SetQuestMissionData(const CQuestMissionData& Data)
{
	if (Data.GetQuestMissionID() <= 0)
	{
		return;
	}

	int32 QuestMissionID = Data.GetQuestMissionID();
	if (!mapQuestMissionData.Contains(QuestMissionID))
	{
		mapQuestMissionData.Add(QuestMissionID, Data);
	}
	else
	{
		mapQuestMissionData[QuestMissionID] = Data;
	}

	// 완료 가능 OR 진행 중인 퀘스트 인지 & 상태에 따라서 이전 상태 리스트에서 제거
	if (Data.GetIsCompletedQuestMission())
	{
		if (!Data.GetIsReceivedReward())
		{
			CompletableMissionIDList.AddUnique(QuestMissionID);
		}
		else
		{
			EndMissionIDList.AddUnique(QuestMissionID);

			if (CompletableMissionIDList.Contains(QuestMissionID))
			{
				CompletableMissionIDList.Remove(QuestMissionID);
			}
		}

		// 완료할 수 있거나 완료된 상태일 때,
		if (StartableMissionIDList.Contains(QuestMissionID))
		{
			StartableMissionIDList.Remove(QuestMissionID);
		}

		if (ProgressingMissionIDList.Contains(QuestMissionID))
		{
			ProgressingMissionIDList.Remove(QuestMissionID);
		}
	}
	else
	{
		if (!Data.GetIsReceivedReward())
		{
			ProgressingMissionIDList.AddUnique(QuestMissionID);

			if (StartableMissionIDList.Contains(QuestMissionID))
			{
				StartableMissionIDList.Remove(QuestMissionID);
			}
		}
	}
}

void USDKQuestManager::SetUnlockContentsByEndQuest(int32 QuestID)
{
	USDKGameInstance* SDKGameInstance = GetWorld()->GetGameInstance<USDKGameInstance>();
	if (IsValid(SDKGameInstance))
	{
		FS_Quest* QuestTable = USDKTableManager::Get()->FindTableRow<FS_Quest>(ETableType::tb_Quest, FString::FromInt(QuestID));
		if (QuestTable != nullptr)
		{
			bool bUnlocked = false;
			if (mapQuestData.Contains(QuestID))
			{
				bUnlocked = mapQuestData[QuestID].GetIsReceivedReward();
			}

			if (!QuestTable->UnlockedContents.IsEmpty() && QuestTable->UnlockedContents != TEXT("0"))
			{
				SDKGameInstance->MyInfoManager->GetUnlockContentsData().SetUnlockedContents(QuestTable->UnlockedContents, bUnlocked);
			}

			if (QuestTable->UnlokedUI != EMenuIconType::None)
			{
				SDKGameInstance->MyInfoManager->GetUnlockContentsData().SetUnlockedContentsByMenuType(QuestTable->UnlokedUI, bUnlocked);
			}
		}
	}
}

void USDKQuestManager::SetActiveQuestActors(EQuestState State, const TArray<int32>& IDList)
{
	if (IDList.Num() <= 0)
	{
		return;
	}

	for (auto& itID : IDList)
	{
		auto QuestTable = USDKTableManager::Get()->FindTableRow<FS_Quest>(ETableType::tb_Quest, FString::FromInt(itID));
		if (QuestTable != nullptr)
		{
			// 퀘스트 액터 경로 
			FString ClassPath = TEXT("");
			EQuestState TagState = State;
			if (State == EQuestState::End)
			{
				ClassPath = QuestTable->EndingQuestActor.ToString();
			}

			if (!ClassPath.IsEmpty())
			{
				// 이미 퀘스트 액터가 있는 경우
				if (mapQuestActor.Contains(itID) && mapQuestActor[itID].IsValid())
				{
					EQuestState ActorQuestState = mapQuestActor[itID]->GetQuestState();
					if (ActorQuestState == State || (ActorQuestState == EQuestState::Available || ActorQuestState == EQuestState::Progressing))
					{
						continue;
					}
					else
					{
						mapQuestActor[itID].Get()->SetLifeSpan(0.01f);
						RemoveQuestActor(mapQuestActor[itID].Get());
					}
				}

				auto QuestClass = LoadClass<ASDKQuest>(GetWorld(), *ClassPath);
				if (IsValid(QuestClass))
				{
					FString SpawnTag = QuestClass->GetName().LeftChop(2);

					TArray<AActor*> TagActors;
					UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), ATargetPoint::StaticClass(), FName(SpawnTag), TagActors);
					if (TagActors.Num() <= 0)
					{
						UE_LOG(LogQuest, Log, TEXT("Not Found Quest Target Point : %d"), itID);
						continue;
					}

					if (!mapQuestMissionActor.Contains(itID))
					{
						auto NewQuestActor = GetWorld()->SpawnActorDeferred<ASDKQuest>(QuestClass, TagActors[0]->GetActorTransform());
						if (IsValid(NewQuestActor))
						{
							NewQuestActor->SetQuestID(itID);
							NewQuestActor->SetQuestState(State);

							UGameplayStatics::FinishSpawningActor(NewQuestActor, TagActors[0]->GetActorTransform());

							mapQuestActor.Add(itID, NewQuestActor);
						}
					}
				}
			}
		}
	}
}

void USDKQuestManager::SetActiveQuestMissionActors(EQuestState State, const TArray<int32>& IDList)
{
	if (IDList.Num() <= 0)
	{
		return;
	}

	if (!IsValid(GetWorld()))
	{
		return;
	}

	TArray<FString> LevelIDs;
	auto SDKRpgMode = GetWorld()->GetAuthGameMode<ASDKRpgGameMode>();
	if (IsValid(SDKRpgMode))
	{
		LevelIDs = SDKRpgMode->GetLoadedLevelIDs();
	}

	FString LevelName = UGameplayStatics::GetCurrentLevelName(GetWorld());
	for (auto& itID : IDList)
	{
		auto QuestMissionTable = USDKTableManager::Get()->FindTableRow<FS_QuestMission>(ETableType::tb_QuestMission, FString::FromInt(itID));
		if (QuestMissionTable != nullptr)
		{
			if (!QuestMissionTable->LevelID.IsNone() && !LevelIDs.Contains(QuestMissionTable->LevelID.ToString()))
			{
				// 특정 레벨ID에서 스폰해야되는 경우
				continue;
			}
			else if (!QuestMissionTable->LevelString.IsNone() && LevelName.Compare(QuestMissionTable->LevelString.ToString()) != 0)
			{
				// 특정 레벨에서 스폰해야되는 경우
				continue;
			}

			// 퀘스트 액터 경로 
			TSoftClassPtr<ASDKQuest> ClassPath;
			if (State == EQuestState::Available || State == EQuestState::Progressing)
			{
				ClassPath = QuestMissionTable->StartQuestActor;
			}
			else if (State == EQuestState::Condition)
			{
				ClassPath = QuestMissionTable->ConditionQuestActor;
			}
			else if (State == EQuestState::Complete)
			{
				ClassPath = QuestMissionTable->CompleteQuestActor;
			}
			else
			{
				ClassPath = QuestMissionTable->EndQuestActor;
			}

			if(!ClassPath.IsNull())
			{
				// 이미 퀘스트 액터가 있는 경우
				if (mapQuestMissionActor.Contains(itID))
				{
					if (State == EQuestState::Progressing)
					{
						// 퀘스트가 진행중인 상태인 경우
						if (mapQuestMissionActor[itID].IsValid() && mapQuestMissionActor[itID].Get()->GetQuestState() == EQuestState::Available)
						{
							mapQuestMissionActor[itID].Get()->SetQuestState(State);

							continue;
						}
					}
					
					if (mapQuestMissionActor[itID].IsValid() && mapQuestMissionActor[itID]->GetQuestState() == State)
					{
						continue;
					}
					else
					{
						// 그 외 상황에서는 제거
						if(mapQuestMissionActor[itID].IsValid())
						{
							mapQuestMissionActor[itID].Get()->SetLifeSpan(0.01f);
						}

						mapQuestMissionActor.Remove(itID);
					}
				}

				TSubclassOf<ASDKQuest> QuestClass(*USDKAssetManager::Get().LoadSynchronous(ClassPath));
				if (IsValid(QuestClass))
				{
					FString SpawnTag = QuestClass->GetName().LeftChop(2);

					TArray<AActor*> TagActors;
					UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), ATargetPoint::StaticClass(), FName(SpawnTag), TagActors);
					if (TagActors.Num() <= 0)
					{
						UE_LOG(LogQuest, Log, TEXT("Not Found Quest Target Point : %d / %s"), itID, *SpawnTag);

						UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATargetPoint::StaticClass(), TagActors);
						if (TagActors.Num() > 0)
						{
							int32 i = 0;
						}

						continue;
					}

					if(!mapQuestMissionActor.Contains(itID))
					{
						auto NewQuestActor = GetWorld()->SpawnActorDeferred<ASDKQuest>(QuestClass, TagActors[0]->GetActorTransform());
						if (IsValid(NewQuestActor))
						{
							NewQuestActor->SetQuestMissionID(itID);
							NewQuestActor->SetQuestState(State);

							UGameplayStatics::FinishSpawningActor(NewQuestActor, TagActors[0]->GetActorTransform());

							mapQuestMissionActor.Add(itID, NewQuestActor);
						}
					}
				}
			}
			else
			{
				if (mapQuestMissionActor.Contains(itID))
				{
					if (mapQuestMissionActor[itID].IsValid() && mapQuestMissionActor[itID]->GetQuestState() == State)
					{
						continue;
					}
					else
					{
						// 퀘스트가 끝난 상태일 때만 제거
						if (mapQuestMissionActor[itID].IsValid())
						{
							if (State != EQuestState::End)
							{
								continue;
							}

							mapQuestMissionActor[itID].Get()->SetLifeSpan(0.01f);
						}

						mapQuestMissionActor.Remove(itID);
					}
				}
			}

			if (State == EQuestState::Progressing && QuestMissionTable->MissionType == EQuestMissionType::OperateUI && mapQuestMissionActor.Contains(itID) == false)
			{
				// 퀘스트 : UI 조작인 경우
				SetProgressingQuestMissionOperateUIType(itID, QuestMissionTable->LinkUI);
			}
			else if (State == EQuestState::Condition && itID == 1030060101)
			{
				SetProgressingQuestMissionOperateUIType(itID, QuestMissionTable->LinkUI);
			}
		}
	}
}

void USDKQuestManager::SortQuestMissioIDListByQuestType(TArray<int32>& IDList)
{
	if (IDList.Num() <= 0)
	{
		return;
	}

	TMap<int32, CQuestMissionData> mapData = mapQuestMissionData;
	IDList.Sort([mapData](const int32 A, const int32 B)
		{
			EQuestType Type_A = EQuestType::Max;
			EQuestType Type_B = EQuestType::Max;

			CQuestMissionData Data_A = mapData[A];
			auto QuestTable_A = USDKTableManager::Get()->FindTableRow<FS_Quest>(ETableType::tb_Quest, FString::FromInt(Data_A.GetQuestID()));
			if (QuestTable_A != nullptr)
			{
				Type_A = QuestTable_A->QuestType;
			}

			CQuestMissionData Data_B = mapData[B];
			auto QuestTable_B = USDKTableManager::Get()->FindTableRow<FS_Quest>(ETableType::tb_Quest, FString::FromInt(Data_B.GetQuestID()));
			if (QuestTable_B != nullptr)
			{
				Type_B = QuestTable_B->QuestType;
			}

			if (Type_A == Type_B)
			{
				return A < B;
			}

			return Type_A < Type_B;
		});
}
