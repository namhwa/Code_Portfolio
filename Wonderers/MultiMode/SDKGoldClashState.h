// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/SDKGameState.h"
#include "Share/SDKStruct.h"
#include "SDKGoldClashState.generated.h"

class ASDKMultiGameController;


UCLASS()
class ARENA_API ASDKGoldClashState : public ASDKGameState
{
	GENERATED_BODY()
	
public:
	ASDKGoldClashState();

	virtual void BeginPlay() override;

	// Begin ASDKGameState Interface
	virtual void OnGameStart() override;
	virtual void OnGameReady() override;
	virtual void FinishGame() override;

	virtual void OnGamePlaying() override;
	// End ASDKGameState Interface 

	UFUNCTION(BlueprintCallable)
	TMap<int32, FGoldClashResultInfo> GetMapPlayerRankInfo() { return MapPlayerRankInfo; }

	TArray<int32> GetGoldRegenRegionList() const { return GoldRegenRegionList; }

	/** 타임 오버로 게임이 끝났는지 */
	bool GetFinishedTimeOver() const { return bFinishedTimerOver; }
	void SetFinishedTimeOver(bool InTimeOver) { bFinishedTimerOver = InTimeOver; }

	/** 현재 1등 유저 넘버 반환 */
	int32 GetCurrentNumberOne() { return CurNo1; }

	// 갱신 : 파티멤버
	void UpdatePartyMember();

	// 갱신 : 전체 점수
	void UpdateGoldRank();

	// 갱신 : 1등
	void UpdateRanker(const int32 NewRankerID);

	/** 1등 위치 갱신 */
	UFUNCTION()
	void UpdateRankerPosition();

	void SetGoldClashPlayingTime(int32 InTime);
	UFUNCTION(BlueprintCallable, Category = Game)
	int32 GetGoldClashPlayingTime() const { return GoldClashPlayTime; }
	
	/** 골드 리젠 규칙 */
	void NotifyUpdateGoldRegenRegion(TArray<int32> RegenIndex);

	UFUNCTION(BlueprintCallable)
	ASDKPlayerCharacter* GetRankPlayer(const int32 TargetRank);

	/** 각 팀의 스폰넘버 설정 */
	TArray<int32> GetTeamMemberData(int32 InTeam);
	void AddTeamMemberData(int32 InTeam, int32 InSpawn);
	void RemoveTeamMemberData(int32 InTeam, int32 InSpawn);

	/** 패널티로 강퇴된 유저 정보 제거 */
	void RemovePlayerRankInfo(int32 InSpawnNum);

	/** 나간 파티원 알림 */
	void NotifyLeaveUser(int32 InTeam, FString InNick);

	/** 연결 끊긴 파티원 알림 */
	void NotifyDisconnectUser(int32 InTeam, FString InNickName);
	/** 재접속 파티원 알림 */
	void NotifyReconnectUser(int32 InTeam, FString InNickName);

	/** 항복 투표 열림 알림*/
	void NotifyOpenSurrenderVote(int32 InTeam);

	/** 항복 투표 갱신 */
	void UpdateSurrenderVote(int32 InTeamNumber, int32 InUID, bool bResult);

	/** 항복 투표 완료 */
	UFUNCTION()
	void FinishSurrenderVote(int32 InTeamNumber);

	/** 항복 투표 결과 노티 */
	void NotifyResultSurrenderVote(bool bSurrender, int32 InTeamNumber);

	/** 1등 팀 번호 반환 */
	int32 GetGoldCrownTeamNumber() { return GoldCrownTeamNumber; }

	/** 정렬한 유저 정보 초기화 */
	void ClearSortPlayerArray() { SortPlayerArray.Empty(); }
	
protected:
	void CheckChangeRanker();
	void SortPlayerArrayByGold(TArray<ASDKPlayerState*>& PlayerStateArray);
	void SortGoldClashRank(TMap<int32, FGoldClashResultInfo>& MapSortRanking);
	void UpdateSortPlayerArray();
	bool CheckValiditySortPlayerArray();

private:
	/** 게임 진행 시간 */
	UPROPERTY(Transient, Replicated)
	int32 GoldClashPlayTime;

	/** 게임 종료 타임 */
	UPROPERTY()
	int32 GameOverTime;

	/** 1등 디버프 시작 골드 */
	UPROPERTY()
	int32 DebuffStartGold;

	UPROPERTY()
	bool DebuffGoldFirst;

	/** 무너지는 지역 번호*/
	UPROPERTY(Replicated)
	int32 CurrInValidRegion;

	/** 각 팀 정보 */
	TMap<int32, TArray<int32>> MapTeamMembers;

	/** 유저 랭킹 정보 */
	TMap<int32, FGoldClashResultInfo> MapPlayerRankInfo;

	/** 패시브 골드(초) */
	float PassiveGoldElapsedTime = 0.f;

	/** 랭킹 갱신 타이머 */
	FTimerHandle TimerHandle_UpdateRanking;

	/** 랭커 위치 갱신 타이머 */
	FTimerHandle TimerHandle_RankerPosition;

	/** 골드 크래쉬 종료 타이머 */
	FTimerHandle TimerHandle_GameOver;

	/** 현재 매칭된 모드(인원) */
	UPROPERTY(Replicated)
	EMatchType CurrentMatchType;

	/** 현재 1등 아이디 */
	UPROPERTY(Replicated)
	int32 CurNo1;

	/** 골드 상자 상태 */
	UPROPERTY(Replicated)
	bool bDestroyGoldBox;

	/** 골드 크라운 팀 */
	UPROPERTY(Replicated)
	int32 GoldCrownTeamNumber;

	/** 골드 리젠 구역 */
	UPROPERTY(Replicated)
	TArray<int32> GoldRegenRegionList;

	/** 타임 오버로 끝난 게임인지에 대한 정보 */
	bool bFinishedTimerOver;

	/** 획득 골드 기준 플레이어 랭킹 정보 */
	UPROPERTY()
	TArray<ASDKPlayerState*> SortPlayerArray;

	/** 항복 투표 내용 */
	TMap <bool, TArray<int32>> MapSurrenderVote;

	int32 GoldClashBoxRegenPreTime;
};
