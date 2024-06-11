// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/SDKGameState.h"
#include "Share/SDKEnum.h"
#include "Share/SDKStruct.h"
#include "SDKRpgState.generated.h"

class ASDKCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRpgDeathEvent);

UCLASS()
class ARENA_API ASDKRpgState : public ASDKGameState
{
	GENERATED_BODY()
	
public:
	ASDKRpgState();

	// Begin ASDKGameState interface
	virtual void OnGameReady() override;
	virtual void OnGameStart() override;
	virtual void OnGamePlaying() override;
	virtual void OnCharDied() override;
	// End ASDKGameState interface

	/** 로그라이크 방 페이즈 진행 여부 */
	UFUNCTION(BlueprintCallable)
	bool GetPlayingRoomPhase() const { return PlayingRoomPhase; }

	/** 설정 : 현재 페이즈 진행중인지 */
	UFUNCTION(BlueprintCallable)
	void SetPlayingRoomPhase(bool bPlaying);

	/** 설정 : 현재 맵 인덱스 */
	UFUNCTION(BlueprintCallable)
	void SetCurrentMapIndex(int32 iIndex);

	/** 현재 방 타입 */
	UFUNCTION(BlueprintCallable)
	void SetCurrentMapType(FName eType);
	UFUNCTION(BlueprintCallable)
	FName GetCurrentMapType() { return CurrentMapType; }

	/** 설정 : 현재 레벨 이름 */
	FString GetCurrentLevelID() const { return CurrentLevelID; }
	void SetCurrentLevelID(FString InID) { CurrentLevelID = InID; }

	/** 챕터 ID */
	UFUNCTION(BlueprintCallable)
	FString GetChapterID() const { return ChapterID; }
	void SetChapterID(FString newID);

	/** 스테이지 ID */
	FString GetStageID() const { return StageID; }
	void SetStageID(FString InStage);

	/** 현재 층 */
	int32 GetCurrentFloor() const { return Floor; }
	void SetCurrentFloor(int32 NewFloor) { Floor = NewFloor; }

	/** 유저가 죽어서 끝났는지에 대한 여부 */
	bool GetIsDieEnd() { return bDieEnd; }

	/** 현재 로그라이크 구역 이름 */
	void SetCurrentSectorName(FString Name);
	UFUNCTION(BlueprintCallable)
	FString GetCurrentSectorName();

	/** 상점에서 드랍될 수 있는 아이템 */
	TArray<int32> GetDroppableStoreItemIDs(int32 DropID) const;
	void SetDroppableStoreItemIDs(int32 DropID, bool bIsBlackMarket = false, bool bSpawnDarkArtifact = false);
	bool SetDroppedStoreItemID(int32 DropID, int32 ItemID, bool bIsBlackMarket = false, bool bDarkArtifact = false);
	void ResetDroppableStoreItemIDs(int32 DropID, bool bIsBlackMarket = false, bool bSpawnDarkArtifact = false);

	/** 드롭될 유물 리스트 반환*/
	TArray<FArtifactData> GetDropReadyArtifactList();
	TArray<FArtifactData> GetDropReadyDarkArtifactList();

	/** 암시장 세트 완료에 근접한 유물 리스트 반환 */
	TArray<FArtifactData> GetNearArtifactSetComplateList();

	/** 드롭될 유물 아이디 리스트 반환*/
	TArray<int32> GetDropReadyArtifactIDList();
	TArray<int32> GetDropReadyDarkArtifactIDList();

	/** 암시장 세트 완료에 근접한 유물 아이디 리스트 반환 */
	TArray<int32> GetNearArtifactSetComplateIDList();

	/** 드롭될 유물 리스트 세팅*/
	void SetDropReadyArtifactList();
	void SetDropReadyDarkArtifactList();

	/** 드롭될 유물 리스트에서 인벤에 있으면 제외*/
	void CheckRemoveArtifactList();

	/** 드롭리스트에 다시 추가 & 삭제 */
	void AddDropReadyArtifact(int32 ItemID);
	void RemoveDropReadyArtifact(int32 ItemID, bool bDarkArtifact = false);

	/** 유물 무료 구매 특성 설정 */
	void InitFreeArtifactCount();
	int32 GetFreeArtifactCount();
	void SetBuyFreeArtifactCount();

