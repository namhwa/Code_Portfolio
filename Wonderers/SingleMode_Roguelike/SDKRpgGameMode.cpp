// Fill out your copyright notice in the Description page of Project Settings.

#include "SingleMode_Roguelike/SDKRpgGameMode.h"

#include "EngineUtils.h"

#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LevelScriptActor.h"
#include "Engine/LevelStreamingDynamic.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Misc/OutputDeviceNull.h"

#include "Engine/SDKAssetManager.h"
#include "Engine/SDKGameInstance.h"
#include "Engine/SDKGameTask.h"
#include "Engine/SDKGameDelegate.h"
#include "Arena/Arena.h"

#include "GameMode/SDKGameState.h"
#include "GameMode/SDKRpgState.h"
#include "GameMode/SDKSectorLocation.h"
#include "GameMode/SDKRoguelikeSector.h"
#include "GameMode/SDKRoguelikeManager.h"

#include "Manager/SDKTableManager.h"
#include "Manager/SDKLobbyServerManager.h"

#include "Character/SDKMyInfo.h"
#include "Character/SDKCharacter.h"
#include "Character/SDKPlayerState.h"
#include "Character/SDKPlayerStart.h"
#include "Character/SDKRpgPlayerState.h"
#include "Character/SDKInGameController.h"
#include "Character/SDKPlayerCharacter.h"
#include "Character/SDKNonePlayerCharacter.h"

#include "Item/SDKItem.h"
#include "Object/SDKObjectRewardPortal.h"
#include "Share/SDKData.h"
#include "Share/SDKHelper.h"


DEFINE_LOG_CATEGORY_STATIC(LogSDKRPGGameMode, Log, All);

ASDKRpgGameMode::ASDKRpgGameMode()
	: CurrentRoomType(FName("Start")), RoomIndex(0), CurColumn(3), PrevColumn(3), DifficultyLevel(0)
{
	PlayerStateClass = ASDKRpgPlayerState::StaticClass();
	bHaveObjectSubLevel = false;
}

void ASDKRpgGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (!IsValid(SDKGameInstance))
	{
		return;
	}

	CRPGModeData& RpgModeData = SDKGameInstance->MyInfoManager->GetRPGModeData();
	if (RpgModeData.IsEnterCamp()) 
	{
		return;
	}

	// 챕터 로직 (기존 로직)
	CurrentContentsType = RpgModeData.GetContentModeType();
	CurrentChapterID = RpgModeData.GetChapterID();
	CurrentStageID = RpgModeData.GetStageID();
	SpecialLevelIndex = RpgModeData.GetEnableSpecialLevelIndex();

	// 로드 확인을 반복할 시간 텀
	CheckSectorIntervalTime = 0.5f;
	CheckSpawnIntervalTime = 0.5f;

	// SA_RpgRoom관련 Delegate 연결
	USDKGameDelegate::Get()->OnRpgRoomStartAckEvent.AddDynamic(this, &ASDKRpgGameMode::OnReceiveRoomStart);
	USDKGameDelegate::Get()->OnRpgRoomEndAckEvent.AddDynamic(this, &ASDKRpgGameMode::OnReceiveRoomEnd);

	auto StageTable = USDKTableManager::Get()->FindTableRow<FS_Stage>(ETableType::tb_Stage, FString::FromInt(CurrentStageID));
	if (StageTable != nullptr)
	{
		Rooms.Empty();

		InitMonster();
		InitRoom();
		InitShuffleRoom();
	}
}

void ASDKRpgGameMode::InitGameState()
{
	Super::InitGameState();

	ASDKRpgState* SDKRpgState = Cast<ASDKRpgState>(GameState);
	if (!IsValid(SDKRpgState)) 
	{
		return;
	}

	USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (!IsValid(SDKGameInstance))
	{
		return;
	}

	SetClearRoom(false);

	CRPGModeData& RpgModeData = USDKMyInfo::Get().GetRPGModeData();
	SDKRpgState->SetRetryGameMode(SDKGameInstance->GetRetryRpgMode());
	SDKRpgState->SetGameMode(EGameMode::Rpg);
	SDKRpgState->SetChapterID(RpgModeData.GetChapterIDByString());
	SDKRpgState->SetStageID(RpgModeData.GetStageIDByString());
}

void ASDKRpgGameMode::PostLogin(APlayerController* NewPlayer)
{
	if (IsValid(NewPlayer) && IsValid(NewPlayer->PlayerCameraManager))
	{
		NewPlayer->PlayerCameraManager->SetManualCameraFade(1.f, FColor::Black, true);
	}

	Super::PostLogin(NewPlayer);
}

void ASDKRpgGameMode::RestartPlayer(AController* NewPlayer)
{
	if (IsValid(NewPlayer->GetPawn()) == true)
	{
		ASDKPlayerCharacter* DefaultCharacter = NewPlayer->GetPawn<ASDKPlayerCharacter>();
		if (IsValid(DefaultCharacter) == true && DefaultCharacter->GetTableID() <= 0)
		{
			DefaultCharacter->SetMovementMode(EMovementMode::MOVE_Flying);
		}
	}
}

void ASDKRpgGameMode::BeginPlay()
{
	Super::BeginPlay();

	IsSetPlayerData = false;

#if WITH_EDITOR
	InitMap();
#else
	// 선 로딩 목록 로딩
	// 로딩된 목록이 있는 경우, AssetsLoadComplete 함수 호출
#endif
}

void ASDKRpgGameMode::AssetsLoadComplete()
{
	UE_LOG(LogSDKRPGGameMode, Log, TEXT("Complete to pre-load"));
	
	InitMap();
}

void ASDKRpgGameMode::InitMap()
{
	USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (!IsValid(SDKGameInstance))
	{
		return;
	}

	// 로그라이크 매니저 생성
	RoguelikeManager = GetWorld()->SpawnActor<ASDKRoguelikeManager>(ASDKRoguelikeManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);

	CRPGModeData& RpgModeData = SDKGameInstance->MyInfoManager->GetRPGModeData();
	if (CurrentChapterID > 0 && CurrentStageID > 0)
	{
		if (RpgModeData.IsEnterCamp() == false)
		{
			// 방 생성
			CreateRoom();
		}
	}
	else
	{
		if (RpgModeData.IsEnterCamp())
		{
			UE_LOG(LogGame, Log, TEXT("Enter Camp : Server Ready Complete"));

			ServerReadyComplete();
		}

#if WITH_EDITOR
		ServerReadyComplete();
#endif
	}

	ResetStoreRefreshCost();
}

void ASDKRpgGameMode::CompletedLoadCreateLevel()
{
	const ASDKInGameController* InGameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(InGameController) == true && IsValid(InGameController->PlayerCameraManager) == true)
	{
		InGameController->PlayerCameraManager->SetManualCameraFade(1.0f, FColor::Black, false);
	}
}

void ASDKRpgGameMode::CompletedStreamLevel()
{
	UE_LOG(LogGame, Log, TEXT("Stream Level Complete Start"));

	SetLoadedRoomSetting();
}

void ASDKRpgGameMode::CompletedObjectStreamLevel()
{
	UE_LOG(LogGame, Log, TEXT("Object Stream Level Complete Start"));
}

bool ASDKRpgGameMode::IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const
{
	if (Player)
	{
		ASDKPlayerStart* TeamStart = Cast<ASDKPlayerStart>(SpawnPoint);
		ASDKRpgPlayerState* PlayerState = Cast<ASDKRpgPlayerState>(Player->PlayerState);

		if (IsValid(PlayerState) && IsValid(TeamStart) && TeamStart->SpawnTeam != PlayerState->GetTeamNumber())
		{
			return false;
		}
	}

	return Super::IsSpawnpointAllowed(SpawnPoint, Player);
}

void ASDKRpgGameMode::ServerReadyComplete()
{
	ASDKGameMode::ServerReadyComplete();

	// 유저 정보 요청 및 생성
	for (auto It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		ASDKPlayerController* SDKPlayerController = Cast<ASDKPlayerController>(It->Get());
		if (IsValid(SDKPlayerController) == true)
		{
			SDKPlayerController->ClientRequestData();
		}
	}

	// 게임 시작
	StartGame();

	// 미션
	ASDKGameState* SDKGameState = Cast<ASDKGameState>(GameState);
	if (IsValid(SDKGameState) == true)
	{
		SDKGameState->InitMissionManagerData();
	}
}

void ASDKRpgGameMode::SetPlayerData(const FServerSendData& SendData)
{
	Super::SetPlayerData(SendData);

	UE_LOG(LogGame, Log, TEXT("RpgMode : SetPlayerData"));

	IsSetPlayerData = true;
	PlayerData = SendData;

	ASDKInGameController* SDKIngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKIngameController))
	{
		ASDKPlayerState* SDKPlayerState = Cast<ASDKPlayerState>(SDKIngameController->PlayerState);
		if (IsValid(SDKPlayerState))
		{
			TArray<FPlayerArtifactData> ArtifactDataList;
			for(const auto& Iter : SendData.ArtifactListData)
			{
				ArtifactDataList.Add(Iter);
			}

#if WITH_EDITOR
			// 에디터? 테스트용 코드
			if (ArtifactDataList.Num() <= 0)
			{
				TArray<FName> ArtifactName = USDKTableManager::Get()->GetRowNames(ETableType::tb_Artifact);

				for (auto Iter : ArtifactName)
				{
					FS_Artifact* ArtifactTable = USDKTableManager::Get()->FindTableRow<FS_Artifact>(ETableType::tb_Artifact, Iter.ToString());

					if (ArtifactTable != nullptr)
					{
						if (ArtifactTable->Default)
						{
							FArtifactData ArtifactData;
							ArtifactData.ArtifactID = FSDKHelpers::StringToInt32(Iter.ToString());
							ArtifactData.Enhance = 0;
							ArtifactDataList.Add(ArtifactData);
						}
					}
				}
			}
#endif
			SDKPlayerState->SetPlayerArtifactList(ArtifactDataList);
		}
	}

	ASDKRpgState* SDKRpgState = GetGameState<ASDKRpgState>();
	if (IsValid(SDKRpgState))
	{
		SDKRpgState->SetDropReadyArtifactList();
		SDKRpgState->SetDropReadyDarkArtifactList();
	}


	TaskAfterSeconds(this, [this]()
	{
		SpawnPlayer();
	}, CheckSpawnIntervalTime);
}

void ASDKRpgGameMode::ReadyGame()
{
	Super::ReadyGame();
}

