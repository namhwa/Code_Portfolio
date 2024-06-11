// Fill out your copyright notice in the Description page of Project Settings.

#include "MultiMode/SDKGoldClashState.h"

#include "Engine/Level.h"
#include "Engine/LevelScriptActor.h"
#include "Engine/LevelStreaming.h"

#include "UObject/CoreNet.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/OutputDeviceNull.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"

#include "MultiMode/SDKGoldClashMode.h"
#include "Share/SDKEnum.h"
#include "Character/SDKMultiGameController.h"
#include "Engine/SDKGameInstance.h"


ASDKGoldClashState::ASDKGoldClashState()
{
	GoldClashPlayTime = 0;
	GameOverTime = 0;
	CurrInValidRegion = INDEX_NONE;
	GoldCrownTeamNumber= INDEX_NONE;
	bFinishedTimerOver = false;
	GoldClashBoxRegenPreTime = 10;
}

void ASDKGoldClashState::BeginPlay()
{
	Super::BeginPlay();

	TRACE_BOOKMARK(TEXT("GoldClash BeginPlay"));

#if !WITH_EDITOR
	// 선 로딩 목록 로딩
#endif

	// 1등 디버프 시작 골드값 설정
	const auto GDTable = USDKTableManager::Get()->FindTableGlobalDefine(EGlobalDefine::GoldClashGameRules);
	if(GDTable && GDTable->Value.IsValidIndex(2))
	{
		DebuffStartGold = FCString::Atoi(*GDTable->Value[2]);
	}
}

void ASDKGoldClashState::OnGameStart()
{
	Super::OnGameStart();	

	// 인게임 UI로 Attach 및 게임 플레이 타임 설정
	for(FConstPlayerControllerIterator ItController = GetWorld()->GetPlayerControllerIterator(); ItController; ++ItController)
	{
		ASDKMultiGameController* MultiGameController = Cast<ASDKMultiGameController>(ItController->Get());
		if(IsValid(MultiGameController))
		{
			MultiGameController->ClientChangeIngameMainUI();
			MultiGameController->Client_GoldClashStartNotifyUI();

			if (GoldClashPlayTime > 0)
			{
				MultiGameController->ClientGamePlayingTimer(GoldClashPlayTime);
			}

			// 유물 상점 리스트 설정
			MultiGameController->Server_SetRelicsShopList();

			// 파티 멤버 초기화
			if (CurrentMatchType == EMatchType::Team)
			{
				MultiGameController->ServerInitPartyMemberData();
			}
		}
	}

	// 골드 크래시 랭킹 초기화
	if (!CheckValiditySortPlayerArray())
	{
		SortPlayerArray.Empty();
	}

	// 골클상자 리젠 알림시간 초기화
	FS_GlobalDefine* GlobalDefineTable = USDKTableManager::Get()->FindTableGlobalDefine(EGlobalDefine::GoldClashEventPreTime);
	if (GlobalDefineTable)
	{
		if (GlobalDefineTable->Value.IsValidIndex(0))
		{
			if (!GlobalDefineTable->Value[0].IsEmpty())
			{
				GoldClashBoxRegenPreTime = FCString::Atoi(*GlobalDefineTable->Value[0]);
			}
		}
	}

	// 랭킹 갱신
	CheckChangeRanker();

	// 랭커 위치 갱신
	if (GetWorldTimerManager().TimerExists(TimerHandle_RankerPosition) == false)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_RankerPosition, this, &ASDKGoldClashState::UpdateRankerPosition, 1.f / 60.f, true);
	}
}

void ASDKGoldClashState::OnGameReady()
{
	Super::OnGameReady();

	// 대기 UI Attach 및 시간 설정
	for(FConstPlayerControllerIterator ItController = GetWorld()->GetPlayerControllerIterator(); ItController; ++ItController)
	{
		ASDKMultiGameController* MultiGameController = Cast<ASDKMultiGameController>(ItController->Get());
		if(IsValid(MultiGameController))
		{
			// 준비 상태의 UI로 변경
			MultiGameController->ClientChangeWaitingRoomUI(EGameplayState::Ready);
		}
	}
}

