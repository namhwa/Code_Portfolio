// Fill out your copyright notice in the Description page of Project Settings.


#include "SingleMode_Roguelike/SDKRpgState.h"

#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/OutputDeviceNull.h"
#include "Net/UnrealNetwork.h"

#include "Character/SDKHUD.h"
#include "Character/SDKMyInfo.h"
#include "Character/SDKRpgPlayerState.h"
#include "Character/SDKPlayerCharacter.h"
#include "Character/SDKInGameController.h"

#include "Engine/TargetPoint.h"
#include "Engine/SDKGameInstance.h"

#include "GameMode/SDKRpgGameMode.h"
#include "GameMode/SDKSectorLocation.h"
#include "GameMode/SDKRoguelikeSector.h"
#include "GameMode/SDKRoguelikeManager.h"
#include "Item/SDKMovingItem.h"

#include "Manager/SDKTableManager.h"
#include "Manager/SDKLobbyServerManager.h"
#include "Manager/SDKPopupWidgetManager.h"

#include "Object/SDKNetworkConnectionRestorer.h"
#include "Object/SDKObject.h"
#include "Share/SDKHelper.h"

#include "UI/SDKLevelUpBuffWidget.h"
#include "UI/SDKInGameMainUIInterface.h"
#include "UI/SDKServerReconnectPopupWidget.h"


ASDKRpgState::ASDKRpgState()
{
	PuzzleTime = 0;
	bPuzzleStart = false;

	bDiceGameStart = false;

	bPetBattleStart = false;
	bDieEnd = false;
}

void ASDKRpgState::OnGameReady()
{
	Super::OnGameReady();

	// 게임 타이머 활성화
	GetWorldTimerManager().SetTimer(TimerHandle_OnGameReady, this, &ASDKGameState::OnGameReadying, 1.f, true);
}

void ASDKRpgState::OnGameStart()
{
	Super::OnGameStart();

	StartIngame();
}

void ASDKRpgState::OnGamePlaying()
{
	Super::OnGamePlaying();

	if (PuzzleTime > 0)
	{
		PlayingPuzzleGame();
	}
}

void ASDKRpgState::OnCharDied()
{
	UE_LOG(LogGame, Log, TEXT("Character Died"));

	if (IsGameActive())
	{
		RpgDeathEvent.Broadcast();

		bDieEnd = true;
		ASDKRpgGameMode* RpgGameMode = GetWorld()->GetAuthGameMode<ASDKRpgGameMode>();
		if (IsValid(RpgGameMode) == true)
		{
			RpgGameMode->SetCharacterDied(true);
			RpgGameMode->ClearCurrentRoom();
		}
	}
}

void ASDKRpgState::SetPlayingRoomPhase(bool bPlaying)
{
	PlayingRoomPhase = bPlaying;
}

void ASDKRpgState::SetCurrentMapIndex(int32 iIndex)
{
	CurrentMapIndex = iIndex;
}

void ASDKRpgState::SetCurrentMapType(FName eType)
{
	CurrentMapType = eType;

	ASDKInGameController* InGameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(InGameController))
	{
		bool bCamp = UGameplayStatics::GetCurrentLevelName(GetWorld()).Find(TEXT("Camp")) > INDEX_NONE;

		InGameController->ClientChangeGlitchGoldType(bCamp);
	}
}

void ASDKRpgState::SetChapterID(FString newID)
{
	ChapterID = newID;
}

void ASDKRpgState::SetStageID(FString InStage)
{
	StageID = InStage;
}

void ASDKRpgState::SetCurrentSectorName(FString Name)
{
	CurrentSectorName = Name;
}

FString ASDKRpgState::GetCurrentSectorName()
{
	return CurrentSectorName;
}

TArray<int32> ASDKRpgState::GetDroppableStoreItemIDs(int32 DropID) const
{
	if(mapDroppableIDs.Contains(DropID) == true)
	{
		TArray<int32> ResultIDs = mapDroppableIDs[DropID];
		if(ResultIDs.Num() > 0)
		{
			auto InGameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
			if(IsValid(InGameController))
			{
				for(auto& itInven : InGameController->GetInventoryItemIDs())
				{
					auto ItemTable = USDKTableManager::Get()->FindTableRow<FS_Item>(ETableType::tb_Item,FString::FromInt(itInven));
					if (ItemTable)
					{
						if (ItemTable->ItemType == EItemType::Artifact)
						{
							if (ResultIDs.Contains(itInven) == true)
							{
								ResultIDs.Remove(itInven);
							}
						}
					}
				}
			}
		}

		return ResultIDs;
	}

	return TArray<int32>();
}