void ASDKRpgGameMode::StartGame()
{
	Super::StartGame();

	ASDKRpgState* SDKRpgState = GetGameState<ASDKRpgState>();
	if (IsValid(SDKRpgState) == true)
	{
		CheckSetStartGameGold();
	}

	if (FSDKHelpers::IsInGlitchCamp(GetWorld()))
	{
		ASDKInGameController* SDKInGameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		if (IsValid(SDKInGameController))
		{
			SDKInGameController->OpenRpgCampTitle();
			SDKInGameController->ClientSetBGMSound(TEXT("130005"));
		}
	}
}

void ASDKRpgGameMode::GiveUpGame(const int32 TeamNumber)
{
	SetGiveupGame(true);
	SetClearRoom(false);

	ClearCurrentRoom();
}

void ASDKRpgGameMode::FinishGame()
{
	Super::FinishGame();

	if (IsValid(GetRoguelikeManager()))
	{
		GetRoguelikeManager()->ClearRoguelikeSectorList();
	}

	if (FSDKHelpers::IsInGlitchCamp(GetWorld()))
	{
		return;
	}

	USDKGameInstance* SDKGameInstance = Cast<USDKGameInstance>(GetGameInstance());
	if (IsValid(SDKGameInstance))
	{
		int32 DieObjectID = 0, DieMonsterID = 0, PlayingTime = 0;
		ASDKRpgState* SDKRpgState = Cast<ASDKRpgState>(GameState);
		if (IsValid(SDKRpgState))
		{
			const FString ChapterID = FString::FromInt(CurrentChapterID);
			SDKRpgState->AddClearStateInfo(*ChapterID, RoomIndex);

			const ASDKInGameController* SDKInGameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
			if (IsValid(SDKInGameController))
			{
				ASDKRpgPlayerState* SDKRpgPlayerState = Cast<ASDKRpgPlayerState>(SDKInGameController->PlayerState);
				if (IsValid(SDKRpgPlayerState))
				{
					SDKRpgPlayerState->SetCharacterDie(SDKRpgState->GetIsDieEnd());

					if (SDKRpgState->GetIsDieEnd())
					{
						if (SDKRpgPlayerState->GetDieReason() == 1)
						{
							DieMonsterID = SDKRpgPlayerState->GetDieCauserID();
						}
						else
						{
							DieObjectID = SDKRpgPlayerState->GetDieCauserID();
						}
					}
				}
			}

			PlayingTime = SDKRpgState->GetGamePlayingTime();
		}
		
		SDKGameInstance->LobbyServerManager->CQ_RpgEnd(GetClearChapter(), GetGiveupGame(), DieObjectID, DieMonsterID, PlayingTime);
	}
}

void ASDKRpgGameMode::OnReceiveRoomStart()
{
	if (CurrentRoomType == FName("Start") && RoomIndex <= 0)
	{
		ServerReadyComplete();
		LevelLoadInSubLevel();
	}
	else
	{
		// 현재 방 정보
		FRoomData& RoomData = Rooms[RoomIndex];

		// 다음 방 생성
		if (RoomData.NextRoomTypes.Num() >= 0)
		{
			// 현재 방 저장 : StageMapID
			AddPassedStageMapID(RoomData.StageMapID);
			GetRoomCountNextSelectable(RoomIndex, PrevColumn);

			ASDKRpgState* SDKRpgState = GetGameState<ASDKRpgState>();
			if (IsValid(SDKRpgState))
			{
				// 선택된 방 정보 저장
				SDKRpgState->AddSelectedRoomInfo(RoomIndex, CurColumn);
				SDKRpgState->SetCurrentFloor(RoomData.Row < 1 ? 1 : RoomData.Row);
				SDKRpgState->SetCurrentMapType(RoomData.RoomType);
				SDKRpgState->SetCurrentLevelID(RoomData.LevelID);

				// 이벤트 정보 초기화
				SDKRpgState->SetClearCurrentSectorEvent(false);
			}
		}

		// 다음방 생성
		CreateRoomStreamLevel(CurrColumnIndex);

		// 방이 생성될 때마다 난이도 상승
		IncreaseDifficultyLevel();

#if WITH_EDITOR
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, *FString::Printf(TEXT("InColumn : %d"), CurrColumnIndex));
		UE_LOG(LogTemp, Log, TEXT("InColumn : %d"), CurrColumnIndex);
#endif
	}
}

void ASDKRpgGameMode::OnReceiveRoomEnd()
{
	if (IsValid(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
	{
		ASDKRpgPlayerState* RpgPlayerState = (UGameplayStatics::GetPlayerController(GetWorld(), 0))->GetPlayerState<ASDKRpgPlayerState>();
		if (IsValid(RpgPlayerState))
		{
			RpgPlayerState->ClearPlayingRoomData();
		}
	}

	ASDKRpgState* SDKRpgState = GetWorld()->GetGameState<ASDKRpgState>();

	if (SDKRpgState->GetGamePlayState() == EGameplayState::Finished)
	{
		UE_LOG(LogGame, Log, TEXT("On Received RoomEnd : Already Send CQ_RpgEnd"));

		FinishGame();
	}
	else if (CurrentRoomType == FName("Boss") && !GetClearChapter())
	{
		SDKRpgState->CheckRpgGame(GetClearRoom());
	}
	else if (bGiveupGame)
	{
		UE_LOG(LogGame, Log, TEXT("On Received RoomEnd : Give up"));

		// 시퀀스 재생중인 경우 멈추기
		USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
		if (SDKGameInstance->GetPlayingLevelSequence() == true)
		{
			AActor* FindActor = UGameplayStatics::GetActorOfClass(GetWorld(), ALevelSequenceActor::StaticClass());
			if (IsValid(FindActor) == true)
			{
				ALevelSequenceActor* SequenceActor = Cast<ALevelSequenceActor>(FindActor);
				if (IsValid(SequenceActor) == true && IsValid(SequenceActor->GetSequencePlayer()) == true)
				{
					SequenceActor->GetSequencePlayer()->Pause();
					SDKGameInstance->SetPlayingLevelSequence(false);
				}
			}
		}

		if (IsValid(SDKRpgState))
		{
			SDKRpgState->CheckRpgGame(!bGiveupGame);
		}
	}
	else if (bCharacterDied)
	{
		UE_LOG(LogGame, Log, TEXT("On Received RoomEnd : Character Died"));

		SDKRpgState->CheckRpgGame(!bCharacterDied);
	}
	else
	{
		UE_LOG(LogGame, Log, TEXT("On Received RoomEnd : Next Level Load"));

		// 저장 변수에서 레벨 제거
		RemoveSubLevelInLevel(CurSubLevel);
		RemoveObejctLevel(CurSubLevel);

		// 실제 레벨 및 오브젝트 제거
		RemovePreviousLevelActors();
		RemoveLoadedAllLevels();

		CurSubLevel = nullptr;

		LoadNextRoom();
	}

	// 방 클리어 변수 초기화
	ResetClearRoom();
}

ASDKRoguelikeSector* ASDKRpgGameMode::GetRoguelikeSector() const
{
	return IsValid(RoguelikeManager) == true ? RoguelikeManager->GetCurrentSector() : nullptr;
}

TArray<FString> ASDKRpgGameMode::GetLoadedLevelIDs()
{
	if (mapLoadedLevelID.Num() > 0)
	{
		TArray<FString> LevelIDs;
		mapLoadedLevelID.GenerateKeyArray(LevelIDs);

		return LevelIDs;
	}

	return TArray<FString>();
}

TArray<FRoomData> ASDKRpgGameMode::GetRoomDataList()
{
	if (Rooms.Num() > 0)
	{
		return Rooms;
	}

	return TArray<FRoomData>();
}

FRoomData ASDKRpgGameMode::GetCurrentRoomData() const
{
	if (Rooms.Num() > 0 && Rooms.IsValidIndex(RoomIndex) == true)
	{
		return Rooms[RoomIndex];
	}

	return FRoomData();
}

void ASDKRpgGameMode::SetClearRoom(bool InClear)
{
	if (!bClearRoom)
	{
		bClearRoom = InClear;
	}

	if (bClearRoom && CurrentRoomType == FName("Boss"))
	{
		UE_LOG(LogGame, Log, TEXT("RPG Stage : Boss Room Clear %s"), InClear ? TEXT("True") : TEXT("False"));

		// 보스 사망 시 남아있는 잡몹 모두 제거
		for (TActorIterator<APawn> Iterator(GetWorld()); Iterator; ++Iterator)
		{
			ASDKCharacter* const SDKCharacter = Cast<ASDKCharacter>(*Iterator);
			if (IsValid(SDKCharacter) && (SDKCharacter->GetPawnData()->NpcType == ENpcType::Normal || SDKCharacter->GetPawnData()->NpcType == ENpcType::Elite))
			{
				SDKCharacter->BeginDie();
			}
		}
	}
}
	
void ASDKRpgGameMode::SetRoomType(FName NewType) 
{ 
	CurrentRoomType = NewType; 

	auto InGameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(InGameController))
	{
		if (CurrentRoomType == TEXT("Puzzle"))
		{
			InGameController->ClientSetBGMSound(TEXT("130001"));
		}
		else if (CurrentRoomType == TEXT("Boss"))
		{
			InGameController->ClientSetBGMSound(TEXT("130004"));
		}
		else if ((CurrentChapterID <= 0 && CurrentRoomType == TEXT("Start")) || CurrentRoomType == TEXT("Store"))
		{
			InGameController->ClientSetBGMSound(TEXT("130005"));
		}
		else
		{
			if (CurrentChapterID == 1)
			{
				InGameController->ClientSetBGMSound(TEXT("130011"));
			}
			else if (CurrentChapterID == 2)
			{
				InGameController->ClientSetBGMSound(TEXT("130010"));
			}
			else if (CurrentChapterID == 3)
			{
				InGameController->ClientSetBGMSound(TEXT("130007"));
			}
			else if (CurrentChapterID == 4)
			{
				InGameController->ClientSetBGMSound(TEXT("130020"));
			}
			else if (CurrentChapterID == 5)
			{
				InGameController->ClientSetBGMSound(TEXT("130017"));
			}
			else if (CurrentChapterID == GLITCH_TUTORIAL_ID)
			{
				InGameController->ClientSetBGMSound(TEXT("130011"));
			}
		}
	}
}

void ASDKRpgGameMode::SetCurrentSector(ASDKRoguelikeSector* pSector, FString SectorID)
{
	if(IsValid(RoguelikeManager) == true)
	{
		if(SectorID.IsEmpty() == false)
		{
			RoguelikeManager->SetCurrentSector(pSector);

			ASDKRpgState* SDKRpgState = Cast<ASDKRpgState>(GameState);
			if(IsValid(SDKRpgState) == true)
			{
				auto LevelTable = USDKTableManager::Get()->FindTableRow<FS_Level>(ETableType::tb_Level, SectorID);
				if(LevelTable != nullptr)
				{
					SDKRpgState->SetCurrentSectorName(LevelTable->Name);
				}
			}
		}	
	}
}

void ASDKRpgGameMode::CheckCurrentPhase()
{
	if (IsValid(RoguelikeManager) == true && IsValid(GetRoguelikeSector()) == true)
	{
		GetRoguelikeSector()->CheckPhase();
	}
	else
	{
		UE_LOG(LogGame, Log, TEXT("Roguelike Sector Invalid!!"));
	}
}

void ASDKRpgGameMode::CheckCurrentStageMode()
{
	UE_LOG(LogGame, Log, TEXT("CheckCurrentStageMode"));

	ASDKRpgState* SDKRpgState = Cast<ASDKRpgState>(GameState);
	if (IsValid(SDKRpgState) == true)
	{
		SDKRpgState->CheckRpgGame(true);
	}
}

void ASDKRpgGameMode::ResetStoreRefreshCost()
{
	auto GDTable = USDKTableManager::Get()->FindTableGlobalDefine(EGlobalDefine::ShopRefresh);
	if (GDTable != nullptr)
	{
		if (GDTable->Value.Num() > 0)
		{
			auto itFirst = GDTable->Value.CreateIterator();
			RefreshStoreCost = FCString::Atoi(**itFirst);
		}
	}
}

void ASDKRpgGameMode::IncreaseDifficultyLevel()
{
	DifficultyLevel++;

	// 몬스터 스탯 상향
	if(DifficultyLevel > 1)
	{
		TArray<AActor*> SpawnedActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASDKNonePlayerCharacter::StaticClass(), SpawnedActors);

		for(auto Actoriter = SpawnedActors.CreateIterator(); Actoriter; Actoriter++)
		{
			auto SDKCharacter = Cast<ASDKNonePlayerCharacter>(*Actoriter);
			if(IsValid(SDKCharacter) == true)
			{
				SDKCharacter->UpdatePawnData();
			}
		}
	}
}

