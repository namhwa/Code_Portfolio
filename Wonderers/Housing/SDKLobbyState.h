// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/SDKGameState.h"
#include "SDKLobbyState.generated.h"


UCLASS()
class ARENA_API ASDKLobbyState : public ASDKGameState
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
};