void ASDKRpgState::SetDroppableStoreItemIDs(int32 DropID, bool bIsBlackMarket, bool bSpawnDarkArtifact)
{
	if(DropID <= 0)
		return;

	if(mapDroppableIDs.Contains(DropID) == false)
	{
		mapDroppableIDs.Add(DropID);
	}

	TArray<int32> DropGroupIDs;
	FSDKHelpers::GetDropGroupIDs(GetWorld(), DropID, DropGroupIDs, bIsBlackMarket, bSpawnDarkArtifact);

	mapDroppableIDs[DropID] = DropGroupIDs;
}

bool ASDKRpgState::SetDroppedStoreItemID(int32 DropID, int32 ItemID, bool bIsBlackMarket, bool bDarkArtifact)
{
	TArray<int32> DropGroupIDs;
	FSDKHelpers::GetDropGroupIDs(GetWorld(), DropID, DropGroupIDs, bIsBlackMarket, bDarkArtifact);

	mapDroppableIDs[DropID] = DropGroupIDs;

	if(mapDroppableIDs.Num() > 0)
	{
		if(mapDroppableIDs.Contains(DropID) == true)
		{
			auto ArtifactTable = USDKTableManager::Get()->FindTableRow<FS_Artifact>(ETableType::tb_Artifact, FString::FromInt(ItemID));
			if (bIsBlackMarket)
			{
				for (int32 i = 0; i < mapDroppableIDs[DropID].Num(); i++)
				{
					if (ItemID == mapDroppableIDs[DropID][i])
					{
						break;
					}
				}
			}

			if (mapDroppableIDs[DropID].Contains(ItemID) == true)
			{
				return mapDroppableIDs[DropID].Remove(ItemID) >= 0;
			}
		}
	}

	return false;
}

void ASDKRpgState::ResetDroppableStoreItemIDs(int32 DropID, bool bIsBlackMarket, bool bSpawnDarkArtifact)
{
	if(mapDroppableIDs.Num() > 0)
	{
		if(mapDroppableIDs.Contains(DropID) == true)
		{
			if(GetDroppableStoreItemIDs(DropID).Num() <= 0)
			{
				SetDroppableStoreItemIDs(DropID, bIsBlackMarket);
			}
		}
	}
}

TArray<FArtifactData> ASDKRpgState::GetDropReadyArtifactList()
{
	return DropReadyArtifactList;
}

TArray<FArtifactData> ASDKRpgState::GetDropReadyDarkArtifactList()
{
	return DropReadyDarkArtifactList;
}

TArray<FArtifactData> ASDKRpgState::GetNearArtifactSetComplateList()
{
	CheckRemoveArtifactList();
	TArray<FArtifactData> TempDropReadyList = GetDropReadyArtifactList();

	auto InGameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(InGameController))
	{
		TArray<int32> InvenArtifactList = InGameController->GetInventoryArtifactList();

		// 인벤 유물의 세트와 그 카운트
		TMap<int32, int32> SetID_SetCountMap;

		for(int32 i = 0; i < InvenArtifactList.Num(); i++)
		{
			auto ArtifactTable = USDKTableManager::Get()->FindTableRow<FS_Artifact>(ETableType::tb_Artifact, FString::FromInt(InvenArtifactList[i]));

			if(ArtifactTable != nullptr)
			{
				// 세트 정보가 없으면 하나 추가
				if (SetID_SetCountMap.Contains(ArtifactTable->ArtifactSet[0]) == false)
				{
					SetID_SetCountMap.Add(ArtifactTable->ArtifactSet[0], 1);
				}
				else
				{
					// 세트 정보가 있으면 세트 개수 카운트 증가
					SetID_SetCountMap[ArtifactTable->ArtifactSet[0]]++;
				}

			}
		}

		int32 RequiredMinCount = 9;
		for (auto SetIt = SetID_SetCountMap.CreateIterator(); SetIt; ++SetIt)
		{
			auto SetTable = USDKTableManager::Get()->FindTableRow<FS_ItemSet>(ETableType::tb_ItemSet, FString::FromInt(SetIt->Key));

			if (SetTable != nullptr)
			{
				SetIt->Value = SetTable->SetCount - SetIt->Value;

				// 이미 완성된 세트는 제외 시킴
				if (SetIt->Value <= 0)
				{
					SetID_SetCountMap.Remove(SetIt->Key);
					continue;
				}

				// 최소로 요구되는 세트 갯수를 찾는다.
				if (RequiredMinCount > SetIt->Value)
				{
					RequiredMinCount = SetIt->Value;
				}
			}
		}

		// 최소 요구치보다 큰 세트는 제외시킴.
		for (auto SetIt = SetID_SetCountMap.CreateIterator(); SetIt; ++SetIt)
		{
			if (SetIt->Value > RequiredMinCount)
			{
				SetID_SetCountMap.Remove(SetIt->Key);
			}
		}

		TArray<FArtifactData> MinSetArtifactList;
		// 최소로 구성된 리스트 구성 준비
		for (int32 i = 0; i < TempDropReadyList.Num(); i++)
		{
			FS_Artifact* ArtifactTable = USDKTableManager::Get()->FindTableRow<FS_Artifact>(ETableType::tb_Artifact, FString::FromInt(TempDropReadyList[i].ArtifactID));
			if (ArtifactTable != nullptr)
			{
				if (SetID_SetCountMap.Contains(ArtifactTable->ArtifactSet[0]) == true)
				{
					if (MinSetArtifactList.Contains(TempDropReadyList[i]) == false)
					{
						MinSetArtifactList.Add(TempDropReadyList[i]);
						continue;
					}
				}
			}
		}

		if (MinSetArtifactList.Num() > 0)
		{
			return MinSetArtifactList;
		}
	}

	return TempDropReadyList;
}