void ASDKRpgGameMode::LoadLevel(const int InColumn)
{
	ASDKRpgState* SDKRpgState = GetGameState<ASDKRpgState>();
	if (IsValid(SDKRpgState) && !SDKRpgState->IsGameActive())
	{
		UE_LOG(LogGame, Log, TEXT("Already Game Finished"));
		return;
	}

	if (bGiveupGame)
	{
		UE_LOG(LogGame, Log, TEXT("Already Give up Game"));
		return;
	}

	CurrColumnIndex = InColumn;

	USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (IsValid(SDKGameInstance))
	{
		if (PassedIDs.IsValidIndex(RoomIndex))
		{
			// 이전 방 정보 삭제
			if (IsValid(CurSubLevel))
			{
				// 방 클리어 패킷 처리
				SetClearRoom(true);
				ClearCurrentRoom();
			}
		}
	}
}

void ASDKRpgGameMode::ClearCurrentRoom()
{
	ASDKRpgState* SDKRpgState = GetGameState<ASDKRpgState>();
	if (IsValid(SDKRpgState))
	{
		SDKRpgState->CheckRemaindItems();

		if (!SDKRpgState->IsGameActive())
		{
			return;
		}
	}

	ASDKInGameController* InGameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!IsValid(InGameController))
	{
		return;
	}

	if (!FSDKHelpers::IsInGlitchCamp(GetWorld()) && Rooms.IsValidIndex(RoomIndex))
	{
		// 룸 클리어 정보
		FRoomClearData ClearData = FRoomClearData();

		// 정보값 설정
		ASDKRpgPlayerState* SDKRpgPlayerState = InGameController->GetPlayerState<ASDKRpgPlayerState>();
		if (IsValid(SDKRpgPlayerState))
		{
			ClearData.LevelID = FCString::Atoi(*Rooms[RoomIndex].LevelID);
			ClearData.CharacterLevel = SDKRpgPlayerState->GetLevel();
			ClearData.WeaponList = SDKRpgPlayerState->GetWeaponList();
			ClearData.ArtifactList = SDKRpgPlayerState->GetArtifactList();
			ClearData.ConsumeGold = SDKRpgPlayerState->GetTotalUseGold();

			ClearData.MapItem = SDKRpgPlayerState->GetPlayingRoomData().MapItem;
			ClearData.MapMonster = SDKRpgPlayerState->GetPlayingRoomData().MapMonster;
			ClearData.ObjectCount = SDKRpgPlayerState->GetPlayingRoomData().ObjectCount;
			ClearData.CurrentGold = SDKRpgPlayerState->GetPlayingRoomData().CurrentGold;
			ClearData.LevelupBuffList = SDKRpgPlayerState->GetLevelUpBuffIDs();
			ClearData.IsRoomClear = GetClearRoom();

			if (GetGiveupGame())
			{
				ClearData.IsRoomClear = GetClearRoom();
			}

			SDKRpgPlayerState->SetTotalUseGold(-ClearData.ConsumeGold);
			SDKRpgPlayerState->SetPlayingRoomData(ClearData);
		}

		// 룸 클리어 정보 서버로 전송
		USDKGameInstance* SDKGameInstance = Cast<USDKGameInstance>(GetGameInstance());
		if (IsValid(SDKGameInstance))
		{
			SDKGameInstance->LobbyServerManager->CQ_RpgRoomEnd(ClearData);
		}
	}
}

void ASDKRpgGameMode::LoadNextRoom()
{
	ASDKRpgState* SDKRpgState = GetGameState<ASDKRpgState>();
	if (!IsValid(SDKRpgState) || !SDKRpgState->IsGameActive())
	{
		return;
	}

	// 현재 방 정보
	FRoomData& RoomData = Rooms[RoomIndex];

	// 다음 방 생성
	if (RoomData.NextRoomTypes.Num() > 0)
	{
		// 다음방 타입 목록
		TArray<int32> ValueArray;
		RoomData.NextRoomTypes.GenerateKeyArray(ValueArray);

		// 현재 컬럼값으로
		PrevColumn = CurColumn;
		if (ValueArray.IsValidIndex(CurrColumnIndex))
		{
			CurColumn = ValueArray[CurrColumnIndex];
		}

		// 다음 방의 인덱스 설정
		if (RoomData.NextStageMapIDs.Contains(CurColumn))
		{
			auto StageMapTable = USDKTableManager::Get()->FindTableRow<FS_StageMap>(ETableType::tb_StageMap, RoomData.NextStageMapIDs[CurColumn]);
			if (StageMapTable != nullptr)
			{
				int32 add = (Rooms.Num() - StageMapTable->Row) - RoomData.Row + 1;
				RoomIndex += add;
			}
		}
		else
		{
			RoomIndex += 1;
		}

		if (Rooms.IsValidIndex(RoomIndex))
		{
			if (Rooms[RoomIndex].StageMapID.IsEmpty())
			{
				SetNextRoomDataBySelectedWay(RoomIndex, CurrColumnIndex, RoomData.Row);
			}

			// 다음 방의 정보 설정
			FRoomData& NextData = Rooms[RoomIndex];
			NextData.RoomType = RoomData.NextRoomTypes[CurColumn];
			NextData.Column = CurColumn;

			if (NextData.RoomType == FName("Puzzle"))
			{
				// 스페셜 레벨 선택
				if (SpecialLevelIndex >= 0)
				{
					NextData.LevelID = USDKTableManager::Get()->GetSpecialLevelIDsByChapter(CurrentChapterID)[SpecialLevelIndex];
				}
			}

			if (NextData.LevelID.IsEmpty())
			{
				NextData.LevelID = GetMakedLevelID(RoomData.NextRoomTypes[CurColumn]);
			}

			USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
			if (SDKGameInstance && SDKGameInstance->IsCompletedGlitchMode())
			{
				SDKGameInstance->LobbyServerManager->CQ_RpgRoomStart(FCString::Atoi(*NextData.LevelID));
			}
		}
	}
	else
	{
		// 마지막 방까지 모두 클리어 한 상황
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Invalid RoomIndex!!"));
	}
}

void ASDKRpgGameMode::ActiveRoomReward(bool bHaveSector)
{
	if (FSDKHelpers::IsGlitchTutorialMode(GetWorld()))
	{
		return;
	}

	UE_LOG(LogGame, Log, TEXT("[%s] ActiveRoomReward"), *GetName());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("GameMode : Active Room Reward"));

	AActor* RewardActor = UGameplayStatics::GetActorOfClass(GetWorld(), ASDKObjectRewardPortal::StaticClass());
	if(IsValid(RewardActor) == true)
	{
		ASDKObjectRewardPortal* SDKObject = Cast<ASDKObjectRewardPortal>(RewardActor);
		if (IsValid(SDKObject) == true)
		{
			if (CurrentRoomType == FName("Store"))
			{
				SDKObject->SetActiveGetAllItems(bHaveSector);
				SDKObject->ActiveRewardPortal();
				return;
			}

			auto LevelTypeTable = USDKTableManager::Get()->FindTableRow<FS_LevelType>(ETableType::tb_LevelType, Rooms[RoomIndex].RoomType.ToString());
			if (LevelTypeTable != nullptr)
			{
				ASDKRpgState* RpgState = GetGameState<ASDKRpgState>();
				if (IsValid(RpgState) == true)
				{
					if (CurrentRoomType != FName("Boss"))
					{
						bool bHaveReward = false;
						if (RpgState->GetClearCurrentSectorEvent() == true)
						{
							// 이벤트를 클리어한 경우
							if (LevelTypeTable->SuccessRewardObjectID.Num() > 0)
							{
								bHaveReward = true;
								int32 index = FMath::RandRange(0, LevelTypeTable->SuccessRewardObjectID.Num() - 1);
								SDKObject->SpawnRewardObject(LevelTypeTable->SuccessRewardObjectID[index]);
							}
						}
						else
						{
							if (LevelTypeTable->RewardObjectID.Num() > 0)
							{
								bHaveReward = true;
								int32 index = FMath::RandRange(0, LevelTypeTable->RewardObjectID.Num() - 1);
								SDKObject->SpawnRewardObject(LevelTypeTable->RewardObjectID[index]);
							}
						}

						if (bHaveReward == false)
						{
							SDKObject->SetActiveGetAllItems(bHaveSector);
							SDKObject->ActiveRewardPortal();
						}
					}
				}
			}
		}
		else
		{
			UE_LOG(LogGame, Warning, TEXT("Failed Cast SDKObjectRewardPortal"));
		}
	}
	else
	{
		UE_LOG(LogGame, Warning, TEXT("Not Found Reward Portal Actor List"));
	}
}

