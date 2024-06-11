// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PainCausingVolume.h"
#include "SDKObjectDeadZone.generated.h"

class USDKSkillDirector;

UCLASS()
class ARENA_API ASDKObjectDeadZone : public APainCausingVolume
{
	GENERATED_BODY()
	
public:
	ASDKObjectDeadZone();

	// Begin APainCausingVolume Interface
	virtual void ActorEnteredVolume(class AActor* Other) override;
	virtual void PainTimer() override;
	virtual void CausePainTo(class AActor* Other) override;
	// End APainCausingVolume Interface
	
	// 오버렙 엑터 반환
	TArray<AActor*> GetOverlapActiveActors();

	// 데드존 초기화
	void InitDeadZone();

	// 데드존 타입 및 인덱스 설정
	void SetDeadZoneIndex(int32 Index);

	// 데드존 및 메테오 구역 활성화
	void ActiveDaedZone();
	void ActiveMeteor();

	// 데드존 및 메테오 비활성화
	void DeActiveDeadZone();
	void DeActiveMeteorZone();

	// 메테오 인터벌 타임 설정
	void SetMeteorIntervalTime(float NewTime);

	// 메테오 드랍
	void CausePainMeteor();

	// 메테오 드랍 위치
	void SetMeteorLocations();

	// 메테오 스폰
	void SpawnMeteor();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = DeadZone)
	USDKSkillDirector* SkillDirector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = DeadZone)
	float DefaultDamage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DeadZone)
	int32 MaxMeteorCount;

private:
	FString ProjectileID;

	UPROPERTY()
	FVector vMeteorLocations;

	UPROPERTY()
	float MaxHeight;

	UPROPERTY()
	float MeteorInterval;

	UPROPERTY()
	int32 DeadZoneIndex;

	UPROPERTY()
	int32 MeteorCount;

	UPROPERTY()
	int32 LocationIndex;

	UPROPERTY()
	bool bIsActive;

	// 메테오 떨어지는 타이머 핸들 : 위치 지정, 생성
	FTimerHandle TimerHadle_LocationInterval;
	FTimerHandle TimerHandle_DropInterval;
};