void ASDKGoldClashState::FinishGame()
{
	UE_LOG(LogArena, Log, TEXT("ASDKGoldClashState::FinishGame"));

	GetWorldTimerManager().ClearTimer(TimerHandle_RankerPosition);
	GetWorldTimerManager().ClearTimer(TimerHandle_GameOver);
	GetWorldTimerManager().ClearTimer(TimerHandle_UpdateRanking);

	auto MultiGameController = Cast<ASDKMultiGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(MultiGameController))
	{
		// 상점 팝업 닫기 처리
		MultiGameController->ClientRespawnShopClose();
	}

	// SDKGoldClashMode::FinishGame() 호출
}

void ASDKGoldClashState::OnGamePlaying()
{
	Super::OnGamePlaying();

	if (IsGameActive() == false)
	{
		return;
	}

	GoldClashPlayTime -= 1;

	bool bPlayCountDown = false;
	if (GoldClashPlayTime <= 10)
	{
		if (GameOverTime > GoldClashPlayTime || !GetWorldTimerManager().IsTimerActive(TimerHandle_GameOver))
		{
			bPlayCountDown = true;
		}
	}

	for (FConstPlayerControllerIterator ItController = GetWorld()->GetPlayerControllerIterator(); ItController; ++ItController)
	{
		ASDKMultiGameController* MultiGameController = Cast<ASDKMultiGameController>(ItController->Get());
		if (IsValid(MultiGameController))
		{
			// 각 클라이언트에 남은 시간 갱신
			MultiGameController->ClientGamePlayingTimer(GoldClashPlayTime, bPlayCountDown);
		}
	}

	if (GetLocalRole() == ROLE_Authority)
	{
		if (GoldClashPlayTime <= 0)
		{
			SetFinishedTimeOver(true);
			FinishGame();
			return;
		}
	}

	ASDKGoldClashMode* SDKGoldClashMode = GetWorld()->GetAuthGameMode<ASDKGoldClashMode>();
	if (IsValid(SDKGoldClashMode))
	{
		PassiveGoldElapsedTime += 1.f;
		
		if (PassiveGoldElapsedTime >= SDKGoldClashMode->GetPassiveGoldIntervalTime())
		{
			// 패시브 골드
			SDKGoldClashMode->UpdateDefaultPassiveGold(MapPlayerRankInfo);
			PassiveGoldElapsedTime = 0;
		}
	}

	// 파티 정보 갱신
	UpdatePartyMember();	
}

void ASDKGoldClashState::UpdatePartyMember()
{
	if (CurrentMatchType != EMatchType::Team)
	{
		return;
	}

	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		ASDKMultiGameController* SDKMultiController = Cast<ASDKMultiGameController>(It->Get());
		if (IsValid(SDKMultiController))
		{
			// 파티멤버 정보 갱신
			SDKMultiController->UpdatePartyMemberData();
		}
	}
}

void ASDKGoldClashState::UpdateGoldRank()
{
	TArray<FGoldClashResultInfo> RankingInfoList;
	MapPlayerRankInfo.GenerateValueArray(RankingInfoList);

	// 랭킹 UI 갱신
	for (FConstPlayerControllerIterator ItController = GetWorld()->GetPlayerControllerIterator(); ItController; ++ItController)
	{
		ASDKMultiGameController* SDKMultiController = Cast<ASDKMultiGameController>(ItController->Get());
		if (IsValid(SDKMultiController))
		{
			SDKMultiController->ClientUpdateGoldClashRank(RankingInfoList);
		}
	}

	if (RankingInfoList.Num() > 0)
	{
		const bool bHaveRanker = (RankingInfoList.IsValidIndex(1) == true) ? RankingInfoList[0].CurrentCoin > RankingInfoList[1].CurrentCoin : true;

		ASDKGoldClashMode* SDKGoldClashMode = GetWorld()->GetAuthGameMode<ASDKGoldClashMode>();
		if (IsValid(SDKGoldClashMode))
		{
			SDKGoldClashMode->CheckModeFinishCondition((bHaveRanker ? RankingInfoList[0].UserID : 0), (bHaveRanker ? RankingInfoList[0].CurrentCoin : -1));
		}
		else
		{
			UE_LOG(LogArena, Error, TEXT("GoldClash_CheckModeFinishCondition Faild"));
		}
	}
}

