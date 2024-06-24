// Fill out your copyright notice in the Description page of Project Settings.

#include "MultiMode/SDKGoldClashMode.h"

#include "NavigationSystem.h"
#include "Arena/Arena.h"
#include "Engine/LevelScriptActor.h"
#include "Engine/SDKGameTask.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/OutputDeviceNull.h"

#include "GameMode/SDKGoldClashState.h"
#include "Engine/SDKDeveloperSettings.h"
#include "Engine/SDKGameInstance.h"
#include "Engine/LevelStreaming.h"

#include "Manager/SDKBiskitEventLogManager.h"
#include "Manager/SDKCoinMaster.h"
#include "Manager/SDKDedicatedServerManager.h"
#include "Manager/SDKMissionManager.h"
#include "Manager/SDKServerToServerManager.h"
#include "Manager/SDKTableManager.h"
#include "Share/SDKData.h"
#include "Share/SDKDataTable.h"
#include "Share/SDKHelper.h"

#include "Character/SDKInGameController.h"
#include "Character/SDKMultiGameController.h"
#include "Character/SDKMyInfo.h"
#include "Character/SDKPlayerCharacter.h"
#include "Character/SDKPlayerStart.h"
#include "Character/SDKPlayerState.h"

#include "AI/SDKHeroAIController.h"
#include "Delegate/SDKGameEventDelegate.h"
#include "Delegate/SDKGameEventStruct.h"

#include "Item/SDKMovingItem.h"
#include "Object/SDKObject.h"
#include "Object/SDKWeaponChangeObject.h"
#include "Object/SDKDestroyComponent.h"

DEFINE_LOG_CATEGORY(LogGoldClash)


void ASDKGoldClashMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// 데디 옵션 파싱

	// 골드 리젠 규칙 초기화
	GoldRegenRuleList.Empty();

	TMap<FName, uint8*> GoldClashRuleTableRowMap = USDKTableManager::Get()->GetRowMap(ETableType::tb_GoldClashRule);
	if (GoldClashRuleTableRowMap.Num() > 0)
	{
		for (auto& Iter : GoldClashRuleTableRowMap)
		{
			const auto RuleTable = reinterpret_cast<FS_GoldClashRule*>(Iter.Value);
			if (!RuleTable || RuleTable->ModeType == EGameMode::None)
			{
				continue;
			}

			if (RuleTable->ModeType == EGameMode::GoldClash)
			{
				GoldRegenRuleList.Add(Iter.Key);
			}
		}
	}

	if (GoldRegenRuleList.Num() > 1)
	{
		CurRegenRuleIndex = 0;
		NextRegenRuleIndex = CurRegenRuleIndex + 1;
	}
}