TArray<int32> ASDKRpgState::GetDropReadyArtifactIDList()
{
	TArray<int32> IDList;
	for(auto Artifact : DropReadyArtifactList)
	{
		IDList.Add(Artifact.ArtifactID);
	}
	
	return IDList;
}

TArray<int32> ASDKRpgState::GetDropReadyDarkArtifactIDList()
{
	TArray<int32> HaveIDList;
	TArray<int32> IDList;

	ASDKInGameController* IngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(IngameController))
	{
		HaveIDList = IngameController->GetInventoryArtifactList();
	}
	
	for (auto Artifact = DropReadyDarkArtifactList.CreateIterator(); Artifact; ++Artifact)
	{
		if (HaveIDList.Num() > 0 && HaveIDList.Contains(Artifact->ArtifactID))
		{
			Artifact.RemoveCurrent();
			continue;
		}

		IDList.Add(Artifact->ArtifactID);
	}

	return IDList;
}

TArray<int32> ASDKRpgState::GetNearArtifactSetComplateIDList()
{
	TArray<FArtifactData> ArtifactData = GetNearArtifactSetComplateList();
	TArray<int32> IDList;

	for (auto Artifact : ArtifactData)
	{
		IDList.Add(Artifact.ArtifactID);
	}

	return IDList;

}

void ASDKRpgState::CheckRemoveArtifactList()
{
	auto InGameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(InGameController))
	{
		TArray<int32> InvenArtifactList = InGameController->GetInventoryArtifactList();

		for(int32 i = 0; i < InvenArtifactList.Num(); i++)
		{
			if (InGameController->IsSameArtifactInventory(InvenArtifactList[i]) == true)
			{
				DropReadyArtifactList.Remove(InvenArtifactList[i]);
			}
		}
	}

	// 드롭 유물리스트가 비워지면 다시 갱신.
	if (DropReadyArtifactList.Num() <= 0)
	{
		SetDropReadyArtifactList();
	}
}

void ASDKRpgState::AddDropReadyArtifact(int32 ArtifactID)
{
	if (USDKMyInfo::Get().GetArtifactData().CheckHaveArtifact(ArtifactID))
	{
		if (DropReadyArtifactList.Contains(ArtifactID) == false)
		{
			DropReadyArtifactList.Add(ArtifactID);
		}
	}
}

void ASDKRpgState::RemoveDropReadyArtifact(int32 ItemID, bool bDarkArtifact)
{
	// 드롭 유물리스트가 비워지면 다시 갱신.
	if(bDarkArtifact)
	{
		if (DropReadyDarkArtifactList.Num() <= 1)
		{
			SetDropReadyDarkArtifactList();
		}

		if (DropReadyDarkArtifactList.Contains(ItemID) == true)
		{
			DropReadyDarkArtifactList.Remove(ItemID);
		}
	}
	else
	{
		if (DropReadyArtifactList.Num() <= 1)
		{
			SetDropReadyArtifactList();
		}

		if (DropReadyArtifactList.Contains(ItemID) == true)
		{
			DropReadyArtifactList.Remove(ItemID);
		}
	}
}

