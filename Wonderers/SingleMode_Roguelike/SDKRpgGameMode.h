// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "GameMode/SDKGameMode.h"
#include "Share/SDKEnum.h"
#include "Share/SDKStruct.h"

#include "SDKRpgGameMode.generated.h"

class ULevelStreamingDynamic;

class ASDKObject;
class ASDKAIController;

class ASDKSectorLocation;
class ASDKRoguelikeSector;
class ASDKRoguelikeManager;

class CItemData;
class CRpgWaveGroup;

USTRUCT()
struct FLoadSubLevel
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	bool bInit;

	UPROPERTY()
	ULevelStreamingDynamic* SubLevel;

	UPROPERTY()
	int32 Idx;

	UPROPERTY()
	FVector Loc;

	FRotator Rot;

	TArray<FLoadSubLevel*> ParentList;

	FLoadSubLevel* pChild;

	FLoadSubLevel() : 
		bInit(false),
		SubLevel(nullptr),
		Idx(0),
		Loc(FVector::ZeroVector),
		Rot(FRotator::ZeroRotator),
		pChild(nullptr)
	{

	}

	FLoadSubLevel(ULevelStreamingDynamic* pLevel, int32 InIdx, FVector InLoc, FRotator InRot, FLoadSubLevel* pInChild) :
		bInit(false), 
		SubLevel(pLevel), 
		Idx(InIdx),
		Loc(InLoc),
		Rot(InRot),
		pChild(pInChild)
	{

	}

	bool operator==(const FLoadSubLevel& Other) 
	{
		return (SubLevel == Other.SubLevel) && (Idx == Other.Idx);
	}
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDele_Dynamic);

UCLASS()
class ARENA_API ASDKRpgGameMode : public ASDKGameMode
{
	GENERATED_BODY()
	
public:
	ASDKRpgGameMode();

	// Begin GameMode Interface
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void BeginPlay() override;
	// End GameMode Interface

	UFUNCTION()
	void AssetsLoadComplete();

	UFUNCTION()
	void InitMap();
		
	/** 현재 레벨 생성완료 */
	UFUNCTION()
	void CompletedLoadCreateLevel();
	UFUNCTION()
	void CompletedStreamLevel();
	UFUNCTION()
	void CompletedObjectStreamLevel();

	// Begin SDKGameMode Interface
	virtual bool IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const override;
	virtual bool IsRespawnHeroAllowed() const override { return true; }
	virtual void ServerReadyComplete() override;
	virtual void SetPlayerData(const FServerSendData& SendData) override;
	virtual void ReadyGame() override;
	virtual void StartGame() override;
	virtual void GiveUpGame(const int32 SpawnNum) override;
	virtual void FinishGame() override;
	// End SDKGameMode Interface

	/** SA_RpgRoom관련 Delegate */
	UFUNCTION()
	void OnReceiveRoomStart();
	UFUNCTION()
	void OnReceiveRoomEnd();

	UFUNCTION(BlueprintCallable)
	ASDKRoguelikeManager* GetRoguelikeManager() const { return RoguelikeManager; }

	UFUNCTION(BlueprintCallable)
	ASDKRoguelikeSector* GetRoguelikeSector() const;

	int32 GetCurrentChapterID() const { return CurrentChapterID; }

	/** 로드된 레벨 아이디 목록 */
	TArray<FString> GetLoadedLevelIDs();

	/** 맵 정보 */
	TArray<FRoomData> GetRoomDataList();
	FRoomData GetCurrentRoomData() const;

	/** 현재 게임 레벨 */
	UFUNCTION(BlueprintCallable)
	FString GetLevelID() const { return CurrentLevelID; }
	void SetLevelID(FString strLevelID) { CurrentLevelID = strLevelID; }

	/** 게임 포기 여부 */
	bool GetGiveupGame() const { return bGiveupGame; }
	void SetGiveupGame(bool bGiveup) { bGiveupGame = bGiveup; }

	/** 게임 성공 여부 */
	bool GetClearChapter() const { return bClearChapter; }
	void SetClearChapter(bool InClear) { bClearChapter = InClear; }

	/** 방 성공 여부 */
	bool GetClearRoom() const { return bClearRoom; }
	void SetClearRoom(bool InClear);
	void ResetClearRoom() { bClearRoom = false; }

	/** 유저 사망 여부 */
	bool GetCharacterDied() const { return bCharacterDied; }
	void SetCharacterDied(bool bDied) { bCharacterDied = bDied; }

	/** 설정 : 현재 StageMap ID */
	void SetStageMapID(FString InID) { StageMapID = InID; }

	/** 설정 : 현재 방 타입 */
	void SetRoomType(FName NewType);

	/** 설정: 현재 방 정보 */
	void SetCurrentSector(ASDKRoguelikeSector* pSector, FString SectorID);

