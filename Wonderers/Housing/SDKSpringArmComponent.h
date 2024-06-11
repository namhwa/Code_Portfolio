// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"

#include "Share/SDKEnum.h"

#include "SDKSpringArmComponent.generated.h"


UCLASS( ClassGroup = (Object), meta = (BlueprintSpawnableComponent) )
class ARENA_API USDKSpringArmComponent : public USpringArmComponent
{
	GENERATED_UCLASS_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime) override;

	void CheckCameraLagMinDistance(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void SetCameraType(EFurnitureType eType);

	void SetSpringArmLagSpeedByType();
	void SetSpringArmRotationByType();

	UFUNCTION(BlueprintCallable)
	void SetWallTypesForwardRotator(bool bForward);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Lag, meta = (editcondition = "bEnableCameraLag", ClampMin = "0.0", UIMin = "0.0"))
	float CameraLagMinDistance;

	UPROPERTY(EditAnywhere, Category = Lag, meta = (editcondition = "bEnableCameraLag"))
	float WallTypeLagSpeed;

	UPROPERTY(EditAnywhere, Category = Lag, meta = (editcondition = "bEnableCameraLag"))
	float GenerateTypeLagSpeed;

	UPROPERTY(EditAnywhere, Category=Camera)
	bool bUseFurnitrueType;

	UPROPERTY(EditAnywhere, Category=Camera, meta = (editcondition = "bUseFurnitrueType"))
	EFurnitureType CameraType;

private:
	UPROPERTY()
	FRotator CameraTypeRotator;
};