void ASDKGoldClashState::UpdateRanker(const int32 NewRankerID)
{
	ASDKGoldClashMode* SDKGoldClashMode = GetWorld()->GetAuthGameMode<ASDKGoldClashMode>();
	if (IsValid(SDKGoldClashMode))
	{
		// 골드 크래시 현황판 왕관 교체 호출
		SDKGoldClashMode->ChangeNo1(CurNo1, NewRankerID);
	}

	// 골드 MVP 교체
	if(CurNo1 != NewRankerID)
	{
		for (auto ItController = GetWorld()->GetPlayerControllerIterator(); ItController; ++ItController)
		{
			ASDKMultiGameController* SDKMultiController = Cast<ASDKMultiGameController>(ItController->Get());
			if (IsValid(SDKMultiController))
			{
				// UI 갱신
				SDKMultiController->Client_ChangeMatchInfoNo1(CurNo1, NewRankerID);
			}
		}

		// 1등 갱신
		CurNo1 = NewRankerID;
	}
}

void ASDKGoldClashState::UpdateRankerPosition()
{
	if (MapPlayerRankInfo.Num() > 0)
	{
		const auto ItRanker = MapPlayerRankInfo.CreateConstIterator();
		const int32 RankerID = ItRanker->Key;
		FVector RankerLoc = FVector::ZeroVector;

		const ASDKGoldClashMode* SDKGoldClashMode = GetWorld()->GetAuthGameMode<ASDKGoldClashMode>();
		if (IsValid(SDKGoldClashMode))
		{
			const AController* RankerController = SDKGoldClashMode->GetPlayingController(RankerID);
			if (IsValid(RankerController) && IsValid(RankerController->GetPawn()))
			{
				RankerLoc = RankerController->GetPawn()->GetActorLocation();
			}
		}

		for (auto ItController = GetWorld()->GetPlayerControllerIterator(); ItController; ++ItController)
		{
			ASDKMultiGameController* SDKMultiController = Cast<ASDKMultiGameController>(ItController->Get());
			if (IsValid(SDKMultiController))
			{
				// 1등 위치 표시 UI 갱신
				SDKMultiController->ClientUpdateGoldClashRankerTransform(RankerLoc, RankerID, ItRanker->Value.TeamNumber);
			}
		}
	}
}

void ASDKGoldClashState::SetGoldClashPlayingTime(int32 InTime)
{
	GoldClashPlayTime = InTime;
}

void ASDKGoldClashState::NotifyUpdateGoldRegenRegion(TArray<int32> RegenIndex)
{
	if (IsGameActive() == true)
	{
		GoldRegenRegionList = RegenIndex;

		for (auto ItController = GetWorld()->GetPlayerControllerIterator(); ItController; ++ItController)
		{
			ASDKMultiGameController* SDKMultiController = Cast<ASDKMultiGameController>(ItController->Get());
			if (IsValid(SDKMultiController))
			{
				SDKMultiController->ClientUpdateGoldRegenRegion(RegenIndex);
			}
		}
	}
}

ASDKPlayerCharacter* ASDKGoldClashState::GetRankPlayer(const int32 TargetRank)
{
	if (TargetRank <= 0)
	{
		return nullptr;
	}

	const ASDKGoldClashMode* SDKGoldClashMode = GetWorld()->GetAuthGameMode<ASDKGoldClashMode>();
	if (!IsValid(SDKGoldClashMode))
	{
		return nullptr;
	}

	// 랭킹 인덱스
	int32 RankIndex = 0;

	// 랭킹 정보
	TArray<int32> RankIDs;
	MapPlayerRankInfo.GenerateKeyArray(RankIDs);

	if (RankIDs.Num() <= 0 || !RankIDs.IsValidIndex(TargetRank-1))
	{
		return nullptr;
	}

	const int32 TargetID = RankIDs[TargetRank-1];

	const AController* RankerController = SDKGoldClashMode->GetPlayingController(TargetID);
	if (!IsValid(RankerController))
	{
		return nullptr;
	}

	ASDKPlayerCharacter* TargetPlayer = Cast<ASDKPlayerCharacter>(RankerController->GetPawn());
	if (!IsValid(TargetPlayer))
	{
		return nullptr;
	}

	return TargetPlayer;
}

