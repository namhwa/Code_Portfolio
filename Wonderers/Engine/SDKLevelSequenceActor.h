// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LevelSequenceActor.h"
#include "SDKLevelSequenceActor.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API ASDKLevelSequenceActor : public ALevelSequenceActor
{
	GENERATED_BODY()
	
public:
	ASDKLevelSequenceActor(const FObjectInitializer& Init);

	// Begin AActor interface
	virtual void BeginPlay() override;
	// End AActor interface

	UFUNCTION(BlueprintCallable)
	bool PlayLevelSequence();

protected:
	UFUNCTION()
	void AfterSequencePlay();

	UFUNCTION()
	void StopLevelSequence();

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sequence")
	bool bHideUI;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sequence")
	bool bHideWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sequence")
	bool bHideArrow;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sequence")
	bool bHidePet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sequence")
	bool bDisableInput;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sequence")
	bool bInvincible;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sequence")
	bool bRemoveProjectile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sequence")
	FString InvisibleActorTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sequence")
	FString DeleteActorTag;
};