void ASDKGoldClashMode::Logout(AController* Exiting)
{
	int32 PlayerID = Exiting->PlayerState->GetPlayerId();
	UE_LOG(LogArena, Log, TEXT("LogOut GoldClash : %d"), PlayerID);

	ASDKGoldClashState* SDKGoldClashState = Cast<ASDKGoldClashState>(GameState);
	if (IsValid(SDKGoldClashState))
	{
		ASDKMultiGameController* ExitController = Cast<ASDKMultiGameController>(Exiting);
		if (IsValid(ExitController))
		{
			ExitController->ServerRemovePet();

			ASDKPlayerState* SDKPlayerState = Cast<ASDKPlayerState>(ExitController->PlayerState);
			if (IsValid(SDKPlayerState))
			{
				if (!SDKPlayerState->IsOnlyASpectator())
				{
					// 나간 유저 데이터 저장
					const FPlayerGameInfo GameInfo = SDKPlayerState->GetPlayerGameInfo();
					const int32 Level = SDKPlayerState->GetLevel();
					const FPlayerHeroData HeroData = SDKPlayerState->GetHeroData();
					const FPlayerHeroSkinData HeroSkinData = SDKPlayerState->GetSkinData();
					const int32 PlayerId = SDKPlayerState->GetPlayerId();
					const int32 CurGold = SDKPlayerState->GetGold();
					const int32 TotalGetGold = SDKPlayerState->GetTotalGetGold();
					const int32 TotalUseGold = SDKPlayerState->GetTotalUseGold();
					const int32 PrevRankPoint = SDKPlayerState->GetRankPoint();

					FGoldClashResultInfo ExitPlayerInfo = FGoldClashResultInfo(FSDKHelpers::GetGameMode(GetWorld()), GameInfo, Level, CurGold, TotalGetGold, TotalUseGold, ExitController->GetInventoryItemCountByType(EItemType::Artifact), ExitController->GetWeaponGrade(), HeroSkinData, HeroData, 0, SDKPlayerState->GetRankPoint(), SDKGoldClashState->IsCustomMatch(), SDKPlayerState->GetExpressionAutoPlayData(), 0);

					// 나간 유저 정보 저장
					AddMapExitPlayerRankInfoData(ExitPlayerInfo);
					AddMapExitPlayerRankPointData(PlayerId, FRankPointData(SDKPlayerState->GetTeamNumber(), PrevRankPoint, SDKPlayerState->GetWinningStreak(), SDKPlayerState->GetNumOfKill(), SDKPlayerState->GetNumOfDeath()));

					// 경쟁전이거나 게임이 진행중인 경우
					if (!SDKGoldClashState->IsCustomMatch() && (SDKGoldClashState->IsGameActive() || SDKGoldClashState->IsGameReady()))
					{
						// 패널티로 강퇴된 유저가 아닌 경우만 정상 루트 처리
						if (!ExitController->GetHavePenaltyPoint())
						{
							if (MapExitPlayerRankInfoData.Contains(PlayerId))
							{
								FS_GlobalDefine* GDTable = USDKTableManager::Get()->FindTableGlobalDefine(EGlobalDefine::DodgeTime);
								if (GDTable && GDTable->Value.IsValidIndex(0))
								{
									MapExitPlayerRankInfoData[PlayerId].LimitTime = FCString::Atoi(*GDTable->Value[0]);
								}
								else
								{
									MapExitPlayerRankInfoData[PlayerId].LimitTime = DODGE_LIMIT_TIME;
								}
							}

							UE_LOG(LogGame, Warning, TEXT("Exit PlayerId : %d"), PlayerId);
						}
						else
						{	
							if (MapExitPlayerRankPointData.Contains(PlayerId))
							{
#if !UE_BUILD_SHIPPING
								UE_LOG(LogGoldClash, Log, TEXT("Input Penalty"));
#endif
								SendMultiGamePenaltyUserInfo(ExitPlayerInfo, MapExitPlayerRankPointData[PlayerId]);
							}
						}
					}

					// 데디에게 유저가 나갔다고 신호
					LeaveGame(SDKPlayerState->GetNickName(), SDKPlayerState->GetSpawnNumber(), SDKPlayerState->GetTeamNumber(), SDKPlayerState->GetNickName());
				}
			}
		}
	}

	Super::Logout(Exiting);
}

void ASDKGoldClashMode::ServerReadyComplete()
{
	Super::ServerReadyComplete();

	// 대기 타이머 시작 (준비 끝나면 ReadyGame 호출)
	GetWorldTimerManager().SetTimer(TimerHandle_OnWaitTime, this, &ASDKGameMode::OnWaitTime, 1.f, true, 1.f);

	// 코인 스포너 생성
}

void ASDKGoldClashMode::StartGame()
{
	// 디폴트 타이머 시작
	GetWorldTimerManager().SetTimer(TimerHandle_DefaultTimer, this, &ASDKGoldClashMode::DefaultTimer, 1.f, true, 1.f);

	ASDKGoldClashState* SDKGoldClashState = Cast<ASDKGoldClashState>(GameState);
	if (IsValid(SDKGoldClashState))
	{
		const ULevel* Level = GetWorld()->GetLevel(0);
		if (IsValid(Level) && Level->GetLevelScriptActor())
		{
			FOutputDeviceNull ar;
			Level->GetLevelScriptActor()->CallFunctionByNameWithArguments(TEXT("OpenDoors"), ar, NULL, true);
		}
	}

	ChangeGoldRegenRegion();

	Super::StartGame();
}