bool ASDKRpgGameMode::IsSetUserAttributeData()
{
	ASDKInGameController* SDKIngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (true == IsValid(SDKIngameController))
	{
		ASDKPlayerState* SDKPlayerState = Cast<ASDKPlayerState>(SDKIngameController->PlayerState);
		if (true == IsValid(SDKPlayerState))
		{
			bool IsSetData = SDKPlayerState->GetAttributeList().Num() > 0;
			return IsSetData;
		}
	}

	return false;
}

void ASDKRpgGameMode::CheckSetStartGameGold()
{
	ASDKInGameController* SDKInGameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKInGameController))
	{
		ASDKPlayerState* SDKPlayerState = Cast<ASDKPlayerState>(SDKInGameController->PlayerState);
		if (IsValid(SDKPlayerState))
		{
			const int32 StartGold = SDKPlayerState->GetAttAbilityValue(EParameter::StartGoldCount);
			SDKPlayerState->SetGold(SDKPlayerState->GetGold() + StartGold);

			SDKPlayerState->SetTotalGetGold(SDKPlayerState->GetGold());

			SDKInGameController->ClientChangeGlitchGoldType(true);
		}
	}
}

void ASDKRpgGameMode::RewardBoxOverlapped(int32 BoxID)
{
	auto SDKGameInstance = Cast<USDKGameInstance>(GetGameInstance());
	if (IsValid(SDKGameInstance) == true)
	{
		SDKGameInstance->LobbyServerManager->CQ_RpgOpenBox(BoxID);
	}
}

bool ASDKRpgGameMode::IsInGlitchCamp()
{
	return FSDKHelpers::IsInGlitchCamp(GetWorld());
}

void ASDKRpgGameMode::BindClearSectorPhase()
{
	ASDKRpgState* BindRpgState = GetGameState<ASDKRpgState>();
	if (IsValid(BindRpgState) == true)
	{
		BindRpgState->ClearSectorPhase();
	}
}

void ASDKRpgGameMode::CompletedLoadedRoguelikeSector()
{
	ASDKInGameController* SDKIngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKIngameController) == true)
	{
		if (IsValid(SDKIngameController->PlayerCameraManager) == true)
		{
			SDKIngameController->PlayerCameraManager->StartCameraFade(1.f, 0.f, 1.f, FColor::Black);
		}
	}

	ASDKRpgState* SDKRpgState = GetGameState<ASDKRpgState>();
	if (IsValid(SDKRpgState))
	{
		if (CurrentRoomType == FName("Start"))
		{
			OnCheckLevelSequenceEvent();
		}
	}

	if (IsValid(GetRoguelikeManager()) == true && GetRoguelikeManager()->GetSectorCount() <= 0)
	{
		// 섹터가 없는 경우, 일단 보상은 스폰되도록
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ASDKRpgGameMode::BindClearSectorPhase, 0.5f, false);
		

		UE_LOG(LogGame, Log, TEXT("Not Found Same Sector Group Index. Faild Spawn RoguelikeSector. LevelID : %s"), *CurrentLevelID);

#if WITH_EDITOR
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("Not Found Same Sector Group Index. Faild Spawn RoguelikeSector."), true, FVector2D(1.5f, 1.5f));
#endif
	}

	if (CurrentRoomType == FName("Start"))
	{
		USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
		if (IsValid(SDKGameInstance) && SDKRpgState->GetGamePlayState() != EGameplayState::Finished)
		{
			if (Rooms.IsValidIndex(RoomIndex))
			{
				int32 LevelID = FCString::Atoi(*Rooms[RoomIndex].LevelID);
				SDKGameInstance->LobbyServerManager->CQ_RpgRoomStart(LevelID);
			}
			else
			{
				UE_LOG(LogGame, Warning, TEXT("Not Fonud RoomIndex : %d"), RoomIndex);
			}
		}

		ServerReadyComplete();
	}

	LevelLoadInSubLevel();
}

void ASDKRpgGameMode::SpawnPlayer()
{
	ASDKRpgState* SDKRpgState = GetGameState<ASDKRpgState>();
	if (IsValid(SDKRpgState))
	{
		USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
		if (IsValid(SDKGameInstance))
		{
			ASDKInGameController* SDKInGameController = Cast<ASDKInGameController>(PlayerData.NewPlayer);
			if (IsValid(SDKInGameController))
			{
				if (IsValid(SDKInGameController->GetPawn()) == false)
				{
					// 플레이어 시작 위치 선별
					const AActor* StartSpot = FindPlayerStartCustom(PlayerData.NewPlayer);
					if (IsValid(StartSpot))
					{
						int32 HeroID = PlayerData.SelectHeroID;
						int32 HairSkinID = PlayerData.SelectHairSkinID;
						int32 BodySkinID = PlayerData.SelectBodySkinID;

						UE_LOG(LogArena, Log, TEXT("SkinID : Server (%s)"), *FString::FromInt(BodySkinID));

						ASDKPlayerState* SDKPlayerState = SDKInGameController->GetPlayerState<ASDKPlayerState>();
						if (IsValid(SDKPlayerState))
						{
							const FHeroData* HeroData = SDKGameInstance->MyInfoManager->GetHeroData().Find(HeroID);
							if (HeroData != nullptr)
							{
								SDKPlayerState->SetHeroData(FPlayerHeroData(*HeroData));
							}

							const FHeroSkinData* HeroSkinData = SDKGameInstance->MyInfoManager->GetHeroSkinData().Find(BodySkinID);
							if (HeroSkinData != nullptr)
							{
								SDKPlayerState->SetSkinData(FPlayerHeroSkinData(*HeroSkinData));
							}

							if (IsValid(SDKRpgState) == true)
							{
								SDKRpgState->InitFreeArtifactCount();
							}
						}

						SDKInGameController->ServerSpawnHero(HeroID, BodySkinID, StartSpot->GetActorTransform());
					}
					else
					{
						if (SpawnLoopCount >= 100)
						{
							ASDKInGameController* PlayerIngameController = Cast<ASDKInGameController>(PlayerData.NewPlayer);
							if (IsValid(PlayerIngameController))
							{
								PlayerIngameController->FailedToSpawnPawn();
							}

							return;
						}

						TaskAfterSeconds(this, [this]()
						{
							SpawnPlayer();
						}, CheckSpawnIntervalTime);

						SpawnLoopCount++;
					}

					if (IsValid(SDKInGameController->GetPawn()))
					{
						SetPlayerDefaults(SDKInGameController->GetPawn());
					}
				}
			}
		}
	}
}

int ASDKRpgGameMode::GetRoomIndex() const
{
	return RoomIndex;
}

void ASDKRpgGameMode::CreateRoom()
{
	// 방 생성
	if (PassedIDs.IsValidIndex(RoomIndex) == false)
	{
		FRoomData& RoomData = Rooms[RoomIndex];
		CurColumn = RoomData.Column;

		if (RoomData.LevelID.IsEmpty() == true)
		{
			RoomData.LevelID = GetMakedLevelID(RoomData.RoomType);
		}

		auto LevelTable = USDKTableManager::Get()->FindTableRow<FS_Level>(ETableType::tb_Level, RoomData.LevelID);
		if (LevelTable != nullptr)
		{
			UE_LOG(LogGame, Log, TEXT("CreateRoom Success : %s"), *RoomData.LevelID);
			UE_LOG(LogGame, Log, TEXT("StageID : %d, LevelID : %s, RoomIdx : %02d"), CurrentStageID, *RoomData.LevelID, RoomIndex);

			SetLevelID(RoomData.LevelID);
			SetRoomType(RoomData.RoomType);
			SetStageMapID(RoomData.StageMapID);

			AddPassedStageMapID(RoomData.StageMapID);

			FString TestPath(TEXT("/Game/Maps/05_Roguelike/"));
			TestPath += FString::Printf(TEXT("Chapter%02d/"), CurrentChapterID);

			LoadMap(TestPath, LevelTable->Name, FVector::ZeroVector, FRotator::ZeroRotator);
			AddLoaedLevel(RoomData.LevelID, FName(LevelTable->Name));

			TArray< FLoadSubLevel> FindValues;
			LoadSubLevelList.MultiFind(FName(LevelTable->Name), FindValues);

			// 로드 : 오브젝트 레벨
			int32 ObjectCount = LevelTable->ObjectGroup1.Num();
			if (ObjectCount > 0)
			{
				int32 idx = FMath::RandRange(0, ObjectCount - 1);
				CreateObjectStreamLevel(LevelTable->ObjectGroup1[idx].ToString());
			}
		}
		else
		{
			UE_LOG(LogGame, Error, TEXT("CreateRoom Fail!! : %s"), *RoomData.LevelID);
		}

		auto SDKRpgState = Cast<ASDKRpgState>(GameState);
		if (IsValid(SDKRpgState) == true)
		{
			SDKRpgState->SetCurrentFloor(CurrentStageID > 0 ? RoomData.Row : 1);
			SDKRpgState->SetCurrentMapType(RoomData.RoomType);
			SDKRpgState->SetCurrentLevelID(RoomData.LevelID);
			SDKRpgState->AddSelectedRoomInfo(RoomIndex, CurColumn);
		}
	}
}

