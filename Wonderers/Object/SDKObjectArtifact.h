// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Object/SDKObject.h"
#include "SDKObjectArtifact.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API ASDKObjectArtifact : public ASDKObject
{
	GENERATED_BODY()
	
public:
	// Begin AActor Interface
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;
	// End AActor Interface

	// Begin ASDKObject Interface
	virtual void InitObject() override;
	// End AActor Interface

	const TArray<int32> GetAritfactIDArray() { return ArtifactList; }

	UFUNCTION()
	void SelectRewardArtifact();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 DropAritfactID;

	UPROPERTY(EditAnywhere)
	bool bSpawnUpgradedArtifact;

private:
	UPROPERTY()
	TArray<int32> ArtifactList;
};