void ASDKRpgState::InitFreeArtifactCount()
{
	ASDKInGameController* SDKIngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKIngameController))
	{
		ASDKPlayerState* SDKPlayerState = Cast<ASDKPlayerState>(SDKIngameController->PlayerState);
		if (IsValid(SDKPlayerState))
		{
			ArtifactFreeCount = SDKPlayerState->GetAttAbilityValue(EParameter::ShopFreeBuy);
		}
	}
}

int32 ASDKRpgState::GetFreeArtifactCount()
{
	return ArtifactFreeCount;
}

void ASDKRpgState::SetBuyFreeArtifactCount()
{
	ArtifactFreeCount -= 1;
}

void ASDKRpgState::SetDropReadyArtifactList()
{
	TArray<FPlayerArtifactData> PlayerArtifactList; 
	ASDKInGameController* SDKIngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKIngameController))
	{
		ASDKPlayerState* SDKPlayerState = Cast<ASDKPlayerState>(SDKIngameController->PlayerState);
		if (IsValid(SDKPlayerState))
		{
			PlayerArtifactList =  SDKPlayerState->GetPlayerArtifactList();
		}
	}

	USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (IsValid(SDKGameInstance))
	{
		TArray<FArtifactData> AllList;

		for(auto Iter : PlayerArtifactList)
		{
			AllList.Add(FArtifactData(Iter.ArtifactID, Iter.Enhance, Iter.Stack));
		}

#if WITH_EDITOR
		// 에디터? 테스트용 코드
		if(AllList.Num() <= 0)
		{
			TArray<FName> ArtifactName = USDKTableManager::Get()->GetRowNames(ETableType::tb_Artifact);

			for(auto Iter : ArtifactName)
			{
				FS_Artifact* ArtifactTable = USDKTableManager::Get()->FindTableRow<FS_Artifact>(ETableType::tb_Artifact, Iter.ToString());

				if(ArtifactTable != nullptr)
				{
					if(ArtifactTable->Default)
					{
						FArtifactData ArtifactData;
						ArtifactData.ArtifactID = FSDKHelpers::StringToInt32(Iter.ToString());
						ArtifactData.Enhance = 0;
						AllList.Add(ArtifactData);
					}
				}
			}
		}
#endif
		TArray<FArtifactData> PVEList;
		FName ChallengeName = SDKGameInstance->MyInfoManager->GetRPGModeData().GetChallengeSeasonID();
		auto ChallengeTb = USDKTableManager::Get()->FindTableRow<FS_ChallengeRpgSeason>(ETableType::tb_ChallengeRpgSeason, ChallengeName.ToString());

		for (int32 i = 0; i < AllList.Num(); i++)
		{
			FS_Artifact* ItemTb = USDKTableManager::Get()->FindTableRow<FS_Artifact>(ETableType::tb_Artifact, FString::FromInt(AllList[i].GetTableID()));

			if (ItemTb != nullptr)
			{
				// 도전모드에서 Ban되어야할 대상을 제외처리. if문 myinfo에서 contentsType 가져와서 수정해야함.
				if (ChallengeTb != nullptr)
				{
					bool IsBan = false;
					for (int32 BanIdx = 0; BanIdx < ChallengeTb->BanArtifact.Num(); BanIdx++)
					{
						if (ItemTb->ArtifactSet[0] == FSDKHelpers::StringToInt32(ChallengeTb->BanArtifact[BanIdx].ToString()))
						{
							IsBan = true;
							break;
						}
					}

					if (IsBan == false)
					{
						PVEList.AddUnique(AllList[i]);
					}
				}
				else
				{
					PVEList.AddUnique(AllList[i]);
				}
			}
		}

		DropReadyArtifactList = PVEList;
	}
}

void ASDKRpgState::SetDropReadyDarkArtifactList()
{
	// 블랙마켓이 활성화 되었을 때 가지고 오도록 함.
	// 아이템 테이블에서 추려 오면 됌.
	//DropReadyBlackMarketArtifactList

	TArray<FArtifactData> DarkArtifactID;
	TArray<FString> LegendaryIDs = USDKTableManager::Get()->GetLegendaryArtifactList();
	if (LegendaryIDs.Num() > 0)
	{
		for (auto& Iter : LegendaryIDs)
		{
			int32 IterID = FCString::Atoi(*Iter);
			FArtifactData FindData = USDKMyInfo::Get().GetArtifactData().GetArtifactData(IterID);
			if (FindData.ArtifactID <= 0)
			{
				DarkArtifactID.Add(FArtifactData(IterID));
			}
			else
			{
				DarkArtifactID.Add(USDKMyInfo::Get().GetArtifactData().GetArtifactData(IterID));
			}
		}
	}

	DropReadyDarkArtifactList = DarkArtifactID;
}