	/** 게임 상태 확인 */
	UFUNCTION(BlueprintCallable)
	void CheckRpgGame(bool bClear);

	/** 방 클리어 후, 남은 보상 확인 */
	UFUNCTION(BlueprintCallable, Category = Game)
	void CheckRemaindItems();

	/** 방 클리어 시, 처리 */
	UFUNCTION()
	virtual void ClearSectorPhase();
	bool IsOpenLevelUpBuff();

	/** 남은 보상 확인 처리 후 */
	UFUNCTION()
	void AfterCheckRemainItems();

	/** 통과한 스테이지 정보 */
	TMap<FString, int32> GetClearStageInfo() { return mapClearStateInfo; }
	void AddClearStateInfo(FString ChapterID, int32 RoomIndex);

	/** 특성 능력치 반환 */
	const FVector2D GetPlayerAttAbility(EParameter Parameter);

	/** 유저가 선택한 방 목록 */
	int32 GetRoomSelectCount() const { return mapSelectedRoom.Num(); }
	TMap<int32, int32> GetmapSelectedRoom() const { return mapSelectedRoom; }
	void AddSelectedRoomInfo(int32 Row, int32 Column);

	/** 유저가 진행중인 인덱스 */
	int32 GetCurrentSelectIndex() const { return CurrentSelectIndex; }
	void SetCurrentSelectIndex(int32 NewIndex);

	/** 씬 연출 */
	UFUNCTION(BlueprintCallable)
	void FinishEnterRPGSequnce();

	/** 재시도 여부 */
	UFUNCTION(BlueprintCallable)
	bool GetRetryGameMode() const { return bRetryGameMode; }
	void SetRetryGameMode(bool InRetry) { bRetryGameMode = InRetry; }

	/** 현재 섹터 이벤트 클리어 */
	bool GetClearCurrentSectorEvent() const { return bClearCurrentSectorEvent; }
	void SetClearCurrentSectorEvent(bool InClear) { bClearCurrentSectorEvent = InClear; }

	/** 섹터 클리어 UI */
	UFUNCTION()
	void OpenUIClearCurrentSector();

	/** 현재 챕터 클리어 여부 */
	bool GetIsClearCurrentChapter() const { return IsClearChapter; }
public:
	UPROPERTY(BlueprintAssignable)
	FRpgDeathEvent RpgDeathEvent;

protected:
	UPROPERTY(Transient, Replicated)
	FString ChapterID;

	UPROPERTY(Transient, Replicated)
	FString StageID;

	UPROPERTY(Transient, Replicated)
	int32 Floor;

	UPROPERTY(Transient, Replicated)
	EDifficulty DifficultyID;

	// 현재 방 페이즈 진행 여부
	UPROPERTY(Transient, Replicated)
	bool PlayingRoomPhase;

	/** 현재 유저가 있는 맵 인덱스 */
	UPROPERTY(Transient, Replicated)
	int32 CurrentMapIndex;

	UPROPERTY(Transient, Replicated)
	FName CurrentMapType;

	/* 드롭 예정 유물 리스트*/
	TArray<FArtifactData> DropReadyArtifactList;

	TArray<FArtifactData> DropReadyDarkArtifactList;

	/* 특성 : 로그라이크 유물 구매 무료 카운트 */
	int32 ArtifactFreeCount = 0;

	TMap<int32, TArray<int32>> mapDroppableIDs;

	FString CurrentLevelID;
	FString CurrentSectorName;

	// Stage 별 룸 클리어 정보
	UPROPERTY()
	TMap<FString, int32> mapClearStateInfo;

	// ServerCheck - testCode begin
	bool bDieEnd;

	// 클리어 방 목록
	UPROPERTY()
	TMap<int32, int32> mapSelectedRoom;

	// 현재 위치
	UPROPERTY()
	int32 CurrentSelectIndex;

	// 재시작 여부
	UPROPERTY()
	bool bRetryGameMode;

	// 현재 섹터 이벤트 완료 여부
	UPROPERTY()
	bool bClearCurrentSectorEvent;

	// 현재 챕터 클리어 여부
	bool IsClearChapter;
};