void ASDKGoldClashMode::GiveUpGame(const int32 TeamNumber)
{
	UE_LOG(LogArena, Log, TEXT("ASDKGoldClashMode::GiveUpGame"));

	TArray<int32> TeamIDs;
	MapTeamIndex.GenerateKeyArray(TeamIDs);

	// 상대팀이 다 나간 상황에서 포기 선언한 경우
	if (TeamIDs.Num() <= 1 || MapExitPlayerRankInfoData.Num() > 0)
	{
		for (auto& Iter : MapExitPlayerRankInfoData)
		{
			if (!TeamIDs.Contains(Iter.Value.TeamNumber))
			{
				TeamIDs.Add(Iter.Value.TeamNumber);
				break;
			}
		}
	}

	if (TeamIDs.Num() > 1)
	{
		TeamIDs.Remove(TeamNumber);
	}

	if (TeamIDs.Num() > 0)
	{
		SetRankerTeamNumber(TeamIDs[0]);
	}

	Super::GiveUpGame(TeamNumber);
}

void ASDKGoldClashMode::FinishGame()
{
	UE_LOG(LogArena, Log, TEXT("ASDKGoldClashMode::FinishGame"));

	GetWorldTimerManager().ClearTimer(TimerHandle_DefaultTimer);
	GetWorldTimerManager().ClearTimer(RegionDisappearTimerHandle);
	GetWorldTimerManager().ClearTimer(WarningRegionDisappearTimerHandle);

	// 게임 결과 정보 및 유저의 랭킹,경기 정보 저장
	SetFinishGameResultRankingData();

	// 각 플레이어의 통계데이터(FMatchStatisticsInfo) 를 모음 및 정렬

	// 통계 및 MVP 표시를 위한 정보 계산
	
	// 경기 결과 로비로 전송
	if (GoldClashType == EGameMode::GoldClash)
	{
		SendFinishGoldClashInfo();
	}

	bool bInInUser = false;
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		// 유저들 경기 결과 씬으로 이동
		const auto SDKMultiController = Cast<ASDKMultiGameController>(It->Get());
		if (IsValid(SDKMultiController))
		{
			bInInUser = true;
			SDKMultiController->ClientEnterToGoldClashResultLevel(ResultInfo, ResultMvpLogs);
		}
	}

	// 유저가 없이 종료된 게임이면 바로 데디 서버 종료

	Super::FinishGame();
}

void ASDKGoldClashMode::LeaveGame(const FString& TagID, int32 SpawnNum, int32 TeamNum, const FString& NickName)
{
	Super::LeaveGame(TagID, SpawnNum, TeamNum, NickName);

	if (IsRunDedicatedServer())
	{
		if (MapTeamIndex.Contains(TeamNum))
		{
			ASDKGoldClashState* SDKGoldClashState = Cast<ASDKGoldClashState>(GameState);
			if (IsValid(SDKGoldClashState))
			{
				SDKGoldClashState->NotifyDisconnectUser(TeamNum, NickName);
			}

			// 팀에 속해 있는 유저 카운트 감소
			MapTeamIndex[TeamNum]--;

			// 유저가 모두 나간 팀이면 데이터 삭제 (AI인원 계산)
			int32 CheckSpawnAICount = MapAITeamIndex.Contains(TeamNum) ? MapAITeamIndex[TeamNum] : 0;
			if (MapTeamIndex[TeamNum] <= CheckSpawnAICount)
			{
				MapTeamIndex.Remove(TeamNum);
				MapTeamIndex.Compact();
				MapTeamIndex.Shrink();

				// 상대팀이 모두 AI였다면 게임 종료를 위해 상대팀도 계산
				int32 EnemyTeamNum = (TeamNum == 1) ? 2 : 1;
				if (MapTeamIndex.Contains(EnemyTeamNum))
				{
					CheckSpawnAICount = MapAITeamIndex.Contains(EnemyTeamNum) ? MapAITeamIndex[EnemyTeamNum] : 0;
					if (MapTeamIndex[EnemyTeamNum] <= CheckSpawnAICount)
					{
						MapTeamIndex.Remove(EnemyTeamNum);
						MapTeamIndex.Compact();
						MapTeamIndex.Shrink();
					}
				}
			}
		}

		// 모든 유저가 잘 빠져나갔다면 게임 종료
		if (MapTeamIndex.Num() == 0)
		{
			ASDKGameState* SDKGameState = Cast<ASDKGameState>(GameState);
			if (IsValid(SDKGameState))
			{
				if (SDKGameState->IsGameReady() || SDKGameState->IsGameActive())
				{
					FinishGame();
				}

				// 데디 서버 종료
			}
		}
	}
}

