// Fill out your copyright notice in the Description page of Project Settings.


#include "TutorialMode/SDKRpgTutorialGameMode.h"

#include "Engine/LevelStreamingDynamic.h"
#include "Kismet/GameplayStatics.h"

#include "Character/SDKMyInfo.h"
#include "Character/SDKRpgPlayerState.h"
#include "Character/SDKPlayerState.h"
#include "Character/SDKInGameController.h"

#include "Engine/SDKGameTask.h"
#include "Engine/SDKGameInstance.h"
#include "GameMode/SDKRpgState.h"
#include "GameMode/SDKRoguelikeManager.h"

#include "Manager/SDKTableManager.h"
#include "Manager/SDKLobbyServerManager.h"
#include "Object/SDKObjectRewardPortal.h"

#include "Share/SDKData.h"
#include "Share/SDKDataStruct.h"
#include "Share/SDKHelper.h"
#include "Share/SDKStruct.h"


void ASDKRpgTutorialGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	ASDKGameMode::InitGame(MapName, Options, ErrorMessage);

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
	CheckSectorIntervalTime = 1.f;
	CheckSpawnIntervalTime = 1.f;

	auto StageTable = USDKTableManager::Get()->FindTableRow<FS_Stage>(ETableType::tb_Stage, FString::FromInt(CurrentStageID));
	if (StageTable != nullptr)
	{
		Rooms.Empty();

		InitRoom();
		InitShuffleRoom();
	}
}

void ASDKRpgTutorialGameMode::InitGameState()
{
	ASDKGameMode::InitGameState();

	USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (!IsValid(SDKGameInstance))
	{
		return;
	}

	ASDKRpgState* SDKRpgState = Cast<ASDKRpgState>(GameState);
	if (!IsValid(SDKRpgState))
	{
		return;
	}

	SetClearRoom(false);

	CRPGModeData& RpgModeData = SDKGameInstance->MyInfoManager->GetRPGModeData();
	SDKRpgState->SetRetryGameMode(false);
	SDKRpgState->SetGameMode(EGameMode::RpgTutorial);
	SDKRpgState->SetChapterID(RpgModeData.GetChapterIDByString());
	SDKRpgState->SetStageID(RpgModeData.GetStageIDByString());
	SDKRpgState->SetCurrentDifficulty(EDifficulty::Normal);
	SDKRpgState->SetDropReadyArtifactList();
	SDKRpgState->SetDropReadyDarkArtifactList();
}

void ASDKRpgTutorialGameMode::FinishGame()
{
	ASDKGameMode::FinishGame();

	if (IsValid(GetRoguelikeManager()))
	{
		GetRoguelikeManager()->ClearRoguelikeSectorList();
	}

	// 튜토리얼 끝났을 때 보내는 패킷
	USDKGameInstance* SDKGameInstance = Cast<USDKGameInstance>(GetGameInstance());
	if (IsValid(SDKGameInstance))
	{
		SDKGameInstance->LobbyServerManager->CQ_RpgTutorialComplete();
	}
}

void ASDKRpgTutorialGameMode::LoadLevel(const int InColumn)
{
	ASDKRpgState* SDKRpgState = GetGameState<ASDKRpgState>();
	if (IsValid(SDKRpgState) && SDKRpgState->IsGameActive() == false)
	{
		UE_LOG(LogGame, Log, TEXT("Already Game Finished"));
		return;
	}

	SelectColumn = InColumn;

	USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (IsValid(SDKGameInstance))
	{
		if (PassedIDs.IsValidIndex(RoomIndex))
		{
			// 이전 방 정보 삭제
			if (IsValid(CurSubLevel))
			{
				// 저장 변수에서 레벨 제거
				RemoveSubLevelInLevel(CurSubLevel);
				RemoveObejctLevel(CurSubLevel);

				// 실제 레벨 및 오브젝트 제거
				RemovePreviousLevelActors();
				RemoveLoadedAllLevels();

				CurSubLevel = nullptr;
			}
		}

		TaskAfterSeconds(this, [this]()
		{
			LoadNextRoomLevel();
		}, 0.5f);
	}
}