void ASDKRpgGameMode::CreateRoomStreamLevel(int32 InColumn /*= 0*/)
{
	// 방 생성
	if (PassedIDs.IsValidIndex(RoomIndex) == true)
	{
		FRoomData& RoomData = Rooms[RoomIndex];

		// 해당 레벨 로드
		auto LevelTable = USDKTableManager::Get()->FindTableRow<FS_Level>(ETableType::tb_Level, RoomData.LevelID);
		if (LevelTable != nullptr)
		{
			UE_LOG(LogGame, Log, TEXT("CreateRoom Success : %s"), *RoomData.LevelID);
			UE_LOG(LogGame, Log, TEXT("RoomType : %s, RoomIdx : %02d"), *RoomData.RoomType.ToString(), RoomIndex);

			SetLevelID(RoomData.LevelID);
			SetRoomType(RoomData.RoomType);
			SetStageMapID(RoomData.StageMapID);

			FString TestPath(TEXT("/Game/Maps/05_Roguelike/"));
			TestPath += FString::Printf(TEXT("Chapter%02d/"), CurrentChapterID);

			LoadMap(TestPath, LevelTable->Name, FVector::ZeroVector, FRotator::ZeroRotator);
			AddLoaedLevel(RoomData.LevelID, FName(LevelTable->Name));

			TArray< FLoadSubLevel> FindValues;
			LoadSubLevelList.MultiFind(FName(LevelTable->Name), FindValues);

			auto InGameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
			if (IsValid(InGameController) == true)
			{
				// 카메라 회전값 설정
				FRotator CameraRotator = FRotator(-50.264f, 45.f, 0.f) + FRotator(0.f, LevelTable->CameraRotation, 0.f);
				InGameController->InitControlRotation(CameraRotator);
				
				auto PlayerCharacter = InGameController->GetPawn<ASDKPlayerCharacter>();
				if (IsValid(PlayerCharacter) == true)
				{
					PlayerCharacter->ClientSetRotatorDefault(CameraRotator);
				}

				// 오브젝트 레벨 로드
				int32 ObjectCount = LevelTable->ObjectGroup1.Num();
				if (ObjectCount > 0)
				{
					bool bRet = false;

					// 상점의 경우, 일반 상점 혹은 암시장이 특성의 확률에 따라 선택되도록 해야 함.
					if (LevelTable->Set == TEXT("Store"))
					{
						auto PlayerState = Cast<ASDKPlayerState>(InGameController->PlayerState);
						if (IsValid(PlayerState) == true)
						{
							// 만분율 계산
							int32 ActiveRate = (int32)(10000 * PlayerState->GetAttAbilityRate(EParameter::BlackMarketActiveRate));
							int32 ActiveBlackMarketPercent = /*FMath::RandRange(1, 10000)*/1;

							int32 idx = 0;
							if (ActiveRate >= ActiveBlackMarketPercent)
							{
								// 암시장으로 설정, 
								// ObjectGroup1의 0번 인덱스를 일반상점, 1번을 암시장으로 설정.
								idx = 1;
							}

							// 오브젝트 그룹에 해당 인덱스 값이 없을 경우
							if (LevelTable->ObjectGroup1.IsValidIndex(idx) == false)
							{
								idx = 0;
							}

							CreateObjectStreamLevel(LevelTable->ObjectGroup1[idx].ToString());
						}
					}
					else
					{
						int32 idx = FMath::RandRange(0, ObjectCount - 1);
						CreateObjectStreamLevel(LevelTable->ObjectGroup1[idx].ToString());
					}

					bHaveObjectSubLevel = true;
				}
				else
				{
					bHaveObjectSubLevel = false;
				}
			} 
		}
		else
		{
			UE_LOG(LogGame, Error, TEXT("CreateRoom Fail!! : %s"), *RoomData.LevelID);
		}
	}
}

void ASDKRpgGameMode::CreateObjectStreamLevel(FString ObjectLevelPath, bool bIsSubRoomLevel /*= false*/)
{
	UWorld* World = GetWorld();
	FString LongPackageName = ObjectLevelPath;

	const FString ShortPackageName = FPackageName::GetShortName(LongPackageName);
	const FString PackagePath = FPackageName::GetLongPackagePath(LongPackageName);

	FString UniqueLevelPackageName = PackagePath + TEXT("/") + FString::FromInt(RoomIndex) + World->StreamingLevelsPrefix + ShortPackageName;

	ULevelStreamingDynamic* StreamingLevel = NewObject<ULevelStreamingDynamic>(World, ULevelStreamingDynamic::StaticClass(), NAME_None, RF_Transient, NULL);
	StreamingLevel->SetWorldAssetByPackageName(FName(*UniqueLevelPackageName));
	StreamingLevel->LevelColor = FColor::MakeRandomColor();
	StreamingLevel->SetShouldBeVisible(true);
	StreamingLevel->SetShouldBeLoaded(true);
	StreamingLevel->OnLevelShown.AddUniqueDynamic(this, &ASDKRpgGameMode::CompletedObjectStreamLevel);

	StreamingLevel->LevelTransform = FTransform(FRotator::ZeroRotator, FVector::ZeroVector);

	// Map to Load
	StreamingLevel->PackageNameToLoad = FName(*LongPackageName);

	World->AddStreamingLevel(StreamingLevel);

	if (map_ObjectLevelList.Contains(CurSubLevel) == true)
	{
		map_ObjectLevelList.Find(CurSubLevel)->Emplace(StreamingLevel);
	}
	else
	{
		TArray<ULevelStreamingDynamic*> AddSubLevels;
		AddSubLevels.Empty();

		AddSubLevels.Emplace(StreamingLevel);

		map_ObjectLevelList.Emplace(CurSubLevel, AddSubLevels);
	}

	UE_LOG(LogGame, Log, TEXT("Success load object level : %s"), *ShortPackageName);
}

void ASDKRpgGameMode::LoadMap(const FString & LevelPath, const FString & LevelName, const FVector& Loc, const FRotator& Rot)
{
	FString LongPackageName = LevelPath + LevelName;

	TArray<FLoadSubLevel> FindValue;
	LoadSubLevelList.MultiFind(FName(*LevelName), FindValue, false);

	UWorld* world = GetWorld();
	FString name = CreateStreamInstance(world, LongPackageName, FName(*LevelName), FindValue.Num(), Loc, Rot);
}

FString ASDKRpgGameMode::CreateStreamInstance(UWorld* World, const FString& LongPackageName, const FName& LevelName, int32 Index, const FVector Location, const FRotator Rotation)
{
	const FString ShortPackageName = FPackageName::GetShortName(LongPackageName);
	const FString PackagePath = FPackageName::GetLongPackagePath(LongPackageName);
	FString UniqueLevelPackageName = PackagePath + TEXT("/") + FString::FromInt(Index) + World->StreamingLevelsPrefix + ShortPackageName;

	// Setup streaming level object that will load specified map
	ULevelStreamingDynamic* StreamingLevel = NewObject<ULevelStreamingDynamic>(World, ULevelStreamingDynamic::StaticClass(), LevelName, RF_Transient, NULL);
	StreamingLevel->SetWorldAssetByPackageName(FName(*UniqueLevelPackageName));
	StreamingLevel->LevelColor = FColor::MakeRandomColor();
	StreamingLevel->SetShouldBeVisible(true);
	StreamingLevel->SetShouldBeLoaded(true);
	StreamingLevel->OnLevelShown.AddUniqueDynamic(this, &ASDKRpgGameMode::CompletedStreamLevel);
	
	// Location Rotation
	StreamingLevel->LevelTransform = FTransform(Rotation, Location);

	// Map to Load
	StreamingLevel->PackageNameToLoad = FName(*LongPackageName);

	World->AddStreamingLevel(StreamingLevel);

	LoadSubLevelList.Emplace(LevelName, FLoadSubLevel(StreamingLevel, Index, Location, Rotation, nullptr));

	CurSubLevel = StreamingLevel;

	return UniqueLevelPackageName;
}

void ASDKRpgGameMode::SetLoadedRoomSetting()
{
	ClearPortalData();

	if (IsValid(GetRoguelikeManager()) == true)
	{
		GetRoguelikeManager()->ClearRoguelikeSectorList();
	}

	TaskAfterSeconds(this, [this]()
	{
		FindLoadedRoguelikeSector();
	}, CheckSectorIntervalTime);
}

void ASDKRpgGameMode::FindLoadedRoguelikeSector()
{
	const auto LevelTable = USDKTableManager::Get()->FindTableRow<FS_Level>(ETableType::tb_Level, CurrentLevelID);
	if (!LevelTable)
	{
		return;
	}

	if(LevelTable->WaveGroup1.Num() > 0 || LevelTable->SectorGroupIndex > 0) // 전투방, SectorLocation 로드를 기다림
	{
		if(LevelTable->WaveGroup1.Num() > 0)	// 전투방
		{
			for (int32 i = 0; i < 4; ++i)
			{
				TArray<FWaveGruop> WaveList;
				TArray<int32> RewardIDs;
				ASDKSectorLocation* SectorLocation = nullptr;

				if (CheckRoguelikeSectorData(i, WaveList, RewardIDs, SectorLocation) == false)
				{
					continue;
				}

				const int32 RewardIndex = FMath::RandRange(0, RewardIDs.Num() - 1);
				const int32 RewardID = RewardIDs.IsValidIndex(RewardIndex) ? RewardIDs[RewardIndex] : 0;
				const int32 WaveIndex = FMath::RandRange(0, WaveList.Num() - 1);

				if (WaveList.IsValidIndex(WaveIndex) && WaveList[WaveIndex].Waves.Num() > 0)
				{
					ASDKRoguelikeSector* NewSector = AddRoguelikeSector(i, RewardID, SectorLocation);
					if (IsValid(NewSector))
					{
						OnSetSectorObjectList(WaveList[WaveIndex].Waves, NewSector);
					}
				}
			}
		}
		else if(LevelTable->SectorGroupIndex > 0)	// 전투는 없지만 섹터가 필요한 타입
		{
			TArray<FWaveGruop> NoneWaveList;
			TArray<int32> RewardIDs;
			ASDKSectorLocation* SectorLocation = nullptr;

			if (CheckRoguelikeSectorData(0, NoneWaveList, RewardIDs, SectorLocation))
			{
				const int32 RewardIndex = FMath::RandRange(0, RewardIDs.Num() - 1);
				const int32 RewardID = RewardIDs.IsValidIndex(RewardIndex) ? RewardIDs[RewardIndex] : 0;

				ASDKRoguelikeSector* NewSector = AddRoguelikeSector(0, RewardID, SectorLocation);
				if (IsValid(NewSector))
				{
					OnSetSectorObjectList(TArray<FName>(), NewSector);
				}
			}
		}

		if (GetRoguelikeManager()->GetSectorCount() > 0)
		{
			CompletedLoadedRoguelikeSector();
		}
		else
		{
			if (SectorLoopCount >= 100)
			{
				UE_LOG(LogGame, Warning, TEXT("Not Found BP_SctorLocation"));
				CompletedLoadedRoguelikeSector();
				return;
			}

			TaskAfterSeconds(this, [this]()
			{
				FindLoadedRoguelikeSector();
			}, CheckSectorIntervalTime);

			SectorLoopCount++;
		}
	}
	else
	{
		CompletedLoadedRoguelikeSector();
	}
}

