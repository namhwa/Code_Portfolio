// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Object/SDKObject.h"
#include "Share/SDKEnum.h"
#include "SDKObjectRewardPortal.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDestoryRewardDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActiveNextRoomPortalDelegate);

class UBoxComponent;
class UBillboardComponent;


/**
 * 
 */
UCLASS()
class ARENA_API ASDKObjectRewardPortal : public ASDKObject
{
	GENERATED_BODY()
	
public:
	ASDKObjectRewardPortal();

	// Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;
	// End AActor Interface

	// Begin ASDKObject Interface
	virtual void InitObject() override;
	virtual void SetActiveObject(bool bActive) override;
	// End AActor Interface

	UFUNCTION(BlueprintCallable)
	ASDKObject* GetRewardActor() const { return RewardObject.Get(); }

	/** 남아있는 아이템 모두 흡수하는 기능 활성화 설정 */
	void SetActiveGetAllItems(bool bActive);

	/** 보상 처리 활성화 */
	UFUNCTION(BlueprintCallable)
	void ActiveRewardPortal();

	/** 보상 스폰 */
	UFUNCTION(BlueprintCallable)
	void SpawnRewardObject(FName RewardObjectID);

	/** 다음 방 포탈 오버랩 알림 */
	UFUNCTION()
	void OnNotifyBeginOverlapNextRoomPortal(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnNotifyEndOverlapNextRoomPortal(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** 선택한 방으로 이동 */
	void MoveToSelectNextRoom();

	UFUNCTION()
	void DestoryReward(AActor* DestroyedActor);

	UFUNCTION(BlueprintImplementableEvent)
	void OnSetNextRoomType(int32 index, FName InType);

	UFUNCTION(BlueprintCallable)
	void SetActiveTutorialNextRoomPortal();

protected:
	void SetActiveNextRoomPortal();

	void SetVisibilityNextRoomPortal(UBoxComponent* InComp, bool bVisible);

public:
	UPROPERTY(BlueprintAssignable)
	FOnDestoryRewardDelegate OnDestoryReward;

	UPROPERTY(BlueprintAssignable)
	FOnActiveNextRoomPortalDelegate OnActiveNextRoomPortal;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent* NextRoomPortal1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent* NextRoomPortal2;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent* NextRoomPortal3;

private:
	UPROPERTY()
	TWeakObjectPtr<ASDKObject> RewardObject;

	UPROPERTY()
	FName LevelType;

	UPROPERTY()
	int32 NextRoomCount;

	bool bActiveGetAllItems;
};