void ASDKGoldClashMode::DefaultTimer()
{
	RunningTime -= 1;

	// 재접속 하지 않은 유저들을 대상으로 계산
	for (auto Iter = MapExitPlayerRankInfoData.CreateIterator(); Iter; ++Iter)
	{
		if(MapExitPlayerRankInfoData[Iter->Key].LimitTime > 0.f)
		{
			MapExitPlayerRankInfoData[Iter->Key].LimitTime -= 1.f;
			
			if (MapExitPlayerRankInfoData[Iter->Key].LimitTime <= 0.f)
			{
#if !UE_BUILD_SHIPPING
				UE_LOG(LogGoldClash, Log, TEXT("Reconnect Penalty"));
#endif
				// 탈주 시점 랭킹 포인트 계산
				SendMultiGamePenaltyUserInfo(MapExitPlayerRankInfoData[Iter->Key], MapExitPlayerRankPointData[Iter->Key]);
			}	
		}
	}
}

float ASDKGoldClashMode::GetPassiveGoldIntervalTime() const
{
	return PassiveGoldIntervalTime;	
}

void ASDKGoldClashMode::SetRankerTeamNumber(int32 InTeamNum)
{
	RankerTeamNumber = InTeamNum;
}

void ASDKGoldClashMode::AddPlayingController(const int32 SpawnNum, AController* NewController)
{
	if (IsValid(NewController))
	{
		if (!MapPlayingController.Contains(SpawnNum))
		{
			MapPlayingController.Add(SpawnNum, NewController);
		}
		else
		{
			MapPlayingController[SpawnNum] = NewController;
		}
	}
}

void ASDKGoldClashMode::RemovePlayingController(const int32 SpawnNum)
{
	if (MapPlayingController.Contains(SpawnNum))
	{
		MapPlayingController.Remove(SpawnNum);
		MapPlayingController.Compact();
		MapPlayingController.Shrink();
	}
}

