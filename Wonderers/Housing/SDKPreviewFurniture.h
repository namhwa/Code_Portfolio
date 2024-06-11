// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Object/SDKFurniture.h"

#include "Character/SDKSpringArmComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"

#include "SDKPreviewFurniture.generated.h"


UCLASS()
class ARENA_API ASDKPreviewFurniture : public ASDKFurniture
{
	GENERATED_BODY()
	
public:
	ASDKPreviewFurniture();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void BeginPlay() override;
	
	virtual void InitFurnitureData() override;
	virtual void SetStaticMeshLocation(FVector vLocation) override;
	virtual void SetStaticMeshLocationByType() override;

public:
	/** 미리보기 가구 설정 */
	void SetPreviewFurnitureSetting();

	/** 설정 : Hud 위젯 표시 여부 */
	void SetHudVisibility(bool bHud);

	/** 설정 : 머터리얼 */
	void SetFurnitureMaterialType(bool bPossibleArrange);

	/** 설정 : 회전된 크기 */
	void SetPreviewRotationSize(FVector vNewSize);

	/** 벽장식 설정 : 스프링암 회전 */
	void SetWallHangingForwardRotator(bool bForward);

	/** 수정 그룹 가구 : Transform */
	void SetPreviewGroupTransform(FTransform tTransform);

	/** 타일 설정 : 크기 */
	void SetPreviewTileSize();

	/** 타일 설정 : 회전 */
	void SetPreviewTileRotation();

	/** 타일 설정 : 위치 */
	void SetPreviewTileLocation();

	/** 타일 설정 : 매쉬에 따른 위치 */
	void SetPreviewTileLocationByMesh(FVector vLocation);

	/** 카메라 설정 : 활성화 */
	void SetCameraActive();

	/** 허드 위젯 설정 */
	void SetModifyWidgetSetting();

	/** 표시 설정 : 타일 */
	void SetVisibilityPreviewTile(bool bVisible);

	/** 표시 설정 : 수정 UI */
	void SetVisibilityModifyWidget(bool bVisible);

	/** 표시 설정 : 수정 그룹 UI */
	void SetVisibilityPreviewGroupModifyWidget(bool bVisible);
	
	/** 수정 관련 UI 부착 */
	void AttachHousingModifyWidget();

	/** 적용 : 미리보기 그룹가구 Transform */
	void ApplyPreviewGroupingTransform();

protected:
	// 타일 매쉬 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* TileComponent;

	// 카메라 관련 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USDKSpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* HUDComponent;

	// 머터리얼 관련
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UMaterialInterface* EnableMaterial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UMaterialInterface* DisableMaterial;

private:
	// 그룹핑된 오브젝트 트랜스폼
	UPROPERTY()
	FTransform GroupingTransform;

	// 허드 표시 여부
	UPROPERTY()
	bool HudVisibility;
};
