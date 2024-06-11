// Fill out your copyright notice in the Description page of Project Settings.

#include "Housing/SDKLobbyMode.h"

#include "Kismet/GameplayStatics.h"

#include "GameMode/SDKLobbyState.h"

#include "Character/SDKMyInfo.h"
#include "Character/SDKPlayerState.h"
#include "Character/SDKLobbyController.h"

#include "Share/SDKHelper.h"
#include "Engine/SDKGameInstance.h"
#include "Manager/SDKTableManager.h"
#include "Manager/SDKLobbyServerManager.h"


ASDKLobbyMode::ASDKLobbyMode()
{
	PlayerControllerClass = ASDKLobbyController::StaticClass();
	GameStateClass = ASDKLobbyState::StaticClass();
}

void ASDKLobbyMode::InitGameState()
{
	Super::InitGameState();

	ASDKLobbyState* SDKLobbyState = Cast<ASDKLobbyState>(GameState);
	if(SDKLobbyState != nullptr)
	{
		SDKLobbyState->SetGameMode(EGameMode::Lobby);
	}

	g_pMyInfo->SetCurrentGameMode(EGameMode::Lobby);
}

void ASDKLobbyMode::BeginPlay()
{
	Super::BeginPlay();
	
	//UE_LOG(LogGame, Log, TEXT("Lobby Mode BeginPlay"));
	ServerReadyComplete();
}

void ASDKLobbyMode::ServerReadyComplete()
{
	Super::ServerReadyComplete();

	for(auto iter = GetWorld()->GetPlayerControllerIterator(); iter; ++iter)
	{
		ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(*iter);
		if(LobbyController == nullptr)
		{
			continue;
		}

		LobbyController->ClientRequestData();
	}

	// 게임 시작
	ASDKGameState* SDKGameState = Cast<ASDKGameState>(GameState);
	if (SDKGameState != nullptr)
	{
		SDKGameState->OnGameStart();
	}
}

void ASDKLobbyMode::SetPlayerData(const FServerSendData& SendData)
{
	Super::SetPlayerData(SendData);

	ASDKLobbyController* SDKLobbyController = Cast<ASDKLobbyController>(SendData.NewPlayer);
	if(SDKLobbyController != nullptr)
	{
		// 플레이어 시작 위치 선별
		AActor* const StartSpot = FindPlayerStartCustom(SendData.NewPlayer);
		if(StartSpot != nullptr)
		{
			// 영웅 스폰
			SDKLobbyController->ServerSpawnHero(SendData.SelectHeroID, SendData.SelectSkinID, StartSpot->GetActorTransform());
		}
	}
}

void ASDKLobbyMode::LoadMyroomData()
{
	if(UGameplayStatics::GetCurrentLevelName(GetWorld()) == TEXT("Myroom"))
	{
		auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		if (LobbyController != nullptr)
		{
			LobbyController->InitMyroomBuildingData();
			LobbyController->InitMyroomTilesData(g_pMyInfo->GetHousingData().GetHousingLevel());

			if (g_pMyInfo->GetHousingData().GetArrangedAllItemCount() <= 0)
			{
				// TODO : 세이브 정보가 없을 경우 임시로 프리셋 적용
				LobbyController->ApplayMyroomPresetData(USDKTableManager::Get()->GetRowNameMyroomPreset()[0].ToString());
			}

			LobbyController->LoadMyroomHousingData();
		}
	}
}