void ASDKGoldClashMode::SetFinishGameResultRankingData()
{
	USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (!IsValid(SDKGameInstance))
	{
		return;
	}
	ASDKGoldClashState* SDKGoldClashState = Cast<ASDKGoldClashState>(GameState);
	if (!IsValid(SDKGoldClashState))
	{
		return;
	}

	// 경기 종료 관련 정보
	int32 WinnerID = 0;
	int32 WinnerGold = 0;
	float WinnerGainGoldTime = 0.f;
	
	int32 WinnerTeamID = RankerTeamNumber; 
	
	const bool bCustomGame = SDKGoldClashState->IsCustomMatch();

	// 유저별 골드 순위 및 승리팀 판별
	TArray<FResultGoldData> ListPlayerGold;

	if(SDKGoldClashState->PlayerArray.Num())
	{
		for (APlayerState* PlayerState : SDKGoldClashState->PlayerArray)
		{
			ASDKPlayerState* SDKPlayerState = Cast<ASDKPlayerState>(PlayerState);
			if (!IsValid(SDKPlayerState))
			{
				continue;
			}

			const int32 UserID = SDKPlayerState->GetSpawnNumber();
			const int32 UserGold = SDKPlayerState->GetGold();
			const float UserGainGoldTime = SDKPlayerState->GetGainGoldTime();
			ListPlayerGold.Add(FResultGoldData(UserID, UserGold, UserGainGoldTime));

			const int32 UserTeamNumber = SDKPlayerState->GetPlayerGameInfo().TeamNumber;

			// 승리팀 및 1등 유저 판별
			
			// 한쪽 팀이 경기를 포기해서 승리팀이 확정된 경우
			if (RankerTeamNumber != 0 && RankerTeamNumber != UserTeamNumber)
			{
				continue;
			}			
			if (WinnerGold > UserGold)
			{
				continue;
			}
			if(WinnerGold == UserGold && WinnerGainGoldTime < UserGainGoldTime)
			{
				continue;
			}

			// 골드가 가장 많거나, 동점 골드를 먼저 획득한 사람 (포기가 나온 게임일 경우엔 승리팀만)
			WinnerID = UserID;
			WinnerTeamID = UserTeamNumber;
			WinnerGold = UserGold;
			WinnerGainGoldTime = UserGainGoldTime;
		}
	}

	//1등 유저로 승리팀 판별
	RankerTeamNumber = WinnerTeamID;

	// 골드 순위별 정렬
	ListPlayerGold.Sort([](auto GoldDataA, auto GoldDataB)
	{
		if (GoldDataA.Gold == GoldDataB.Gold)
		{
			return GoldDataA.GainGoldTime < GoldDataB.GainGoldTime;
		}
		return GoldDataA.Gold > GoldDataB.Gold;
	});

	ResultInfo = FGoldClashResultUIInfo();
	
	int32 FinishRank = 1;
	TArray<FGoldClashResultInfo>& ResultRankingList = ResultInfo.ResultInfoList;
	for (const FResultGoldData& IterData : ListPlayerGold)
	{
		// 유저 정보 설정

		// 랭킹 관련 로직 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/** CalcRankPoint Start*/
		int32 RankPoint = 0;

		if(SDKPlayerState->GetPlayerId() > 0)
		{
			// 실력 지수
			float MySkillScore = FMath::Pow(10.f, ((SDKPlayerState->GetRankPoint() - 30.f) / 600.f));
			const int32 EnemyTeamNumber = (GameInfo.TeamNumber == 1) ? 2 : 1;
			float EnemyAvgSkillScore = FMath::Pow(10.f, ((GetTeamAvgRankingPoint(EnemyTeamNumber) - 30.f) / 600.f));
			
			float SkillScore = MySkillScore / EnemyAvgSkillScore;
			
			// 예상 승률
			float WinnerRate = round(SkillScore / (SkillScore + 1) * 100.f) / 100.f;
			
			// ELO Rating
			float WinnerPoint = GameInfo.TeamNumber == RankerTeamNumber ? 1.f : 0.f; // 무승부 처리필요
			float ELORating = round(SDKPlayerState->GetRankPoint() + 30.f * (WinnerPoint - WinnerRate)) - SDKPlayerState->GetRankPoint();
			
			// 연승점수
			float WinningStreakScore = FMath::Min(5.f, FMath::Max(0.f, static_cast<float>(SDKPlayerState->GetWinningStreak()) - 1.f));
			
			// 킬포인트
			float KillDeathPoint = SDKPlayerState->GetNumOfKill() - SDKPlayerState->GetNumOfDeath();
			bool IsActiveKillPoint = KillDeathPoint > -20.f;
			float KillDeathLerpPoint = KillDeathPoint + 20.f;
			float KillDeathScore = 0.f;
			if (IsActiveKillPoint)
			{
				KillDeathScore = FMath::Max(0.f, (KillDeathPoint / KillDeathLerpPoint) * 10.f);
			}
			
			RankPoint = FMath::RoundToInt(ELORating + WinningStreakScore + KillDeathScore);
			if (GameInfo.TeamNumber == RankerTeamNumber)
			{
				RankPoint = FMath::Max(1, RankPoint);
			}
			else
			{
				RankPoint = FMath::Min(-1, RankPoint);
			}
			
		}
		/** CalcRankPoint End*/

		// 비스킬 로그 및 결과 정보 저장

		++FinishRank;
	}

	// 재접속 대기중인 유저 랭킹 정보 처리
	SetFinishExitPlayerRankingData(FinishRank, ResultRankingList);

#if !UE_BUILD_SHIPPING
	UE_LOG(LogGoldClash, Log, TEXT("ResultRankingList Count : %d"), ResultRankingList.Num());
#endif

	// 게임에서 나간 유저의 정보도 결과정보에 포함

	// 경기정보

	// RankInfo 와 매치되는 유저의 MatchInfo 저장 
}

