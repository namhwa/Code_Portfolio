// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/SDKRpgState.h"
#include "SDKRpgTutorialState.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API ASDKRpgTutorialState : public ASDKRpgState
{
	GENERATED_BODY()
	
public:
	// Begin SDKRpgState Interface
	virtual void ClearSectorPhase() override;
	// End SDKRpgState Interface
};
