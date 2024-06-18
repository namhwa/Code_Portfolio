// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Share/SDKEnum.h"

#include "SDKFurniture.generated.h"

class UParticleSystemComponent;
class UStaticMeshComponent;
class UBoxComponent;

UCLASS()
class ARENA_API ASDKFurniture : public AActor
{
	GENERATED_BODY()

public:
	ASDKFurniture();

	// Begin Actor Interface
	virtual void BeginPlay() override;
	virtual void NotifyActorOnReleased(FKey ButtonReleased = EKeys::LeftMouseButton) override;
	virtual void NotifyActorOnInputTouchEnd(const ETouchIndex::Type FingerIndex) override;
	// End Actor Interface

	/** 가구 정보 초기화 */
	virtual void InitFurnitureData();

	/** 예시 가구인가 반환 */
	bool GetIsPreviewActor() { return IsPreviewActor; }

	/** 베이스 가구의 ID 반환 */
	FString GetBaseTableID() const { return BaseTableID; }

	/** 가구 유니크 아이디 */
	FString GetUniqueID() const { return UniqueID; }
	void SetUniqueID(FString newUniqueID);

	/** 가구 테이블 아이디 */
	FString GetFurnitureID() const { return FurnitureID; }
	void SetFurnitureID(FString NewTableID);

	/** 가구 타입 */
	EFurnitureType GetFurnitureType() const { return FurnitureType; }
	void SetFurnitureType(EFurnitureType NewType);

	/** 배치된 방 인덱스 */
	UFUNCTION(BlueprintCallable)
	int32 GetArrangedRoomIndex() const { return RoomIndex; }
	void SetArrangedRoomIndex(int32 newIndex);
	
	/** 가구 크기 */
	FVector GetFurnitureSize() const { return Size; }
	void SetFurnitureSize(FVector vNewSize);

	/** 배치된 시작 인덱스 */
	FVector GetSavedIndex() const { return SavedIndex; }
	void SetSavedIndex(FVector vNewIndex);

	/** 매쉬 위치 */
	FVector GetStaticMeshLocation() const;
	virtual void SetStaticMeshLocation(FVector vLocation);

	/** 가구 매쉬 : 스케일값 */
	FVector GetStaticMeshScale() const;
	void SetStaticMeshScale(FVector vScale);

	/** 매쉬 설정 : 매쉬*/
	void SetStaticMesh(UStaticMesh* NewMesh);
	
	/** 매쉬 설정 : 머터리얼 */
	void SetStaticMeshMaterial(UMaterialInterface* NewMaterial);

	/** 매쉬 설정 : 타입에 따른 위치 */
	virtual void SetStaticMeshLocationByType();

	/** 콜리전 설정 : 크기 */
	void SetCollisionSize(FVector vNewSize);

	/** 콜리전 설정 : 활성화 */
	void SetCollisionEnabled(bool bEnable);

	/** 설정 : 그림자 처리 */
	void SetFurnitureShadow();

	/** 설정 : 배치 이펙트 */
	void SetFurnitureArrangeEffect();

public:
	FORCEINLINE class UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ShadowMeshComponent;
	
	UPROPERTY()
	UParticleSystemComponent* SpawnEffectComponent;

protected:
	// 클라에서 지정한 유니크 아이디
	FString UniqueID;
	
	// 가구 테이블 아이디
	FString FurnitureID;
	
	// 콜리전 크기
	FVector CollsionSize;

	// 가구 크기
	UPROPERTY()
	FVector Size;

	// 저장된 오브젝트인 경우 시작 인덱스
	UPROPERTY()
	FVector SavedIndex;

	// 베이스 테이블 아이디
	UPROPERTY(EditAnywhere, Category = "Furniture", meta = (AllowPrivateAccess = "true"))
	FString BaseTableID;

	// 배치된 방 인덱스
	UPROPERTY(EditAnywhere, Category = "Furniture", meta = (AllowPrivateAccess = "true"))
	int32 RoomIndex;

	// 가구 타입
	UPROPERTY(EditAnywhere, Category = "Furniture", meta = (AllowPrivateAccess = "true"))
	EFurnitureType FurnitureType;

	// 샘플 액터 여부
	UPROPERTY()
	bool IsPreviewActor;
};