bool ASDKRpgGameMode::CheckRoguelikeSectorData(int32 index, TArray<FWaveGruop>& WaveList, TArray<int32>& RewardIDs, ASDKSectorLocation*& SectorLocation)
{
	auto LevelTable = USDKTableManager::Get()->FindTableRow<FS_Level>(ETableType::tb_Level, CurrentLevelID);
	if (LevelTable != nullptr)
	{
		switch (index)
		{
		case 0:
		{
			WaveList = LevelTable->WaveGroup1;
			RewardIDs = LevelTable->Rewards1;
		}
		break;
		case 1:
		{
			WaveList = LevelTable->WaveGroup2;
			RewardIDs = LevelTable->Rewards2;
		}
		break;
		case 2:
		{
			WaveList = LevelTable->WaveGroup3;
			RewardIDs = LevelTable->Rewards3;
		}
		break;
		case 3:
		{
			WaveList = LevelTable->WaveGroup4;
			RewardIDs = LevelTable->Rewards4;
		}
		break;
		default:
			break;
		}

		if (WaveList.Num() > 0 || RewardIDs.Num() > 0 || index == 0)
		{
			TArray<AActor*> LocationActorList;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASDKSectorLocation::StaticClass(), LocationActorList);
			if (LocationActorList.Num() > 0)
			{
				for (auto iter : LocationActorList)
				{
					SectorLocation = Cast<ASDKSectorLocation>(iter);
					if (IsValid(SectorLocation) == true)
					{
						if (SectorLocation->GetSectorGroupIndex() == LevelTable->SectorGroupIndex && SectorLocation->GetSectorLocationIndex() == index + 1)
						{
							return true;
						}
					}
				}
			}

			return false;
		}
	}

	return false;
}

ASDKRoguelikeSector* ASDKRpgGameMode::AddRoguelikeSector(int32 index, int32 RewardID, ASDKSectorLocation* SectorLocation)
{
	if (IsValid(SectorLocation) == false)
	{
		return nullptr;
	}

	UE_LOG(LogGame, Log, TEXT("[GameMode][Add_Roguelike_Sector] Index : %d"), index);

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	auto BPSectorClass = LoadClass<ASDKRoguelikeSector>(nullptr, TEXT("Blueprint'/Game/Blueprints/Manager/BP_RoguelikeSector.BP_RoguelikeSector_C'"));
	if (IsValid(BPSectorClass) == true)
	{
		ASDKRoguelikeSector* NewSector = GetWorld()->SpawnActor<ASDKRoguelikeSector>(BPSectorClass, SectorLocation->GetActorTransform(), SpawnInfo);
		if (IsValid(NewSector) == true)
		{
			NewSector->SetActorScale3D(SectorLocation->GetActorScale3D());

			if (IsValid(GetRoguelikeManager()) == true)
			{
				GetRoguelikeManager()->AddRoguelikeSector(NewSector, FRpgSectorData(CurrentLevelID, FString::FromInt(RewardID), index, CurrentRoomType));
			}

			NewSector->SetSectorLocation(SectorLocation);
			NewSector->SetActorEnableCollision(true);

			UE_LOG(LogGame, Log, TEXT("[GameMode][Add_Roguelike_Sector] Successed Create Sector"));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("[GameMode][Add_Roguelike_Sector] Successed Create Sector"));
			return NewSector;
		}
	}

	return nullptr;
}

void ASDKRpgGameMode::InitRoom(bool bLast /*= true*/)
{
	auto StageTable = USDKTableManager::Get()->FindTableRow<FS_Stage>(ETableType::tb_Stage, FString::FromInt(CurrentStageID));
	if (StageTable != nullptr)
	{
		// 각 행에 따른 최대 열
		TArray<FLevelSet> arrMapMaxColumn = USDKTableManager::Get()->GetMapMaxColumnByChapter(CurrentChapterID);
		GetMapColumnDataByStage(StageTable->Difficulty, arrMapMaxColumn);

		// 방 초기 설정
		int32 RoomCount = FMath::RandRange(StageTable->CountMin, StageTable->CountMax);
		for (int32 i = 1; i <= RoomCount; i++)
		{
			FRoomData RoomData;
			RoomData.Row = i;

			// 다음 방 Column
			FLevelSet NextRoomData = arrMapMaxColumn.IsValidIndex(i) ? arrMapMaxColumn[i] : FLevelSet();
			int32 MaxNext = NextRoomData.Columns.Num();

			// 방 타입 설정
			if (i == 1)
			{
				// 시작방
				RoomData.RoomType = FName("Start");
			}
			else if (i == RoomCount)
			{
				// 보스방
				RoomData.RoomType = FName("Boss");
			}
			else
			{
				// 그 외의 방
				RoomData.RoomType = FName("None");
			}

			// 현재 방 StageMapID
			if (arrMapMaxColumn.IsValidIndex(i - 1) == true)
			{
				int32 index = i - 1;
				if (arrMapMaxColumn[index].ColumnStageMapIDs.Num() == 1)
				{
					RoomData.StageMapID = arrMapMaxColumn[index].ColumnStageMapIDs.Last();
					auto StageMapTable = USDKTableManager::Get()->FindTableRow<FS_StageMap>(ETableType::tb_StageMap, RoomData.StageMapID);
					if (StageMapTable != nullptr)
					{
						RoomData.Column = StageMapTable->Column;
						CurColumn = StageMapTable->Column;
					}
				}
			}

			for (int32 j = 0; j < MaxNext; j++)
			{
				if (NextRoomData.Columns.IsValidIndex(j) == true)
				{
					(RoomData.NextRoomTypes).Emplace(NextRoomData.Columns[j], (i == RoomCount - 1) ? FName("Boss") : FName("None"));
					(RoomData.NextStageMapIDs).Emplace(NextRoomData.Columns[j], NextRoomData.ColumnStageMapIDs[j]);
				}
			}

			Rooms.Emplace(RoomData);
		}

		InitLevelSet();
		InitLinkedNextRoomData();
	}

	SetNextRoomDataBySelectedWay(0, CurColumn, 0);
}

void ASDKRpgGameMode::InitLevelSet()
{
	FS_Stage* StageTable = USDKTableManager::Get()->FindTableRow<FS_Stage>(ETableType::tb_Stage, FString::FromInt(CurrentStageID));
	if (StageTable != nullptr)
	{
		// 특수방으로 교체 설정
		// 특수방 만큼 루프
		int32 LevelSetCount = StageTable->LevelSet.Num();
		for (int32 i = 0; i < LevelSetCount; i++)
		{
			if (StageTable->LevelSet[i] <= 0)
			{
				continue;
			}

			auto LevelSetTable = USDKTableManager::Get()->FindTableRow<FS_LevelSet>(ETableType::tb_LevelSet, FString::FromInt(StageTable->LevelSet[i]));
			if (LevelSetTable != nullptr && LevelSetTable->Set.Num() > 0)
			{
				// 랜덤 세트 인덱스 초기화
				TArray<int32> RandSetIdxes;
				for (int32 j = 0; j < LevelSetTable->Set.Num(); j++)	// 특수방 종류 만큼 추가
				{
					RandSetIdxes.Add(j);
				}

				// 랜덤 세트 뒤섞기
				if (RandSetIdxes.Num() > 1)
				{
					int32 LastIndex = RandSetIdxes.Num() - 1;
					for (int32 j = 0; j <= RandSetIdxes.Num(); ++j)
					{
						int32 Index = FMath::RandRange(j, LastIndex);
						if (j != Index)
						{
							RandSetIdxes.Swap(j, Index);
						}
					}
				}
				int32 SpecialRoomCount = StageTable->Count[i];
				int32 SpecialRoomIdx = FMath::RandRange(StageTable->StepMin[i], StageTable->StepMax[i]) - 2;
				
				if (Rooms.IsValidIndex(SpecialRoomIdx) == true)
				{
					// 다음방 목록에서 일반방 목록 인덱스만 추출
					TArray<int32> NextIndex;
					for (auto& iter : Rooms[SpecialRoomIdx].NextRoomTypes)
					{
						if (iter.Value != FName("None"))
						{
							continue;
						}

						NextIndex.Add(iter.Key);
					}

					// 랜덤 세트 인덱스 설정
					int32 SetIdx = 0;

					// 카운트 만큼 루프
					for (int32 iCount = 0; iCount < SpecialRoomCount; iCount++)
					{
						if (NextIndex.Num() <= 0)
						{
							break;
						}

						int32 index = NextIndex[0];
						NextIndex.RemoveAt(0);

						// 특수방으로 설정
						Rooms[SpecialRoomIdx].NextRoomTypes[index] = LevelSetTable->Set[RandSetIdxes[SetIdx]];

						// 퍼즐방이면서 이미 다 퍼즐방을 클리어한 상태인 경우
						if (Rooms[SpecialRoomIdx].NextRoomTypes[index] == FName("Puzzle"))
						{
							if (SpecialLevelIndex < 0)
							{
								TArray<FName> TypeList = { FName("Levelup"), FName("Gold"), FName("Relic"), FName("Enhance"), FName("EnhanceStone") };
								int32 TypeIdx = FMath::RandRange(0, TypeList.Num() - 1);
								Rooms[SpecialRoomIdx].NextRoomTypes[index] = TypeList[TypeIdx];
							}
						}

						// 방 타입이 여러 가지인 경우, 랜덤 세트 인덱스 증가
						if (RandSetIdxes.Num() > 2)
						{
							SetIdx++;
						}
					}
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SpecialRoom Index is Invalid!!"));
				}
			}
		}
	}
}

void ASDKRpgGameMode::InitLinkedNextRoomData()
{
	if (Rooms.Num() <= 0)
	{
		UE_LOG(LogGame, Log, TEXT("Room Data List EMPTY"));
		return;
	}

	for (int32 i = 0; i < Rooms.Num(); ++i)
	{
		if (Rooms[i].StageMapID.IsEmpty() == true)
		{
			continue;
		}

		SetNextRoomData(Rooms[i]);
	}
}

