// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Share/SDKEnum.h"
#include "Share/SDKStruct.h"
#include "SDKRoguelikeSector.generated.h"

DECLARE_DYNAMIC_DELEGATE(FOnCompletedSectorSettingDelegate);

class ASDKObject;
class ASDKSectorLocation;
class ALevelSequenceActor;

USTRUCT(BlueprintType)
struct FPhaseActors
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<ASDKObject*> Objects;

	FPhaseActors()
	{
		Objects.Empty();
	}
};


UCLASS()
class ARENA_API ASDKRoguelikeSector : public AActor
{
	GENERATED_BODY()
		 
public:
	ASDKRoguelikeSector();

	// Begin Actor interface
	virtual void BeginPlay() override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;
	virtual void SetLifeSpan(float InLifespan) override;
	// End Actor interface

	/** 반환 : 전체 페이즈 정보 */
	UFUNCTION(BlueprintCallable)
	TArray<FPhaseActors> GetObjectPhases() const { return ObjectPhases; }

	/** 반환 : 레벨 이름 */
	UFUNCTION(BlueprintCallable)
	FString GetLevelName() const { return LevelName; }

	/** 반환 : 보상ID */
	UFUNCTION(BlueprintCallable)
	FString GetRewardID() const { return RewardID; }
	
	/** 반환 : 현재 페이즈 값 */
	UFUNCTION(BlueprintCallable)
	int32 GetPhaseCount() const { return PhaseCount; }

	/** 반환 : 오브젝트 페이즈 카운트 */
	UFUNCTION(BlueprintCallable)
	int32 GetObjectPhaseCount() const { return ObjectPhases.Num(); }

	/** 반환 : 레벨에 배치된 SectorLocation*/
	ASDKSectorLocation* GetSectorLocation() { return SDKSectorLocation; }

	/** 반환 : 상태값 */
	UFUNCTION(BlueprintCallable)
	bool IsLevelPlaying() const { return bLevelPlaying; }
	bool IsInitSetting() { return bInitSetting; }
	bool IsLevelClear() { return bLevelClear; }
	bool IsJumpTypeLevel();
	
	/** 설정 : 현재 섹터의 레벨 정보 */
	void SetLevelData(struct FRpgSectorData data);
	bool SetCurrentSector();
	void SetPlayingRoomPhase(bool bPlaying);

	/** 설정 : 연결된 섹터 로케이션 */
	UFUNCTION(BlueprintCallable)
	void SetSectorLocation(ASDKSectorLocation* newObject);

	/** 조건값 증가 */
	UFUNCTION(BlueprintCallable)
	void IncreaseConditionValue(int32 value);

	/** 오브젝트 페이즈 */
	UFUNCTION(BlueprintCallable)
	void AddObjectPhase(FPhaseActors newPhase);
	void ClearObjectPhases();

	/** 섹터 상태 체크 */
	UFUNCTION(BlueprintCallable)
	void CheckPhase();

	/** 섹터 오브젝트 목록 설정 후  */
	UFUNCTION(BlueprintCallable)
	void CompletedSetSectorObejctList();

	/** 섹터 설정 완료된 상태 후 */
	UFUNCTION()
	void CompletedInitSectorSetting();

	/** 방 이벤트(미션) 있는 경우, 시작 & 완료 */
	UFUNCTION()
	void StartEventPhase();
	UFUNCTION()
	void ClearEventPhase(bool bClear);

	/** 방 이벤트(미션)에 대한 바인딩 함수 */
	UFUNCTION()
	void OnRoomEventTimeAttack();
	UFUNCTION()
	void OnTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);
	UFUNCTION()
	void OnFailedEventAction();

	/** Platform 방 이벤트 시작&완료 */
	UFUNCTION()
	void StartPlatformRoomEvent();
	UFUNCTION()
	void ClearPlatformRoomEvent(bool bClear);

	/** Platform 방 이벤트 바인딩 함수 */
	UFUNCTION()
	void OnPlatformEventTimeAttack();
	UFUNCTION()
	void OnOverlapPlatformGoal(AActor* OverlappedActor, AActor* OtherActor);

	/** BP 이벤트 : 가이드 이미지 제거 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnClearGuidImage();

	/** BP 이벤트 : 현재 섹터 보상 스폰 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnSpawnClearReward();

	/** BP 이벤트 : 콜리전 활성화 처리 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnSetActiveCollision(bool bActive);

protected:
	/** 초반 섹터 초기화 */
	void InitRootSectorSetting();

	/** 섹서 설정 마무리 */
	UFUNCTION()
	void InitSectorSetting();
	void InitEventMissionSetting();
	void InitPlatformMissionSetting();

	/** 오버랩 이벤트 시, 호출 함수 */
	void SectorOverlappedAcotr(AActor* OtherActor);

	/** 섹터 시작 */
	void StartPhase();

	/** 섹터 클리어 */
	void ClearPhase();

	/** 섹터 클리어 보상 */
	void ClearPhaseReward();

	/** 섹터 클리어 : 남은 아이템 흡수 */
	UFUNCTION()
	void ClearRemainItems();

	/** 설정 : 섹터 이벤트 정보 */
	void SetSectorEvent(FString LevelID);

public:
	/** Delegate Event */
	UPROPERTY(BlueprintReadOnly)
	FOnCompletedSectorSettingDelegate OnCompletedSectorSettingEvent;

protected:
	// 현재 방 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString LevelName;
	// 현재 섹터 인덱스
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RoomIndex;

	// 현재 섹터 방 타입
	UPROPERTY()
	FName SectorType;

	// 보상 ID
	UPROPERTY()
	FString RewardID;

private:
	UPROPERTY()
	TArray<FPhaseActors> ObjectPhases;

	UPROPERTY()
	ASDKSectorLocation* SDKSectorLocation;

	UPROPERTY()
	int32 RoomEventIndex;

	/*보스 레벨 시퀀스를 담을 배열*/
	UPROPERTY()
	TArray<AActor*> SequenceActors;

	// 레벨 에셋 이름
	UPROPERTY()
	FString LevelPathName;

	// 몬스터 및 페이즈 카운트
	UPROPERTY()
	int32 ConditionValue;
	UPROPERTY()
	int32 PhaseCount;

	// 섹터 설정 여부
	UPROPERTY()
	bool bIsRootSector;
	UPROPERTY()
	bool bInitRootSetting;
	UPROPERTY()
	bool bInitSetting;

	// 게임중 / 완료
	UPROPERTY()
	bool bLevelPlaying;
	UPROPERTY()
	bool bLevelClear;

	// 마지막 섹터
	UPROPERTY()
	bool IsLastSector;

	FTimerHandle ClearTimerHandle;

private:
	// SequenceActor
	UPROPERTY()
	TWeakObjectPtr<ALevelSequenceActor> BossSequenceActor;
	FTimerHandle SequenceTimerHandle;

	// 미션 이벤트 정보
	UPROPERTY()
	ERoguelikeEventType RoomEventType;
	UPROPERTY()
	bool bClearRoomEvent;
	UPROPERTY()
	int32 EventTimeLimt;
	UPROPERTY()
	FName EventSuccessRewardID;
	
	FTimerHandle TimeAttackHandle;

	// Platform 방에서의 이벤트 정보
	FPlatformMissionData PlatformRoomData;
	FTimerHandle PlatformTimerHandle;
};