TArray<int32> ASDKGoldClashState::GetTeamMemberData(int32 InTeam)
{
	if (MapTeamMembers.Contains(InTeam))
	{
		return MapTeamMembers[InTeam];
	}

	return TArray<int32>();
}

void ASDKGoldClashState::AddTeamMemberData(int32 InTeam, int32 InSpawn)
{
	if (MapTeamMembers.Contains(InTeam) == false)
	{
		MapTeamMembers.Add(InTeam, { InSpawn });
	}
	else
	{
		MapTeamMembers[InTeam].AddUnique(InSpawn);
	}
}

void ASDKGoldClashState::RemoveTeamMemberData(int32 InTeam, int32 InSpawn)
{
	if (MapTeamMembers.Contains(InTeam))
	{
		MapTeamMembers[InTeam].Remove(InSpawn);
	}
}

void ASDKGoldClashState::RemovePlayerRankInfo(int32 InSpawnNum)
{
	if (MapPlayerRankInfo.Contains(InSpawnNum))
	{
		MapPlayerRankInfo.Remove(InSpawnNum);
		MapPlayerRankInfo.Compact();
		MapPlayerRankInfo.Shrink();
	}

	CheckChangeRanker();
}

void ASDKGoldClashState::NotifyLeaveUser(int32 InTeam, FString InNick)
{
	if (IsGameActive() == true)
	{
		for (auto ItController = GetWorld()->GetPlayerControllerIterator(); ItController; ++ItController)
		{
			ASDKMultiGameController* SDKMultiController = Cast<ASDKMultiGameController>(ItController->Get());
			if (IsValid(SDKMultiController) == false || SDKMultiController->GetTeamNumber() != InTeam)
			{
				continue;
			}

			if (MapTeamMembers.Contains(InTeam))
			{
				SDKMultiController->ClientNotifyLeaveUser(InNick, MapTeamMembers[InTeam].Num() <= 3);
			}
		}
	}
}

void ASDKGoldClashState::NotifyDisconnectUser(int32 InTeam, FString InNickName)
{
	if (IsGameActive() == true)
	{
		for (auto ItController = GetWorld()->GetPlayerControllerIterator(); ItController; ++ItController)
		{
			ASDKMultiGameController* SDKMultiController = Cast<ASDKMultiGameController>(ItController->Get());
			if (IsValid(SDKMultiController) == false || SDKMultiController->GetTeamNumber() != InTeam)
			{
				continue;
			}

			SDKMultiController->ClientNotifyDisconnectUser(InNickName);
		}
	}
}

void ASDKGoldClashState::NotifyReconnectUser(int32 InTeam, FString InNickName)
{
	if (IsGameActive() == true)
	{
		for (auto ItController = GetWorld()->GetPlayerControllerIterator(); ItController; ++ItController)
		{
			ASDKMultiGameController* SDKMultiController = Cast<ASDKMultiGameController>(ItController->Get());
			if (IsValid(SDKMultiController) == false || SDKMultiController->GetTeamNumber() != InTeam)
			{
				continue;
			}

			SDKMultiController->ClientNotifyReconnectUser(InNickName);
		}
	}
}

