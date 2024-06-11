// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GameMode/SDKRoguelikeSector.h"
#include "Share/SDKStruct.h"

#include "SDKRoguelikeManager.generated.h"

struct FRpgSectorData;


UCLASS()
class ARENA_API ASDKRoguelikeManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ASDKRoguelikeManager();

	// Begin Actor interface
	virtual void BeginPlay() override;
	// End Actor interface

	int32 GetCurrentIndex() const { return CurrentIndex; }

	UFUNCTION(BlueprintCallable)
	int32 GetSectorCount() const { return SectorList.Num(); }

	bool GetIsNonePhaseSector() const;
	
	UFUNCTION(BlueprintCallable)
	ASDKRoguelikeSector* GetCurrentSector() const { return CurrentSector; }
	ASDKRoguelikeSector* GetRoguelikeSector(int32 Index) const;

	UFUNCTION(BlueprintCallable)
	void SetCurrentSector(ASDKRoguelikeSector* pSector);

	UFUNCTION(BlueprintCallable)
	void AddRoguelikeSector(ASDKRoguelikeSector* newSector, FRpgSectorData Data);
	
	void ClearRoguelikeSectorList();

private:
	UPROPERTY()
	TArray<ASDKRoguelikeSector*> SectorList;

	UPROPERTY()
	ASDKRoguelikeSector* CurrentSector;

	UPROPERTY()
	int32 CurrentIndex;
};