void ASDKGoldClashMode::SetFinishExitPlayerRankingData(int32& FinishRank, TArray<FGoldClashResultInfo>& InResultInfo)
{
	if (MapExitPlayerRankInfoData.Num() <= 0 && MapExitPlayerRankPointData.Num() <= 0)
	{
		return;
	}

	USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (!IsValid(SDKGameInstance))
	{
		return;
	}

	ASDKGoldClashState* SDKGoldClashState = GetGameState<ASDKGoldClashState>();
	if (!IsValid(SDKGoldClashState))
	{
		return;
	}

	for (auto& Iter : MapExitPlayerRankInfoData)
	{
		bool bIsSame = false;
		for (auto ItInfo : InResultInfo)
		{
			if (Iter.Value.UserID == ItInfo.UserID)
			{
				bIsSame = true;
				break;
			}
		}

		if (bIsSame)
		{
			continue;
		}

		if (MapExitPlayerRankPointData.Contains(Iter.Key))
		{
			FRankPointData& RankData = MapExitPlayerRankPointData[Iter.Key];
			
			float SkillScore = 0;
			float MySkillScore = FMath::Pow(10.f, ((RankData.CurrentRankPoint - 30.f) / 600.f));
			
			const int32 EnemyTeamNumber = (RankData.TeamNumber == 1) ? 2 : 1;
			const float EnemyAvgPoint = GetTeamAvgRankingPoint(EnemyTeamNumber);
			float EnemyAvgSkillScore = FMath::Pow(10.f, ((EnemyAvgPoint - 30.f) / 600.f));

			//실력 지수
			SkillScore = MySkillScore / EnemyAvgSkillScore;
			
			// 예상 승률
			float WinnerRate = round(SkillScore / (SkillScore + 1) * 100.f) / 100.f;
			
			// ELO Rating
			float WinnerPoint = Iter.Value.TeamNumber == RankerTeamNumber ? 1.f : 0.f;
			float ELORating = round(RankData.CurrentRankPoint + 30.f * (WinnerPoint - WinnerRate)) - RankData.CurrentRankPoint;
			
			// 연승점수
			float WinningStreakScore = FMath::Min(5.f, FMath::Max(0.f, static_cast<float>(RankData.WinningStreak) - 1.f));
			
			// 킬포인트
			float KillDeathPoint = RankData.KillCount - RankData.DeathCount;
			bool IsActiveKillPoint = KillDeathPoint > -20.f;
			float KillDeathLerpPoint = KillDeathPoint + 20.f;
			float KillDeathScore = 0.f;
			if (IsActiveKillPoint)
			{
				KillDeathScore = FMath::Max(0.f, (KillDeathPoint / KillDeathLerpPoint) * 10.f);
			}
			
			int32 Result = FMath::RoundToInt(ELORating + WinningStreakScore + KillDeathScore);
			if (Iter.Value.TeamNumber == RankerTeamNumber)
			{
				Result = FMath::Max(1, Result);
			}
			else
			{
				Result = FMath::Min(-1, Result);
			}
			
			Iter.Value.RankPoint = Result;
			InResultInfo.Add(Iter.Value);

			// BiskitLog

			++FinishRank;
		}
	}
}

void ASDKGoldClashMode::AddMapExitPlayerRankInfoData(const FGoldClashResultInfo& InGoldClashResultInfo)
{
	if (!MapExitPlayerRankInfoData.Contains(InGoldClashResultInfo.UserID))
	{
		MapExitPlayerRankInfoData.Emplace(InGoldClashResultInfo.UserID, InGoldClashResultInfo);
	}
}

void ASDKGoldClashMode::DeleteMapExitPlayerRankInfoData(const int32 InUserID)
{
	if (MapExitPlayerRankInfoData.Num() > 0)
	{
		if (MapExitPlayerRankInfoData.Contains(InUserID))
		{
			MapExitPlayerRankInfoData.Remove(InUserID);

			if (MapExitPlayerRankInfoData.Num() > 0)
			{
				MapExitPlayerRankInfoData.Compact();
				MapExitPlayerRankInfoData.Shrink();
			}
		}
	}
}

void ASDKGoldClashMode::AddMapExitPlayerRankPointData(const int32 InUserID,const FRankPointData& InGoldClashRankInfo)
{
	if (!MapExitPlayerRankPointData.Contains(InUserID))
	{
		MapExitPlayerRankPointData.Emplace(InUserID, InGoldClashRankInfo);
	}
}

