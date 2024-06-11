// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/SDKGameMode.h"
#include "Object/SDKFurniture.h"
#include "SDKLobbyMode.generated.h"

UCLASS()
class ARENA_API ASDKLobbyMode : public ASDKGameMode
{
	GENERATED_BODY()
	
public:
	ASDKLobbyMode();

	// Begin GameMode Interface
	virtual void InitGameState() override;
	virtual void BeginPlay() override;
	// End GameMode Interface

	// Begin SDKGameMode Interface
	virtual void ServerReadyComplete() override;
	virtual void SetPlayerData(const FServerSendData& SendData) override;
	// End SDKGameMode Interface

	/** 마이룸 정보 불러오기 */
	void LoadMyroomData();
};
