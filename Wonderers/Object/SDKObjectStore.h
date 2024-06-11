// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Object/SDKObject.h"
#include "SDKObjectStore.generated.h"


UCLASS()
class ARENA_API ASDKObjectStore : public ASDKObject
{
	GENERATED_BODY()
	
public:
	ASDKObjectStore();

	// Begin Actor Interface
	virtual void BeginPlay() override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;
	// End Actor Interface

	// Begin SDKObject Interface
	virtual void InitObject() override;
	virtual void UseObject(AActor* UseActor) override;
	// End SDKObject Interface

	/** 오브젝트 리프레시 타입 반환*/
	ERPGRefreshType GetObjectRefreshType() { return RefreshType; }

	/** 상점 아이템 ID 설정, bUpgrade : 업그레이드 유물 체크용, */
	void SetStoreItemID(int32 ItemID, bool bIsBlackMarket = false, bool bDarkArtifact = false);
	UFUNCTION(BlueprintCallable)
	int32 GetStoreItemID() { return StoreItemID; }

	UFUNCTION(BlueprintCallable)
	int32 GetRefreshCount() { return StoreRefreshCount; }

	UFUNCTION(BlueprintCallable)
	void AddRefreshCount() { StoreRefreshCount++; }

	/** 특성으로 사용할 수 있는 상점 슬롯인지 반환 */
	bool GetIsAttributeSlotType() const { return bIsAttributeShopSlot; }

	void RespawnStoreDropItem();

	UFUNCTION(NetMulticast, Reliable)
	void MultiSetRefreshStoreItem(bool bActive);

	UFUNCTION(BlueprintImplementableEvent)
	void OnUpdateRefreshCostWidget();

	/** 주사위 리프레시 관련 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnSpawnDice();

	int32 GetDiceRefreshCost() { return DiceRefreshCost; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SDKObjectShop")
	bool bIsAttributeShopSlot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SDKObjectShop|Refresh")
	int32 DiceRefreshCost;

private:
	UPROPERTY(EditDefaultsOnly, Category = "SDKObjectShop|Refresh")
	ERPGRefreshType RefreshType;

private:
	/** 오브젝트의 아이템 아이디 */
	UPROPERTY()
	int32 StoreRefreshCount;

	UPROPERTY()
	int32 StoreItemID;

	UPROPERTY()
	int32 DropStoreID;
};