void ASDKGoldClashState::NotifyOpenSurrenderVote(int32 InTeam)
{
	if (IsGameActive() == true)
	{
		ASDKGoldClashMode* GoldClashMode = GetWorld()->GetAuthGameMode<ASDKGoldClashMode>();
		if (IsValid(GoldClashMode))
		{
			for (auto ItController = GetWorld()->GetPlayerControllerIterator(); ItController; ++ItController)
			{
				ASDKMultiGameController* SDKMultiController = Cast<ASDKMultiGameController>(ItController->Get());
				if (IsValid(SDKMultiController) == false || SDKMultiController->GetTeamNumber() != InTeam)
				{
					continue;
				}

				SDKMultiController->ClientToggleSurrenderVote(true);
			}
		}

		TaskAfterSeconds(this, [this, InTeam]()
		{
			FinishSurrenderVote(InTeam);
		}, 15.f);
	}
}

void ASDKGoldClashState::UpdateSurrenderVote(int32 InTeamNumber, int32 InUID, bool bResult)
{
	if (MapSurrenderVote.Contains(bResult) == false)
	{
		MapSurrenderVote.Add(bResult);
	}

	MapSurrenderVote[bResult].AddUnique(InUID);

	if (MapSurrenderVote.Num() > 0)
	{
		int32 TotalCount = 0;
		for (auto& Iter : MapSurrenderVote)
		{
			TotalCount += Iter.Value.Num();
		}

		ASDKGameMode* SDKGameMode = GetWorld()->GetAuthGameMode<ASDKGameMode>();
		if (IsValid(SDKGameMode))
		{
			int32 TeamMemberCount = SDKGameMode->GetTeamMemberCount(InTeamNumber);
			int32 AIMemberCount = SDKGameMode->GetAITeamMemberCount(InTeamNumber);

			if (TotalCount >= (TeamMemberCount - AIMemberCount))
			{
				for (FConstPlayerControllerIterator ItController = GetWorld()->GetPlayerControllerIterator(); ItController; ++ItController)
				{
					ASDKMultiGameController* MultiController = Cast<ASDKMultiGameController>(ItController->Get());
					if (IsValid(MultiController))
					{
						MultiController->ClientNotifyAllMemberSurrenderVote();
					}
				}

				FinishSurrenderVote(InTeamNumber);
			}
		}
	}
}

void ASDKGoldClashState::FinishSurrenderVote(int32 InTeamNumber)
{
	if (MapSurrenderVote.Num() <= 0)
	{
		UE_LOG(LogGame, Log, TEXT("Map Surrender Vote Is Empty!"));
		return;
	}

	int32 SurrenderCount = MapSurrenderVote.Contains(true) ? MapSurrenderVote[true].Num() : 0;
	int32 KeepGoingCount = MapSurrenderVote.Contains(false) ? MapSurrenderVote[false].Num() : 0;

	// 팀 인원과 결과 값 갱신
	ASDKGameMode* SDKGameMode = GetWorld()->GetAuthGameMode<ASDKGameMode>();
	if (IsValid(SDKGameMode))
	{
		int32 UserCount = SDKGameMode->GetTeamMemberCount(InTeamNumber) - SDKGameMode->GetAITeamMemberCount(InTeamNumber);
		if ((SurrenderCount + KeepGoingCount) == UserCount)
		{
			UE_LOG(LogGame, Log, TEXT("Surrender Result : %s Lose Team : %d"), SurrenderCount > KeepGoingCount ? TEXT("True") : TEXT("False"), InTeamNumber);

			NotifyResultSurrenderVote(SurrenderCount > KeepGoingCount, InTeamNumber);
		}

		MapSurrenderVote.Empty();
	}
}

void ASDKGoldClashState::NotifyResultSurrenderVote(bool bSurrender, int32 InTeamNumber)
{
	for (FConstPlayerControllerIterator ItController = GetWorld()->GetPlayerControllerIterator(); ItController; ++ItController)
	{
		ASDKMultiGameController* MultiController = Cast<ASDKMultiGameController>(ItController->Get());
		if (IsValid(MultiController))
		{
			if (bSurrender)
			{
				if (MultiController->GetTeamNumber() != InTeamNumber)
				{
					MultiController->ClientInstantMessageTableText(TEXT("UIText_GoldClash_SurrenderEnemy"), 3.f);
				}
				else
				{
					MultiController->ClientInstantMessageTableText(TEXT("UIText_GoldClash_Surrender"), 3.f);
				}
			}
		}
	}

	if (bSurrender)
	{
		TaskAfterSeconds(this, [this, InTeamNumber]()
		{
			GiveUpGame(InTeamNumber);
		}, 3.f);
	}
}