void ASDKRpgGameMode::InitShuffleRoom()
{
	int32 MaxCount = 99;
	if (CurrentStageID > 0)
	{
		auto StageTable = USDKTableManager::Get()->FindTableRow<FS_Stage>(ETableType::tb_Stage, FString::FromInt(CurrentStageID));
		if (StageTable != nullptr)
		{
			MaxCount = StageTable->CountMax;
		}
	}

	int32 StageGroup = 0;
	FS_Chapter* ChapterTable = USDKTableManager::Get()->FindTableRow<FS_Chapter>(ETableType::tb_Chapter, FString::FromInt(CurrentChapterID));
	if (ChapterTable != nullptr)
	{
		StageGroup = ChapterTable->StageGroup;
	}

	for (int32 iType = int32(ERoomType::Start); iType < int32(ERoomType::Max); ++iType)
	{
		FString RoomTypeString = FSDKHelpers::EnumToString(TEXT("ERoomType"), iType);
		if (RoomTypeString.IsEmpty() == true)
		{
			continue;
		}

		ShuffleLevelNumber.Emplace(iType);
		ExcludeShuffleLevelNumber.Emplace(iType);

		TMap<int32, TArray<int32>>* mapLevelNumber = ShuffleLevelNumber.Find(iType);
		TArray<int32>* LevelNumber = nullptr;

		for (int32 i = 1; i < 100; ++i)
		{
			FString LevelName = FString::Printf(TEXT("%d%02d%02d"), StageGroup, iType, i);
			auto LevelTable = USDKTableManager::Get()->FindTableRow<FS_Level>(ETableType::tb_Level, LevelName);
			if (LevelTable == nullptr)
			{
				break;
			}

			if (LevelTable->StepMin == 99 || LevelTable->StepMax == 99)
			{
				continue;
			}

			if (LevelTable->StepMin != LevelTable->StepMax)
			{
				for (int32 Step = LevelTable->StepMin; Step <= LevelTable->StepMax; ++Step)
				{
					if (mapLevelNumber->Contains(Step) == false)
					{
						mapLevelNumber->Emplace(Step);
					}

					LevelNumber = mapLevelNumber->Find(Step);
					LevelNumber->AddUnique(i);
				}
			}
			else
			{
				if (mapLevelNumber->Contains(LevelTable->StepMin) == false)
				{
					mapLevelNumber->Emplace(LevelTable->StepMin);
				}

				LevelNumber = mapLevelNumber->Find(LevelTable->StepMin);
				LevelNumber->AddUnique(i);
			}
		}

		if (LevelNumber != nullptr && LevelNumber->Num() > 0)
		{
			const int32 NumShuffles = LevelNumber->Num() - 1;
			for (int32 i = 0; i < NumShuffles; ++i)
			{
				int32 SwapIdx = FMath::RandRange(i, NumShuffles);
				LevelNumber->Swap(i, SwapIdx);
			}
		}
	}
}

void ASDKRpgGameMode::GetMapColumnDataByStage(EDifficulty Difficulty, TArray<FLevelSet>& arrResult)
{
	if (Difficulty == EDifficulty::None || arrResult.Num() <= 0)
	{
		return;
	}

	int32 iMax = arrResult.Num() - 1;
	for (int32 idx = iMax; idx >= 0; idx--)
	{
		if (arrResult[idx].Difficulty != Difficulty || arrResult[idx].StageID != CurrentStageID)
		{
			arrResult.RemoveAt(idx);
		}
	}
}

void ASDKRpgGameMode::SetNextRoomData(FRoomData& OutRoomData)
{
	auto StageMapTable = USDKTableManager::Get()->FindTableRow<FS_StageMap>(ETableType::tb_StageMap, OutRoomData.StageMapID);
	if (StageMapTable == nullptr)
	{
		return;
	}

	if (StageMapTable->NextLevelID.Num() <= 0)
	{
		return;
	}

	// 다다음 방의 타입 목록
	TMap<int32, FName> mapNextTypes = OutRoomData.NextRoomTypes;
	int32 NextIndex = OutRoomData.Row - 1;

	OutRoomData.NextStageMapIDs.Empty();
	OutRoomData.NextRoomTypes.Empty();

	for (auto itID = StageMapTable->NextLevelID.CreateIterator(); itID; ++itID)
	{
		auto NextTable = USDKTableManager::Get()->FindTableRow<FS_StageMap>(ETableType::tb_StageMap, *itID);
		if (NextTable != nullptr)
		{
			OutRoomData.NextStageMapIDs.Add(NextTable->Column, *itID);
			if (StageMapTable->Line.IsValidIndex(itID.GetIndex()) == true)
			{
				if (mapNextTypes.Contains(NextTable->Column) == true)
				{
					// 기존에 갖고 있던 정보
					OutRoomData.NextRoomTypes.Add(NextTable->Column, mapNextTypes[NextTable->Column]);
				}
				else
				{
					if (StageMapTable->Line[itID.GetIndex()] >= EUIAttributeLine::LongCenter)
					{
						if (Rooms[NextIndex + 1].NextRoomTypes.Contains(NextTable->Column) == true)
						{
							OutRoomData.NextRoomTypes.Add(NextTable->Column, Rooms[NextIndex + 1].NextRoomTypes[NextTable->Column]);
						}
					}
				}
			}
		}
	}

	// 소팅
	OutRoomData.NextRoomTypes.KeySort([](int32 A, int32 B) {return A < B; });
	OutRoomData.NextStageMapIDs.KeySort([](int32 A, int32 B) {return A < B; });
}

int32 ASDKRpgGameMode::GetRoomLevelCount(int32 RoomType, int32 Row)
{
	if (ShuffleLevelNumber.Contains(RoomType) == true)
	{
		TMap<int32, TArray<int32>>* ShuffleValue = ShuffleLevelNumber.Find(RoomType);
		if (ShuffleValue->Contains(Row) == true)
		{
			TArray<int32>* ShuffleNumber = ShuffleValue->Find(Row);

			if (ExcludeShuffleLevelNumber.Contains(RoomType) == false || ExcludeShuffleLevelNumber[RoomType].Num() <= 0)
			{
				return ShuffleNumber->Num();
			}
			else
			{
				TArray<int32> EnableNumber = *ShuffleNumber;
				TArray<int32>* ExcludeNumber = ExcludeShuffleLevelNumber.Find(RoomType);

				for (auto iter = ExcludeNumber->CreateIterator(); iter; ++iter)
				{
					if (EnableNumber.Contains(*iter) == true)
					{
						EnableNumber.Remove(*iter);
					}
				}

				// 해당 방타입의 레벨 넘버를 다 사용한 경우
				if (EnableNumber.Num() <= 0)
				{
					ExcludeNumber->Empty();

					return ShuffleNumber->Num();
				}

				return EnableNumber.Num();
			}
		}
	}

	return 0;
}

int32 ASDKRpgGameMode::GetRoomLevelRandomNumber(int32 RoomType, int32 Row, int32 Idx)
{
	if (ShuffleLevelNumber.Contains(RoomType) == true)
	{
		TMap<int32, TArray<int32>>* ShuffleValue = ShuffleLevelNumber.Find(RoomType);
		if (ShuffleValue->Contains(Row) == true)
		{
			TArray<int32>* ShuffleNumber = ShuffleValue->Find(Row);

			if (ExcludeShuffleLevelNumber.Contains(RoomType) == false || ExcludeShuffleLevelNumber[RoomType].Num() <= 0)
			{
				if (ShuffleNumber->IsValidIndex(Idx) == true)
				{
					// 선택된 레벨 다음에 제외되도록 추가
					if (ExcludeShuffleLevelNumber.Contains(RoomType) == false)
					{
						ExcludeShuffleLevelNumber.Add(RoomType);
					}

					ExcludeShuffleLevelNumber[RoomType].AddUnique((*ShuffleNumber)[Idx]);

					return (*ShuffleNumber)[Idx];
				}
			}
			else
			{
				TArray<int32> EnableNumber = *ShuffleNumber;
				TArray<int32>* ExcludeNumber = ExcludeShuffleLevelNumber.Find(RoomType);

				for (auto iter = ExcludeNumber->CreateIterator(); iter; ++iter)
				{
					if (EnableNumber.Contains(*iter) == true)
					{
						EnableNumber.Remove(*iter);
					}
				}

				if (EnableNumber.IsValidIndex(Idx) == true)
				{
					// 선택된 레벨 다음에 제외되도록 추가
					ExcludeNumber->AddUnique(EnableNumber[Idx]);

					return EnableNumber[Idx];
				}
			}
		}
	}

	return 0;
}

int32 ASDKRpgGameMode::GetRoomCountNextSelectable(int32 RoomIdx, int32 InColumn)
{
	int32 Cnt = 0;
	NextRoomTypeValues.Empty();

	if (PassedIDs.IsValidIndex(RoomIdx) == true)
	{
		FString MapID = PassedIDs[RoomIdx];
		FRoomData RoomData = Rooms[RoomIdx];

		if (InColumn == 1)
		{
			if (RoomData.NextRoomTypes.Contains(InColumn))
			{
				NextRoomTypeValues.Add(Cnt, RoomData.NextRoomTypes[InColumn]);
				Cnt++;
			}
			if (RoomData.NextRoomTypes.Contains(InColumn + 1))
			{
				NextRoomTypeValues.Add(Cnt, RoomData.NextRoomTypes[InColumn + 1]);
				Cnt++;
			}
		}
		else if (InColumn == 5)
		{
			if (RoomData.NextRoomTypes.Contains(InColumn - 1))
			{
				NextRoomTypeValues.Add(Cnt, RoomData.NextRoomTypes[InColumn - 1]);
				Cnt++;
			}
			if (RoomData.NextRoomTypes.Contains(InColumn))
			{
				NextRoomTypeValues.Add(Cnt, RoomData.NextRoomTypes[InColumn]);
				Cnt++;
			}
		}
		else
		{
			if (RoomData.NextRoomTypes.Contains(InColumn - 1))
			{
				NextRoomTypeValues.Add(Cnt, RoomData.NextRoomTypes[InColumn - 1]);
				Cnt++;
			}
			if (RoomData.NextRoomTypes.Contains(InColumn))
			{
				NextRoomTypeValues.Add(Cnt, RoomData.NextRoomTypes[InColumn]);
				Cnt++;
			}
			if (RoomData.NextRoomTypes.Contains(InColumn + 1))
			{
				NextRoomTypeValues.Add(Cnt, RoomData.NextRoomTypes[InColumn + 1]);
				Cnt++;
			}
		}
	}

	return Cnt;
}

FString ASDKRpgGameMode::GetMakedLevelID(FName RoomTypeName)
{
	int32 RoomType = FSDKHelpers::StringToEnum(TEXT("ERoomType"), RoomTypeName.ToString());
	int32 RoomLevelCount = GetRoomLevelCount(RoomType, RoomIndex + 1);
	if (RoomLevelCount > 0)
	{
		int32 RandomIndex = 0;
		if (RoomType != (int32)ERoomType::Start && RoomType != (int32)ERoomType::Boss)
		{
			RandomIndex = FMath::RandRange(0, RoomLevelCount - 1);
		}

		int32 RandomLevelNumber = GetRoomLevelRandomNumber(RoomType, RoomIndex + 1, RandomIndex);

		FS_Chapter* ChapterTable = USDKTableManager::Get()->FindTableRow<FS_Chapter>(ETableType::tb_Chapter, FString::FromInt(CurrentChapterID));
		if (ChapterTable != nullptr)
		{
			FString NextLevelID = FString::Printf(TEXT("%d%02d%02d"), ChapterTable->StageGroup, RoomType, RandomLevelNumber);

			FS_Level* CurrentTable = USDKTableManager::Get()->FindTableRow<FS_Level>(ETableType::tb_Level, CurrentLevelID);
			if(CurrentTable != nullptr)
			{
				FS_Level* NextTable = USDKTableManager::Get()->FindTableRow<FS_Level>(ETableType::tb_Level, NextLevelID);
				if (NextTable != nullptr)
				{
					if (CurrentTable->LevelPath.ToString() == NextTable->LevelPath.ToString())
					{
						NextLevelID = GetMakedLevelID(RoomTypeName);
					}
				}
			}

			return NextLevelID;
		}
	}
	else
	{
		UE_LOG(LogGame, Warning, TEXT("Not Fonud Room Level Count : %s Type / %d Step"), *RoomTypeName.ToString(), RoomIndex + 2);
	}

	return TEXT("");
}