	/** 확인 : 현재 페이즈 */
	void CheckCurrentPhase();

	/** 확인 : 다음 스테이지 이동 또는 스테이지 종료 */
	void CheckCurrentStageMode();

	/** 상점 리세 가격 */
	void SetRefreshStoreCost(int32 NewCoin) { RefreshStoreCost = NewCoin; }
	int32 GetRefreshStoreCost() const { return RefreshStoreCost; }
	void ResetStoreRefreshCost();

	/** 위협도 : 레벨 반환 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "GetMonsterLevel"))
	int32 GetDifficultyLevel() { return DifficultyLevel; }

	/** 위협도 : 증가 */
	UFUNCTION()
	void IncreaseDifficultyLevel();

	/** 다음 방 선택 및 Room End 알림 */
	UFUNCTION(BlueprintCallable)
	virtual void LoadLevel(const int InColumn);

	/** 현재 방 클리어 */
	virtual void ClearCurrentRoom();

	/** 선택한 다음방 로드 */
	void LoadNextRoom();

	/** 현재 방에 대한 보상 활성화 */
	UFUNCTION()
	virtual void ActiveRoomReward(bool bHaveSector);

	/** 현재 StageMapID 반환 */
	UFUNCTION(BlueprintCallable)
	FString GetStageMapID() const { return StageMapID; }

	/** 로그라이크 플레이어 정보 세팅 확인 */
	bool CheckSetPlayerData() { return IsSetPlayerData; }
	bool IsSetUserAttributeData();

	/** 특성에 따른 시작 골드 지급 확인 */
	void CheckSetStartGameGold();

	UFUNCTION(BlueprintCallable)
	void RewardBoxOverlapped(int32 BoxID);

	UFUNCTION(BlueprintCallable)
	bool IsInGlitchCamp();

	UFUNCTION()
	void BindClearSectorPhase();

	UFUNCTION()
	virtual void CompletedLoadedRoguelikeSector();

	UFUNCTION()
	virtual void SpawnPlayer();
	
	UFUNCTION(BlueprintPure)
	int GetRoomIndex() const;

protected:	
	/** 방 생성 */
	void CreateRoom();
	void CreateRoomStreamLevel(int32 InColumn = 0);

	/** 방 생성 : 오브젝트 */
	virtual void CreateObjectStreamLevel(FString ObjectLevelPath, bool bIsSubRoomLevel = false);

	/** 방 생성 : 맵 로드 */
	void LoadMap(const FString& LevelPath, const FString& LevelName, const FVector& Loc, const FRotator& Rot);
	virtual FString CreateStreamInstance(UWorld* World, const FString& LongPackageName, const FName& LevelName, int32 Index, const FVector Location, const FRotator Rotation);

	/** 설정 : 현재 방에 대한 */
	UFUNCTION()
	void SetLoadedRoomSetting();
	UFUNCTION()
	void FindLoadedRoguelikeSector();
	bool CheckRoguelikeSectorData(int32 index, TArray<FWaveGruop>& WaveList, TArray<int32>& RewardIDs, ASDKSectorLocation*& SectorLocation);
	ASDKRoguelikeSector* AddRoguelikeSector(int32 index, int32 RewardID, ASDKSectorLocation* SectorLocation);

	/** 설정 : 방 정보 */
	void InitRoom(bool bLast = true);
	void InitLevelSet();
	void InitLinkedNextRoomData();
	virtual void InitShuffleRoom();
	void GetMapColumnDataByStage(EDifficulty Difficulty, TArray<FLevelSet>& arrResult);

	/** 설정 : 다음 방의 정보 */
	void SetNextRoomData(FRoomData& OutRoomData);
	/** 설정 : 선택한 다음 방의 정보 */
	void SetNextRoomDataBySelectedWay(int32 NextIndex, int32 SelectedIndex, int32 PrevRow);

	/** 설정 : 현재 레벨 몬스터 미리 로드 */
	void InitMonster();

	/** 반환 : 방 정보 */
	int32 GetRoomLevelCount(int32 RoomType, int32 Row);
	int32 GetRoomLevelRandomNumber(int32 RoomType, int32 Row, int32 Idx);
	int32 GetRoomCountNextSelectable(int32 RoomIdx, int32 InColumn);
	virtual FString GetMakedLevelID(FName RoomTypeName);

	/** BlueprintImplementableEvent *************************************************************************/
	UFUNCTION(BlueprintImplementableEvent)
	void OnSetSectorObjectList(const TArray<FName>& GroupIDs, ASDKRoguelikeSector* NewSector);

	UFUNCTION(BlueprintImplementableEvent)
	void OnCheckLevelSequenceEvent();

protected:
	/** 지나간 StageMap ID 추가 */
	void AddPassedStageMapID(FString ID);

	/** 서브레벨에 붙어있는 서브레벨 로드*/
	UFUNCTION()
	void LevelLoadInSubLevel();

