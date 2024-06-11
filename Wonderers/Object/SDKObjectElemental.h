// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Object/SDKObject.h"
#include "SDKObjectElemental.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API ASDKObjectElemental : public ASDKObject
{
	GENERATED_BODY()
	
public:
	// Begin AActor Interface
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;
	// End AActor Interface
	
	UFUNCTION()
	void SelectRewardElemental();

protected:
	/** 무기 설정 */
	void SetWeaponElemental(const TArray<int32>& InWeapons);

	/** 무기 타입에 맞는 속성 ID 정리 */
	void InitWeaponElementalIDList(EItemCategory InCategory);

	/** 무기 등급 설정 */
	void SetWeaponGrade();
	void SetSpecialWeaponUpgrade();

protected:
	UPROPERTY(EditAnywhere)
	bool bUpgradeReward;

	UPROPERTY()
	TMap<EElementalType, int32> mapElementalWeaponID;

	UPROPERTY()
	TMap<EElementalType, EItemGrade> mapHaveElementals;
};
