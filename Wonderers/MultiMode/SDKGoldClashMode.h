// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/SDKGameMode.h"
#include "Share/SDKStruct.h"

#include "SDKGoldClashMode.generated.h"

class ASDKItem;
class ASDKObject;
class ASDKCoinMaster;
class ASDKWeaponChangeObject;
class ASDKMultiGameController;
class ASDKPlayerState;

DECLARE_LOG_CATEGORY_EXTERN(LogGoldClash, Log, All);


UCLASS()
class ARENA_API ASDKGoldClashMode : public ASDKGameMode
{
	GENERATED_BODY()

public:
	// Begin AGameMode Interface
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void Logout(AController* Exiting) override;
	// End AGameMode Interface

	// Begin SDKGameMode Interface
	virtual void ServerReadyComplete() override;
	virtual void StartGame() override;
	virtual void GiveUpGame(const int32 TeamNumber) override;
	virtual void FinishGame() override;
	virtual void LeaveGame(const FString& TagID, int32 SpawnNum, int32 TeamNum, const FString& NickName) override;
	// End SDKGameMode interface

	/** 골드 클래시 디폴트 타이머 */
	UFUNCTION()
	void DefaultTimer();

	/** 게임 종료 조건 확인 */
	void CheckModeFinishCondition(const int32 SpawnID, const int32 Gold);

	/** 최초 시작 시 기본 골드 지급 튜토리얼 */
	void SetBaseGoldTutorial();

	/** 패시브 골드 인터벌 간격 (초) */
	float GetPassiveGoldIntervalTime() const;
	
	/** 컨트롤러 추가/삭제 */
	void RemovePlayingController(const int32 SpawnNum);
	void AddPlayingController(const int32 SpawnNum, AController* NewController);

private:
	/** 골클 결과창 정보 */
	void SetFinishGameResultRankingData();

	/** 골클 재접속 유저의 랭킹 정보 */
	void SetFinishExitPlayerRankingData(int32& FinishRank, TArray<FGoldClashResultInfo>& InResultInfo);

	/** 골클 나간 유저 정보 저장 및 제거 */
	void AddMapExitPlayerRankInfoData(const FGoldClashResultInfo& InGoldClashResultInfo);
	void DeleteMapExitPlayerRankInfoData(const int32 InUserID);

	/** 골클 나간 유저 랭킹 관련 정보 저장 및 제거 */
	void AddMapExitPlayerRankPointData(const int32 InUserID, const FRankPointData& InGoldClashRankInfo);
	void DeleteMapExitPlayerRankPointData(const int32 InUserID);

	// 결과창 표시를 위한, 탈주 유저 정보 저장
	void UpdateExitedUserResultInfo(const int32 InUesrID, const FGoldClashResultInfo& InData);
	void AppenExitedUserResultInfo(TArray<FGoldClashResultInfo>& InTargetList);

	/** 골클  게임 시작 */
	void SendStartGoldClashInfo();

	/** 골클 게임 종료 */
	void SendFinishGoldClashInfo();

	float GetTeamAvgRankingPoint(const int32 TeamNum);
	int32 GetWinnerTeamNumberByTimeOver();

	/** 골클 트롤 패널티 */
	void SendMultiGamePenaltyUserInfo(const FGoldClashResultInfo PlayerData, const FRankPointData InRankData);

	/** 랭킹 점수 계산 */
	int32 CalculatePenaltyRankingPoint(const FRankPointData& InRankData);
	
private:
	// 목표 골드량
	int32 MaxGoldPoint;

	// 관전자 수
	int32 MaxSpectators;

	// 1등 스폰 넘버
	int32 RankerSpawnNumber;
	int32 RankerTeamNumber;
	UPROPERTY()
	int32 RankersGold;

	// 플레이중인 컨트롤러
	UPROPERTY()
	TMap<int32, TWeakObjectPtr<AController>> MapPlayingController;

	// 골드 리젠 구역 정보
	TArray<FName> GoldRegenRuleList;
	int32 CurRegenRuleIndex;
	int32 NextRegenRuleIndex;

	TMap<int32, FGoldClashResultInfo> MapExitPlayerRankInfoData;
	TMap<int32, FRankPointData> MapExitPlayerRankPointData;

	// 한번이라도 나간적 있는 유저의 정보를 남겨둡니다.
	TMap<int32, FGoldClashResultInfo> MapExitedUserResultInfo;

	// 팀 전체 랭크 포인트
	TMap<int32, int32> MapTotalTeamRankPoint;
	// 팀 전체 인원(탈주 여부 상관없이 처음에 입장값 그대로)
	TMap<int32, int32> MapInitTeamCount;

	// 게임 종료시 저장되는 결과정보
	FGoldClashResultUIInfo ResultInfo;
	TArray<FString> ResultMvpLogs;

	// 트롤링 당한 팀 넘버
	TArray<int32> TrollingTeamNumberList;
};