void ASDKGoldClashMode::DeleteMapExitPlayerRankPointData(const int32 InUserID)
{
	if (MapExitPlayerRankPointData.Num() > 0)
	{
		if (MapExitPlayerRankPointData.Contains(InUserID))
		{
			MapExitPlayerRankPointData.Remove(InUserID);

			if (MapExitPlayerRankPointData.Num() > 0)
			{
				MapExitPlayerRankPointData.Compact();
				MapExitPlayerRankPointData.Shrink();
			}
		}
	}
}

float ASDKGoldClashMode::GetTeamAvgRankingPoint(const int32 TeamNum)
{
	const int32 TeamMemberCount = MapInitTeamCount.Contains(TeamNum) ? MapInitTeamCount[TeamNum] : 0.f;
	if (TeamMemberCount > 0)
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogGoldClash, Log, TEXT("Enermy TeamMember Count : %d"), TeamMemberCount);
#endif
		const int32 TeamTotalPoint = MapTotalTeamRankPoint.Contains(TeamNum) == true ? MapTotalTeamRankPoint[TeamNum] : 0.f;
		return (static_cast<float>(TeamTotalPoint) / static_cast<float>(TeamMemberCount));
	}
	else
	{
		UE_LOG(LogArena, Error, TEXT("Error : TeamIndexCount Zero!!"));
	}

	return 0.f;
}

int32 ASDKGoldClashMode::GetWinnerTeamNumberByTimeOver()
{
	TMap<int32, int32> MapTeamTotalGold;

	ASDKGoldClashState* GoldClashState = GetGameState<ASDKGoldClashState>();
	if (IsValid(GoldClashState))
	{
		if (GoldClashState->GetMapPlayerRankInfo().Num() > 0)
		{
			for (auto& RankInfo : GoldClashState->GetMapPlayerRankInfo())
			{
				if (!MapTeamTotalGold.Contains(RankInfo.Value.TeamNumber))
				{
					MapTeamTotalGold.Add(RankInfo.Value.TeamNumber, RankInfo.Value.CurrentCoin);
				}
				else
				{
					MapTeamTotalGold[RankInfo.Value.TeamNumber] += RankInfo.Value.CurrentCoin;
				}
			}
		}
	}

	if (MapExitPlayerRankInfoData.Num() > 0)
	{
		for (auto IterExitPlayer : MapExitPlayerRankInfoData)
		{
			if (MapTeamTotalGold.Contains(IterExitPlayer.Value.TeamNumber))
			{
				MapTeamTotalGold[IterExitPlayer.Value.TeamNumber] += IterExitPlayer.Value.CurrentCoin;
			}
			else
			{
				MapTeamTotalGold.Add(IterExitPlayer.Value.TeamNumber, IterExitPlayer.Value.CurrentCoin);
			}
		}
	}

	if (MapTeamTotalGold.Num() >= 2)
	{
		MapTeamTotalGold.ValueSort([](int32 A, int32 B) {return A > B; });
	}

	return MapTeamTotalGold.CreateIterator()->Key;
}