void ASDKRpgGameMode::SetNextRoomDataBySelectedWay(int32 NextIndex, int32 SelectedIndex, int32 PrevRow)
{
	// 다음 방
	FRoomData& NextRoom = Rooms[NextIndex];

	// 다다음 방의 타입 목록
	TMap<int32, FName> mapNextTypes = NextRoom.NextRoomTypes;

	// 현재 방(이동 전)
	int32 PrevIndex = NextIndex - (NextIndex - (PrevRow - 1));
	if(Rooms.IsValidIndex(PrevIndex) == true)
	{
		auto CurrentTable = USDKTableManager::Get()->FindTableRow<FS_StageMap>(ETableType::tb_StageMap, Rooms[PrevIndex].StageMapID);
		if (CurrentTable != nullptr)
		{
			if (CurrentTable->NextLevelID.IsValidIndex(SelectedIndex) == true)
			{
				NextRoom.StageMapID = CurrentTable->NextLevelID[SelectedIndex];
			}
		}
	}

	// 선택한 방의 다음 방들의 정보 설정
	SetNextRoomData(NextRoom);
}

void ASDKRpgGameMode::InitMonster()
{
	const auto MonsterList = USDKTableManager::Get()->GetMonsterListByList(CurrentChapterID);
	if (MonsterList.Num() > 0)
	{
		for (auto Iter : MonsterList)
		{
			if (!Iter.Value.IsValid())
			{
				continue;
			}

			mapLoadedMonster.Emplace(Iter.Key, USDKAssetManager::Get().LoadSynchronousSubclass<AActor>(Iter.Value));
		}
	}
}

void ASDKRpgGameMode::AddPassedStageMapID(FString ID)
{
	if (RoomIndex > 1 && PassedIDs.IsValidIndex(RoomIndex - 1) == false)
	{
		PassedIDs.Add(TEXT(""));
	}

	if (PassedIDs.Contains(ID) == false)
	{
		PassedIDs.Add(ID);
	}
}

void ASDKRpgGameMode::LevelLoadInSubLevel()
{
	ULevelStreamingDynamic* CurSelectLevel = CurSubLevel;
	
	if (IsValid(CurSelectLevel) == false) 
	{
		return;
	}
		
	UPackage* InSubPackage = FindObject<UPackage>(nullptr, *CurSelectLevel->GetWorldAsset().GetLongPackageName());
	if (IsValid(InSubPackage) == true)
	{
		TArray<ULevelStreaming*> LoadLevelArr = UWorld::FindWorldInPackage(InSubPackage)->GetStreamingLevels();
		if (LoadLevelArr.Num() > 0)
		{
			TArray<ULevelStreamingDynamic*> AddSubLevels;
			AddSubLevels.Empty();
			UWorld* World = GetWorld();
			for(auto Iter : LoadLevelArr)
			{
				if(Iter->ShouldBeAlwaysLoaded() != true)
				{
					continue;
				}

				if (IsValid(Iter))
				{
					FString LevelName = Iter->GetWorldAssetPackageName();
					FString PrefixLevelName = UWorld::RemovePIEPrefix(LevelName);

					bool bRet = false;
					FVector LaodPostion = CurSelectLevel->LevelTransform.GetLocation() + Iter->LevelTransform.GetLocation();
					ULevelStreamingDynamic* StreamingLevel = ULevelStreamingDynamic::LoadLevelInstance(GetWorld(), PrefixLevelName, LaodPostion, FRotator::ZeroRotator, bRet);
					AddSubLevels.Add(StreamingLevel);
				}
			}

			if(AddSubLevels.Num() > 0)
			{
				if (map_SubLevelList.Contains(CurSelectLevel) == true)
				{
					for (auto iter : AddSubLevels)
					{
						map_SubLevelList.Find(CurSelectLevel)->Emplace(iter);
					}
				}
				else
				{
					map_SubLevelList.Emplace(CurSelectLevel, AddSubLevels);
				}
			}
		}
	}

	FTimerHandle NextRoomTimer;
	FTimerDelegate NextRoomDelegate;
	NextRoomDelegate.BindUFunction(this, FName("CompletedLoadNextRoom"));
	GetWorld()->GetTimerManager().SetTimer(NextRoomTimer, NextRoomDelegate, 1.f, false);
}

void ASDKRpgGameMode::CompletedLoadNextRoom()
{
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);

	ASDKInGameController* InGameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(InGameController) == true)
	{
		InGameController->ClientMoveToNextRoom(false);

		if (PlayerStarts.Num() > 0)
		{
			ASDKPlayerCharacter* SDKCharacter = InGameController->GetPawn<ASDKPlayerCharacter>();
			if (IsValid(SDKCharacter) == true)
			{
				FVector vLocation = PlayerStarts[0]->GetActorLocation();
				vLocation.Z += 50.f;

				SDKCharacter->BeginTeleport(vLocation);
				SDKCharacter->SetActorRotation(PlayerStarts[0]->GetActorRotation());

				if(CurrentRoomType == FName("Boss"))
				{
					if (IsValid(GetRoguelikeManager()->GetRoguelikeSector(0)))
					{
						GetRoguelikeManager()->GetRoguelikeSector(0)->OnSetActiveCollision(true);
					}
				}
			}
		}
		else
		{
			UE_LOG(LogGame, Warning, TEXT("Not Found PlayerStart : %s"), *CurrentLevelID)
		}
	}
}

void ASDKRpgGameMode::RemoveSubLevelInLevel(ULevelStreamingDynamic* RemoveLevel)
{
	if(IsValid(RemoveLevel) == true)
	{
		if (map_SubLevelList.Contains(RemoveLevel) == false)
		{
			return;
		}

		for (auto Iter : map_SubLevelList[RemoveLevel])
		{
			if (IsValid(Iter) == true)
			{
				RemoveLoadedLevel(Iter->PackageNameToLoad.ToString());
			}
		}

		map_SubLevelList.Remove(RemoveLevel);
		RemoveLoadedLevel(RemoveLevel->PackageNameToLoad.ToString());
	}
}

void ASDKRpgGameMode::RemoveObejctLevel(ULevelStreamingDynamic* RemoveLevel)
{
	if (IsValid(RemoveLevel) == true)
	{
		if (map_ObjectLevelList.Contains(RemoveLevel))
		{
			for (auto Iter : map_ObjectLevelList[RemoveLevel])
			{
				if (IsValid(Iter) == true)
				{
					RemoveLoadedLevel(Iter->PackageNameToLoad.ToString());
				}
			}
		}

		map_ObjectLevelList.Remove(RemoveLevel);
		RemoveLoadedLevel(RemoveLevel->PackageNameToLoad.ToString());
	}
}

void ASDKRpgGameMode::RemoveSubLevel(ULevelStreamingDynamic* RemoveLevel)
{
	if (IsValid(RemoveLevel) == true)
	{
		RemoveLevel->SetShouldBeLoaded(false);
		RemoveLevel->SetIsRequestingUnloadAndRemoval(true);
	}
}

void ASDKRpgGameMode::RemoveLoadedAllLevels()
{
	if (IsValid(GetWorld()) && GetWorld()->GetStreamingLevels().Num() > 0)
	{
		for (auto itLevel : GetWorld()->GetStreamingLevels())
		{
			itLevel->SetShouldBeLoaded(false);
			itLevel->SetIsRequestingUnloadAndRemoval(true);
		}
	}

	CurSubLevel = nullptr;
}

void ASDKRpgGameMode::RemovePreviousLevelActors()
{
	RemoveSpawnedItems();
	RemoveSpawnedRandomBlockList();
	RemoveSubPlayerStart();
	RemoveSubNextLevelPortal();
}

void ASDKRpgGameMode::RemoveSpawnedItems()
{
	TArray<AActor*> ItemList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASDKItem::StaticClass(), ItemList);
	if (ItemList.Num() > 0)
	{
		for (auto iter : ItemList)
		{
			if (IsValid(Cast<ASDKPlayerCharacter>(iter->GetOwner())) == true)
			{
				continue;
			}

			iter->SetLifeSpan(0.01f);
		}
	}
}

void ASDKRpgGameMode::RemoveSpawnedRandomBlockList()
{
	TArray<AActor*> BlockList;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), AActor::StaticClass(), FName("RandomBlock"), BlockList);
	if (BlockList.Num() > 0)
	{
		for (auto iter : BlockList)
		{
			iter->Destroy();
		}
	}
}

void ASDKRpgGameMode::RemoveSubPlayerStart()
{
	TArray<AActor*> StartList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), StartList);
	if (StartList.Num() > 0)
	{
		for (auto iter : StartList)
		{
			iter->Destroy();
		}
	}
}

void ASDKRpgGameMode::RemoveSubNextLevelPortal()
{
	TArray<AActor*> NextPortalList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASDKObjectRewardPortal::StaticClass(), NextPortalList);
	if (NextPortalList.Num() > 0)
	{
		for (auto iter : NextPortalList)
		{
			iter->Destroy();
		}
	}
}

void ASDKRpgGameMode::AddLoaedLevel(FString ID, FName Name)
{
	if (mapLoadedLevelID.Contains(ID) == false)
	{
		mapLoadedLevelID.Add(ID, Name);
	}
}

void ASDKRpgGameMode::RemoveLoadedLevel(FString LevelName)
{
	int32 Index = LevelName.Find(TEXT("."));
	if (Index <= INDEX_NONE)
	{
		Index = LevelName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	}

	if (Index > INDEX_NONE)
	{
		LevelName = LevelName.RightChop(Index + 1);
	}

	if (mapLoadedLevelID.FindKey(FName(LevelName)) != nullptr)
	{
		FString ID = *mapLoadedLevelID.FindKey(FName(LevelName));
		mapLoadedLevelID.Remove(ID);
	}

	TArray<FLoadSubLevel> FindValue;
	LoadSubLevelList.MultiFind(FName(LevelName), FindValue, false);

	if (FindValue.Num() > 0)
	{
		LoadSubLevelList.Remove(FName(LevelName));
	}
}