void ASDKRpgState::CheckRpgGame(bool bClear)
{
	IsClearChapter = bClear;

	// GameplayState 가 EGameplayState::Playing 이 아니면서, 래밸업버프위젯도 켜져있지 않으면
	if (GameplayState == EGameplayState::Finished && !IsOpenLevelUpBuff())
	{
		return;
	}

	UE_LOG(LogGame, Log, TEXT("Check RPG Game State : %s"), bClear ? TEXT("TRUE") : TEXT("FALSE"));

	ASDKRpgGameMode* SDKRpgMode = GetWorld()->GetAuthGameMode<ASDKRpgGameMode>();
	if (IsValid(SDKRpgMode))
	{
		SDKRpgMode->SetClearChapter(bClear);
	}

	// 보스 네임 HUD 제거
	for(FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		auto SDKPlayerController = Cast<ASDKInGameController>(It->Get());
		if(IsValid(SDKPlayerController) == true)
		{
			SDKPlayerController->ClientBossDie();
		}
	}

	if(GetLocalRole() == ROLE_Authority)
	{
		FinishGame();
	}
}

void ASDKRpgState::CheckRemaindItems()
{
	ASDKInGameController* InGameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(InGameController) == true)
	{
		ASDKPlayerCharacter* MyCharacter = Cast<ASDKPlayerCharacter>(InGameController->GetPawn());
		if (IsValid(MyCharacter) == true)
		{
			MyCharacter->MultiGainGoldRadius(3000.f);
		}
	}

	// 획득 반경 복원
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUFunction(this, FName("AfterCheckRemainItems"));
	GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 3.f, false);
}