void ASDKGoldClashMode::SendMultiGamePenaltyUserInfo(const FGoldClashResultInfo PlayerData, const FRankPointData InRankData)
{
	ASDKGoldClashState* GoldClashState = GetGameState<ASDKGoldClashState>();
	if (!IsValid(GoldClashState))
	{
		return;
	}

	GoldClashState->RemovePlayerRankInfo(PlayerData.UserID);
	GoldClashState->RemoveTeamMemberData(PlayerData.TeamNumber, PlayerData.UserID);

	// 나간 유저 정보 데이터에서 제거
	DeleteMapExitPlayerRankInfoData(PlayerData.UserID);
	
	FSendGoldClashResult TrollResultData;
	TrollResultData.UID = PlayerData.UserID;
	TrollResultData.HeroID = PlayerData.HeroID;
	TrollResultData.GoldClashMapID = UGameplayStatics::GetCurrentLevelName(GetWorld());
	TrollResultData.bCustomMatch = PlayerData.bCustomMatch;
	TrollResultData.bDodge = true;
	TrollResultData.RankingPoint = CalculatePenaltyRankingPoint(InRankData);

	const auto SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (IsValid(SDKGameInstance))
	{
		// 비스킷 로그 데이터 얻어오기
		if (IsValid(SDKGameInstance->BiskitEventLogManager))
		{
			// 비스킷로그:  캐릭터 정보 (Dodge 여부)
			FBiskitCharacter NewBiskitCharacter;
			NewBiskitCharacter.EventType = EBiskitPlayerEventType::Dodge;
			NewBiskitCharacter.PlayerId = PlayerData.UserID;
			NewBiskitCharacter.bValue = true;
			FSDKGameEventDelegates::OnBiskitCharacterDelegate.Broadcast(NewBiskitCharacter);

			TArray<FBiskitLogData> OutLogArray;
			SDKGameInstance->BiskitEventLogManager->GetPlayerBiskitLogDataList(TrollResultData.UID, OutLogArray);
			TrollResultData.BiskitLog = OutLogArray;
		}

		TrollingTeamNumberList.AddUnique(PlayerData.TeamNumber);
		SDKGameInstance->ServerToServerManager->SendFinishGoldClash({ TrollResultData }, GoldClashState->GetMatchID());
		SDKGameInstance->ServerToServerManager->SetPenaltyMatchingPlayer(PlayerData.UserID);

		UE_LOG(LogGoldClash, Log, TEXT("Send Penalty GoldClash Info"));
	}

	// 나간 유저 랭킹 정보 삭제
	DeleteMapExitPlayerRankPointData(PlayerData.UserID);

	// 현재 게임 참여중인 컨트롤러에서 제거
	RemovePlayingController(PlayerData.UserID);

	// 노티 처리
	GoldClashState->NotifyLeaveUser(PlayerData.TeamNumber, PlayerData.Nickname);

	if (!MapTeamIndex.Contains(PlayerData.TeamNumber))
	{
		GoldClashState->GiveUpGame(PlayerData.TeamNumber);
	}
}

int32 ASDKGoldClashMode::CalculatePenaltyRankingPoint(const FRankPointData& InRankData)
{
	float SkillScore = 0;
	float MySkillScore = FMath::Pow(10.f, ((InRankData.CurrentRankPoint - 30.f) / 600.f));
	
	const int32 EnemyTeamNumber = (InRankData.TeamNumber == 1) ? 2 : 1;
	const float EnemyAvgPoint = GetTeamAvgRankingPoint(EnemyTeamNumber);
	float EnemyAvgSkillScore = FMath::Pow(10.f, ((EnemyAvgPoint - 30.f) / 600.f));
	
	//실력 지수
	SkillScore = MySkillScore / EnemyAvgSkillScore;
	
	// 예상 승률
	float WinnerRate = round(SkillScore / (SkillScore + 1) * 100.f) / 100.f;
	
	// ELO Rating
	float WinnerPoint = 0.f;
	float ELORating = round(InRankData.CurrentRankPoint + 30.f * (WinnerPoint - WinnerRate)) - InRankData.CurrentRankPoint;
	
	// 연승점수
	float WinningStreakScore = FMath::Min(5.f, FMath::Max(0.f, static_cast<float>(InRankData.WinningStreak) - 1.f));
	
	// 킬포인트
	float KillDeathPoint = (FMath::Min(InRankData.KillCount, 5)) - InRankData.DeathCount;
	bool IsActiveKillPoint = KillDeathPoint > -20.f;
	float KillDeathLerpPoint = KillDeathPoint + 20.f;
	float KillDeathScore = 0.f;
	if (IsActiveKillPoint)
	{
		KillDeathScore = FMath::Max(0.f, (KillDeathPoint / KillDeathLerpPoint) * 10.f);
	}
	
	int32 PenaltyPoint = 10;
	FS_GlobalDefine* GDTable = USDKTableManager::Get()->FindTableGlobalDefine(EGlobalDefine::GoldClashDodgeVariablePoint);
	if (GDTable && GDTable->Value.Num() > 0)
	{
		PenaltyPoint = FCString::Atoi(*GDTable->Value[0]);
	}

	int32 Result = FMath::Min(FMath::RoundToInt(ELORating + WinningStreakScore + KillDeathScore) - PenaltyPoint, -PenaltyPoint);
	
	return Result;
}