void ASDKRpgTutorialGameMode::ActiveRoomReward(bool bHaveSector)
{
	UE_LOG(LogGame, Log, TEXT("[%s] ActiveRoomReward"), *GetName());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("GameMode : Active Room Reward"));

	AActor* RewardActor = UGameplayStatics::GetActorOfClass(GetWorld(), ASDKObjectRewardPortal::StaticClass());
	if (IsValid(RewardActor) == true)
	{
		ASDKObjectRewardPortal* SDKObject = Cast<ASDKObjectRewardPortal>(RewardActor);
		if (IsValid(SDKObject) == true)
		{
			if (CurrentRoomType == FName("Store"))
			{
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
							bHaveReward = true;
							if (Rooms[RoomIndex].RoomType != FName("Start"))
							{
								int32 index = FMath::RandRange(0, LevelTypeTable->TutorialRewardObjectID.Num() - 1);
								SDKObject->SpawnRewardObject(LevelTypeTable->TutorialRewardObjectID[index]);
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

void ASDKRpgTutorialGameMode::CompletedLoadedRoguelikeSector()
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
	if (IsValid(SDKRpgState) == true)
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
		ServerReadyComplete();
	}

	LevelLoadInSubLevel();
}

FString ASDKRpgTutorialGameMode::CreateStreamInstance(UWorld* World, const FString& LongPackageName, const FName& LevelName, int32 Index, const FVector Location, const FRotator Rotation)
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

void ASDKRpgTutorialGameMode::CreateObjectStreamLevel(FString ObjectLevelPath, bool bIsSubRoomLevel /*= false*/)
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

void ASDKRpgTutorialGameMode::InitShuffleRoom()
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
			FString LevelName = FString::Printf(TEXT("%d%02d"), StageGroup, i);

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

FString ASDKRpgTutorialGameMode::GetMakedLevelID(FName RoomTypeName)
{
	FS_Chapter* ChapterTable = USDKTableManager::Get()->FindTableRow<FS_Chapter>(ETableType::tb_Chapter, FString::FromInt(CurrentChapterID));
	if (ChapterTable != nullptr)
	{
		USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
		if (IsValid(SDKGameInstance))
		{
			if (RoomTypeName == FName("Start"))
			{
				return FString::Printf(TEXT("%d%02d"), ChapterTable->StageGroup, SDKGameInstance->IsCompletedGoldClashMode() ? 5 : 1);
			}
			else
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
					return FString::Printf(TEXT("%d%02d"), ChapterTable->StageGroup, RandomLevelNumber);
				}
			}
		}
	}
	else
	{
		UE_LOG(LogGame, Warning, TEXT("Not Fonud Room Level Count : %s Type / %d Step"), *RoomTypeName.ToString(), RoomIndex + 2);
	}

	return TEXT("");
}

void ASDKRpgTutorialGameMode::LoadNextRoomLevel()
{
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
		if (ValueArray.IsValidIndex(SelectColumn))
		{
			CurColumn = ValueArray[SelectColumn];
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
				SetNextRoomDataBySelectedWay(RoomIndex, SelectColumn, RoomData.Row);
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
				else
				{
					// 임시
					TArray<FName> TypeList = { FName("Levelup"), FName("Gold"), FName("Relic"), FName("Enhance"), FName("EnhanceStone") };
					int32 index = FMath::RandRange(0, TypeList.Num() - 1);
					NextData.RoomType = TypeList[index];
				}
			}

			if (NextData.LevelID.IsEmpty())
			{
				if (NextData.LevelID.IsEmpty())
				{
					NextData.LevelID = GetMakedLevelID(RoomData.NextRoomTypes[CurColumn]);
				}
			}

			// 현재 방 저장 : StageMapID
			AddPassedStageMapID(NextData.StageMapID);
			GetRoomCountNextSelectable(RoomIndex, PrevColumn);

			ASDKRpgState* SDKRpgState = GetGameState<ASDKRpgState>();
			if (IsValid(SDKRpgState) == true)
			{
				// 선택된 방 정보 저장
				SDKRpgState->AddSelectedRoomInfo(RoomIndex, CurColumn);
				SDKRpgState->SetCurrentFloor(RoomData.Row < 1 ? 1 : RoomData.Row);
				SDKRpgState->SetCurrentMapType(NextData.RoomType);
				SDKRpgState->SetCurrentLevelID(NextData.LevelID);

				// 이벤트 정보 초기화
				SDKRpgState->SetClearCurrentSectorEvent(false);
			}
		}

		// 다음방 생성
		CreateRoomStreamLevel(SelectColumn);

		// 방이 생성될 때마다 난이도 상승
		IncreaseDifficultyLevel();

#if WITH_EDITOR

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, FString::Printf(TEXT("SelectColumn : %d / Column : %d"), SelectColumn, SelectColumn));
		UE_LOG(LogTemp, Log, TEXT("SelectColumn : %d / Column : %d"), SelectColumn, SelectColumn);
#endif
	}
	else
	{
		// 마지막 방까지 모두 클리어 한 상황
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Invalid RoomIndex!!"));
	}
}

void ASDKRpgTutorialGameMode::CompletedRpgTutorial()
{
	UE_LOG(LogGame, Log, TEXT("Rpg Tutorial Game Mode::CompletedRpgTutorial"));

	ASDKInGameController* SDKIngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKIngameController) == true)
	{
		ASDKRpgState* SDKRpgState = GetGameState<ASDKRpgState>();
		if (IsValid(SDKRpgState))
		{
			SDKIngameController->ClientRpgFinishGame(SDKRpgState->GetGamePlayingTime(), true);
		}
	}
}
