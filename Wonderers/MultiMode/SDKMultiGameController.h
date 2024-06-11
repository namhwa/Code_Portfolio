// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/SDKInGameController.h"
#include "Share/SDKStruct.h"
#include "SDKMultiGameController.generated.h"

class ULevelSequence;
class ASDKHeroAIController;

UCLASS()
class ARENA_API ASDKMultiGameController : public ASDKInGameController
{
	GENERATED_BODY()
	
public:
	// Begin SDKPlayerController inteface
	UFUNCTION(Client, Reliable)
	virtual void ClientAttachHud() override;
	virtual void ClientAttachBackgroundMainWidget() override;
	virtual void ClientToggleState_Implementation() override {}
	// End SDKPlayerController interface

	/** 골드 크래시 모드 */
	UFUNCTION(Client, Reliable)
	void ClientUpdateGoldClashRankerTransform(FVector RankerLoc, int32 RankerSpawnNum, int32 RankerTeamNum);

	UFUNCTION(Client, Reliable)
	void ClientUpdateGoldRegenRegion(const TArray<int32>& RegenIndex);

	//*** HUD ***************************************************************************************************************//
	/** UI : 인게임 메인 UI 변경 */
	UFUNCTION(Client, Reliable)
	virtual void ClientChangeWaitingRoomUI(EGameplayState eState);

	UFUNCTION(Client, Reliable)
	virtual void ClientChangeIngameMainUI();

	/** UI : 상점 Close*/
	UFUNCTION(Client, Reliable)
	void ClientRespawnShopClose();

	/** 알림 : 인벤토리 초기 설정 */
	void ClientInitMiscItems(const TArray<FItemSlotData>& MiscList);

	/** 갱신 : 골드 크러쉬 랭크 */
	UFUNCTION(Client, Reliable)
	void ClientUpdateGoldClashRank(const TArray<FGoldClashResultInfo>& RankingList);

	/** 갱신 : 골드 크러쉬 랭커 위치 */
	UFUNCTION(Client, Reliable)
	void ClientUpdateGoldClashRankerPositionAndRotation(bool bIsMe, FVector2D vPosition = FVector2D::ZeroVector, float fRotator = 0.0f);

	/** UI : 멀티모드 인게임 유저 상태창 */
	UFUNCTION(Client, Reliable)
	void Client_ToggleState();

	/** 갱신 : 인게임 현황판 1등 왕관*/
	UFUNCTION(Client, Reliable)
	void Client_ChangeMatchInfoNo1(int32 OldNo1, int32 CurNo1);

	/** 1등팀 표시 갱신 */
	UFUNCTION(Client, Reliable)
	void ClientUpdateChangeNo1Team(int32 InTeamNum, bool bSpectator = false);

	/** 파티 정보 갱신 */
	void UpdatePartyMemberData();

	UFUNCTION(Client, Reliable)
	void ClientEnterToGoldClashResultLevel(const FGoldClashResultUIInfo& InResultInfo, const TArray<FString>& GoldClashMvpLogs);

	/** 알림 : 인풋이 없어서 패널티 먹을 수 있는 상황 */
	void ClientNotifyPenaltyInput();

	/** 패널티 받은 유저 리커넥트 레벨로 이동 */
	UFUNCTION(Client, Reliable)
	void ClientEnterToDodgeReconnect();

	/** 패널티 받은 유저인지 */
	bool GetHavePenaltyPoint() { return bHavePenaltyPoint; }
	void SetHavePenaltyPoint(bool bHavePoint) { bHavePenaltyPoint = bHavePoint; }

	/** 알림 : 나간 유저 같은 팀한테 알림 처리 */
	UFUNCTION(Client, Reliable)
	void ClientNotifyLeaveUser(const FString& InNick, bool bVisibleButton);

	/** 알림 : 연결 끊긴 유저 같은 팀한테 알림 처리 */
	UFUNCTION(Client, Reliable)
	void ClientNotifyDisconnectUser(const FString& InNick);

	/** 알림 : 재접속 유저 같은 팀한테 알림 처리 */
	UFUNCTION(Client, Reliable)
	void ClientNotifyReconnectUser(const FString& InNick);

	/** 알림 : 항복 투표 시작을 서버에 알림*/
	UFUNCTION(Server, Reliable)
	void ServerClickedSurrenderVote();

	/** 항복 투표 UI On/Off */
	UFUNCTION(Client, Reliable)
	void ClientToggleSurrenderVote(bool bOpen);

	/** 항복 결정 넘김 처리 */
	UFUNCTION(Server, Reliable)
	void ServerSendSelectSurrenderVote(bool bAccept);

	/** 항복 투표 UI On/Off */
	UFUNCTION(Client, Reliable)
	void ClientNotifyAllMemberSurrenderVote();

private:
	// 골드 랭킹 스폰 넘버
	int32 OldRanker;
	int32 CurRanker;

	int32 ResultPartyMemberCount;

	bool bHavePenaltyPoint;

	UPROPERTY()
	float ElapsedPingDeltaTime;

	bool bShowPing = false;
	bool bShowFPS = false;

	UPROPERTY()
	ASDKHeroAIController* HeroAIController;
};