void ASDKRpgState::ClearSectorPhase()
{
	// GameplayState 가 EGameplayState::Playing 이 아니면서, 래밸업버프위젯도 켜져있지 않으면
	if (!IsGameActive() && !IsOpenLevelUpBuff())
	{
		return;
	}

	UE_LOG(LogGame, Log, TEXT("RPG Stage : Completed Room Clear"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("RPG Stage : Completed Room Clear"));

	ASDKRpgGameMode* SDKRpgMode = GetWorld()->GetAuthGameMode<ASDKRpgGameMode>();
	if (IsValid(SDKRpgMode) == true)
	{
		SetPlayingRoomPhase(false);

		int32 CurIndex = SDKRpgMode->GetRoguelikeManager()->GetCurrentIndex() + 1;
		int32 MaxIndex = SDKRpgMode->GetRoguelikeManager()->GetSectorCount();

		// 룸 클리어
		if (CurIndex >= MaxIndex)
		{
			SDKRpgMode->SetClearRoom(true);
			SDKRpgMode->ActiveRoomReward(MaxIndex > 0);
			SDKRpgMode->GetRoguelikeManager()->ClearRoguelikeSectorList();

			if (CurrentMapType != TEXT("Start"))
			{
				OpenUIClearCurrentSector();
			}
		}

		if (CurrentMapType == TEXT("Boss"))
		{
			CheckRemaindItems();
			SDKRpgMode->SetClearRoom(true);
			SDKRpgMode->ClearCurrentRoom();
		}
	}
}

bool ASDKRpgState::IsOpenLevelUpBuff()
{
	if (GetGamePlayState() == EGameplayState::Pause) 
	{
		ASDKInGameController* SDKIngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		if (IsValid(SDKIngameController))
		{
			ASDKHUD* SDKHUD = SDKIngameController->GetHUD<ASDKHUD>();
			if (IsValid(SDKHUD))
			{
				ISDKInGameMainUIInterface* ModeMainWidget = Cast<ISDKInGameMainUIInterface>(SDKHUD->GetModeMainUI());
				if (ModeMainWidget != nullptr)
				{
					USDKLevelUpBuffWidget* LevelUpBuffWidget = ModeMainWidget->GetLevelUpBuffListWidget();
					if (IsValid(LevelUpBuffWidget))
					{
						if (LevelUpBuffWidget->GetVisibility() != ESlateVisibility::Collapsed)
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

void ASDKRpgState::AfterCheckRemainItems()
{
	ASDKInGameController* InGameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(InGameController) == true)
	{
		ASDKPlayerCharacter* MyCharacter = Cast<ASDKPlayerCharacter>(InGameController->GetPawn());
		if (IsValid(MyCharacter) == true)
		{
			MyCharacter->MultiGainGoldRadius(GOLD_DEFAULT_RANGE + MyCharacter->GetPawnData()->ObjectGoldGainRange);
		}
	}
}

void ASDKRpgState::AddClearStateInfo(FString chapterID, int32 roomIndex)
{
	ASDKRpgGameMode* SDKRpgGameMode = GetWorld()->GetAuthGameMode<ASDKRpgGameMode>();
	if (IsValid(SDKRpgGameMode))
	{
		if (SDKRpgGameMode->GetClearRoom())
		{
			roomIndex += 1;
		}

		mapClearStateInfo.Emplace(chapterID, roomIndex);
	}

	/*if (ClearStageMap == true)
	{
		roomIndex += 1;
	}*/
}

const FVector2D ASDKRpgState::GetPlayerAttAbility(EParameter Parameter)
{
	ASDKInGameController* SDKIngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKIngameController))
	{
		ASDKPlayerState* SDKPlayerState = Cast<ASDKPlayerState>(SDKIngameController->PlayerState);
		if (IsValid(SDKPlayerState))
		{
			return SDKPlayerState->GetAttAbility(Parameter);
		}
	}

	return FVector2D::ZeroVector;
}

void ASDKRpgState::AddSelectedRoomInfo(int32 Row, int32 Column)
{
	if (mapSelectedRoom.Contains(Row) == false)
	{
		mapSelectedRoom.Emplace(Row, Column);
	}

	// 현재 진행중인 인덱스
	CurrentSelectIndex = Row;

	ASDKInGameController* SDKIngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKIngameController))
	{
		SDKIngameController->ClientUpdateRpgModeMap();
	}
}

void ASDKRpgState::SetCurrentSelectIndex(int32 NewIndex)
{
	if (CurrentSelectIndex != NewIndex)
	{
		CurrentSelectIndex = NewIndex;
	}
}

void ASDKRpgState::FinishEnterRPGSequnce()
{
	ASDKRpgGameMode* SDKRpgMode = GetWorld()->GetAuthGameMode<ASDKRpgGameMode>();
	if (IsValid(SDKRpgMode))
	{
		ASDKInGameController* SDKIngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		if (IsValid(SDKIngameController))
		{
			ASDKHUD* SDKHUD = SDKIngameController->GetHUD<ASDKHUD>();
			if (IsValid(SDKHUD))
			{
				SDKHUD->CloseUI(EUI::FloorTitle);
			}

			USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
			if (IsValid(SDKGameInstance) == true)
			{
				if (SDKGameInstance->GetPlayingLevelSequence() == true)
				{
					SDKIngameController->ClientToggleWidgetBySequence(true);
				}
			}
		}
	}
}

void ASDKRpgState::OpenUIClearCurrentSector()
{
	if (CurrentMapType == FName("Store"))
	{
		return;
	}

	ASDKRpgGameMode* RpgGameMode = GetWorld()->GetAuthGameMode<ASDKRpgGameMode>();
	if (IsValid(RpgGameMode) == true)
	{
		if (RpgGameMode->GetRoguelikeManager()->GetCurrentIndex() < 0)
		{
			return;
		}
	}

	ASDKInGameController* IngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(IngameController) == true)
	{
		ASDKHUD* SDKHUD = IngameController->GetHUD<ASDKHUD>();
		if (IsValid(SDKHUD) == true)
		{
			if (SDKHUD->IsOpenUI(EUI::UI_ScriptScene) == false)
			{
				SDKHUD->OpenUI(EUI::Popup_ClearRoom);
			}
		}
	}
}

void ASDKRpgState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASDKRpgState, ChapterID);
	DOREPLIFETIME(ASDKRpgState, StageID);
	DOREPLIFETIME(ASDKRpgState, Floor);
	DOREPLIFETIME(ASDKRpgState, DifficultyID);

	DOREPLIFETIME(ASDKRpgState, PlayingRoomPhase);

	DOREPLIFETIME(ASDKRpgState, CurrentMapIndex);
	DOREPLIFETIME(ASDKRpgState, CurrentMapType);
}