void ASDKGoldClashState::CheckChangeRanker()
{
	TMap<int32, FGoldClashResultInfo> MapCurrentRankInfo;
	SortGoldClashRank(MapCurrentRankInfo);
	
	// 등수 갱신
	MapPlayerRankInfo = MapCurrentRankInfo;

	// 새로운 1등 찾기
	int32 NewNo1 = 0;
	int32 NewNo1Coin = 0;
	if (MapPlayerRankInfo.Num() > 0)
	{
		const auto CurrentFirst = MapPlayerRankInfo.CreateConstIterator();
		if(CurrentFirst)
		{
			NewNo1 = CurrentFirst->Key;
			NewNo1Coin = CurrentFirst->Value.CurrentCoin;
		}
	}

	// 1등 갱신
	if ((CurNo1 != NewNo1 && NewNo1Coin != 0) || NewNo1Coin >= DebuffStartGold || DebuffGoldFirst)
	{
		UpdateRanker(NewNo1);
	}

	// UI 갱신
	UpdateGoldRank();
}

void ASDKGoldClashState::SortPlayerArrayByGold(TArray<ASDKPlayerState*>& PlayerStateArray)
{
	PlayerStateArray.StableSort([](const ASDKPlayerState& A, const ASDKPlayerState& B)
	{
		if (A.GetGold() == B.GetGold())
		{
			return A.GetGainGoldTime() < B.GetGainGoldTime();
		}
		else
		{
			return A.GetGold() > B.GetGold();
		}
	});
}

void ASDKGoldClashState::SortGoldClashRank(TMap<int32, FGoldClashResultInfo>& MapSortRanking)
{
	UpdateSortPlayerArray();
	SortPlayerArrayByGold(SortPlayerArray);

	for (const auto& SDKPlayerState : SortPlayerArray)
	{
		if (IsValid(SDKPlayerState) && !SDKPlayerState->IsOnlyASpectator())
		{
			const int32 SpawnNum = SDKPlayerState->GetSpawnNumber();
			const int32 TeamNum = SDKPlayerState->GetTeamNumber();
			const int32 CurCoin = SDKPlayerState->GetGold();
			const FString NickName = SDKPlayerState->GetNickName();
			MapSortRanking.Emplace(SpawnNum, FGoldClashResultInfo(SpawnNum, TeamNum, CurCoin, NickName));
		}
	}
}

void ASDKGoldClashState::UpdateSortPlayerArray()
{
	if (SortPlayerArray.Num() == 0 || SortPlayerArray.Num() != PlayerArray.Num())
	{
		SortPlayerArray.Empty();
		for (const auto& Iter : PlayerArray)
		{
			ASDKPlayerState* SDKPlayerState = Cast<ASDKPlayerState>(Iter);
			if (IsValid(SDKPlayerState))
			{
				SortPlayerArray.Emplace(SDKPlayerState);
			}
		}
	}
}

bool ASDKGoldClashState::CheckValiditySortPlayerArray()
{
	for (const auto& Iter : SortPlayerArray)
	{
		if (!IsValid(Iter))
		{
			return false;
		}
	}
	return true;
}

void ASDKGoldClashState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASDKGoldClashState, MatchInfoList);
	DOREPLIFETIME(ASDKGoldClashState, CurrentMatchType);
	DOREPLIFETIME(ASDKGoldClashState, GoldClashPlayTime);
	DOREPLIFETIME(ASDKGoldClashState, CurNo1);
	DOREPLIFETIME(ASDKGoldClashState, bDestroyGoldBox);
	DOREPLIFETIME(ASDKGoldClashState, GoldCrownTeamNumber);
	DOREPLIFETIME(ASDKGoldClashState, GoldRegenRegionList);
	DOREPLIFETIME(ASDKGoldClashState, CurrInValidRegion);	
}