	/** 다음 방 로드 완료 */
	UFUNCTION()
	void CompletedLoadNextRoom();

	/** 로드된 레벨 제거 */
	void RemoveSubLevelInLevel(ULevelStreamingDynamic* RemoveLevel);
	void RemoveObejctLevel(ULevelStreamingDynamic* RemoveLevel);
	void RemoveSubLevel(ULevelStreamingDynamic* RemoveLevel);

	void RemoveLoadedAllLevels();
	void RemovePreviousLevelActors();
	void RemoveSpawnedItems();
	void RemoveSpawnedRandomBlockList();
	void RemoveSubPlayerStart();
	void RemoveSubNextLevelPortal();

	/** 로드한 레벨 정보 */
	void AddLoaedLevel(FString ID, FName Name);
	void RemoveLoadedLevel(FString LevelName);

protected:
	UPROPERTY()
	float CheckSectorIntervalTime;
	UPROPERTY()
	float CheckSpawnIntervalTime;

protected:
	// 현재 게임 레벨 ID
	UPROPERTY(BlueprintReadOnly)
	FString CurrentLevelID;

	UPROPERTY(BlueprintReadOnly)
	FName CurrentRoomType;

	// 로그라이크 매니저
	UPROPERTY()
	ASDKRoguelikeManager* RoguelikeManager;

	// 현재 층 포탈 정보
	TMap<int32, FWarpPointData> mapWarpPoints;

	// 상점 갱신 비용
	UPROPERTY()
	int32 RefreshStoreCost;

/**************************** 챕터 및 스테이지 정보 */
	// 현재 컨텐츠 타입
	EContentsType CurrentContentsType;
	// 현재 챕터
	UPROPERTY()
	int32 CurrentChapterID;
	// 현재 스테이지
	UPROPERTY()
	int32 CurrentStageID;

	// 현재 게임 방 정보
	UPROPERTY()
	TArray<FRoomData> Rooms;

	// 현재 스테이지 맵 ID
	UPROPERTY()
	FString StageMapID;

	// 스페셜 레벨 인덱스
	UPROPERTY()
	int32 SpecialLevelIndex;

	// 현재 방 인덱스
	UPROPERTY()
	int32 RoomIndex;
	
	// 선택한 다음방 컬럼 인덱스
	UPROPERTY()
	int32 CurrColumnIndex;
	// 현재 컬럼
	UPROPERTY()
	int32 CurColumn;
	// 현재 컬럼
	UPROPERTY()
	int32 PrevColumn;

	// 위협도 레벨
	int32 DifficultyLevel;

	// 클리어 포탈 정보
	FString PortalID;
	FTransform PortalTransform;

	// 로드중인 방의 오브젝트 레벨 여부
	bool bHaveObjectSubLevel;

	// 진행한 방 정보
	UPROPERTY()
	TArray<FString> PassedIDs;

	// 레벨 로드
	TMultiMap<FName, FLoadSubLevel> LoadSubLevelList;
	TArray<TSharedPtr<FStreamableHandle>> StreamingHandleList;

	// 레벨 셔플
	//	 RoomType	 Row	LevelNumber
	TMap<int32, TMap<int32, TArray<int32>>> ShuffleLevelNumber;
	//	 RoomType	   LevelNumber
	TMap<int32, TArray<int32>> ExcludeShuffleLevelNumber;

	// 서브 레벨 및 오브젝트 목록
	TMap< ULevelStreamingDynamic*, TArray<ULevelStreamingDynamic*>> map_SubLevelList;
	TMap< ULevelStreamingDynamic*, TArray<ULevelStreamingDynamic*>> map_ObjectLevelList;

	/** 생성된 레벨 아이디 목록 */
	UPROPERTY()
	TMap<FString, FName> mapLoadedLevelID;

	/** 게임 포기 정보 */
	UPROPERTY()
	bool bGiveupGame;

	/** 게임 성공 정보 */
	UPROPERTY()
	bool bClearChapter;

	/** 룸 클리어 정보 */
	UPROPERTY()
	bool bClearRoom;

	// 캐릭터 죽었는지 확인용
	UPROPERTY()
	bool bCharacterDied;

	// 현재 챕터 몬스터 로드 목록
	TMap<int32, TSubclassOf<AActor>> mapLoadedMonster;

public:
	// 현재 서브 레벨
	UPROPERTY()
	ULevelStreamingDynamic* CurSubLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<int32, FName> NextRoomTypeValues;

	UPROPERTY()
	TArray<FSoftObjectPath> AsyncLoadObjectList;

	UPROPERTY()
	bool IsSetPlayerData;

	// 로드 로딩 횟수
	UPROPERTY()
	int32 SectorLoopCount;
	UPROPERTY()
	FServerSendData PlayerData;
	UPROPERTY()
	int32 SpawnLoopCount;
};