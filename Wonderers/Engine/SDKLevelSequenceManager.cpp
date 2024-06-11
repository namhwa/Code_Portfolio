// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SDKLevelSequenceManager.h"

#include "Kismet/GameplayStatics.h"
#include "LevelSequence/Public/LevelSequenceActor.h"
#include "LevelSequence/Public/LevelSequencePlayer.h"

#include "Share/SDKHelper.h"

#include "Character/SDKPlayerController.h"
#include "Character/SDKPlayerCharacter.h"

#include "Engine/SDKLevelSequenceActor.h"
#include "Subsystem/SDKGameInstanceSubsystem.h"

DEFINE_LOG_CATEGORY(LogSequence)


bool USDKLevelSequenceManager::IsValidPlay()
{
	return IsValid(CurrentSequenceActor) && IsValid(CurrentSequenceActor->GetSequencePlayer());
}

void USDKLevelSequenceManager::SetLevelSequence(ALevelSequenceActor* InSequenceActor)
{
	if (IsValid(InSequenceActor))
	{
		if (IsValid(CurrentSequenceActor) && CurrentSequenceActor->GetSequencePlayer()->IsPlaying())
		{
			CurrentSequenceActor->GetSequencePlayer()->Stop();
		}

		CurrentSequenceActor = InSequenceActor;
	}
}

void USDKLevelSequenceManager::PlayLevelSequence(bool bAllActorHidden /*= false*/)
{
	if (IsValid(CurrentSequenceActor) == false || IsValid(CurrentSequenceActor->GetSequencePlayer()) == false|| IsValid(CurrentSequenceActor->GetSequencePlayer()->GetSequence()) == false)
	{
		UE_LOG(LogSequence, Log, TEXT("[Level Sequenece Manager] CurrentSequenceActor is NULL."));
		return;
	}

	EGameMode CurrentGameMode = FSDKHelpers::GetGameMode(GetWorld());
	bool bCheckMode = CurrentGameMode == EGameMode::Rpg || CurrentGameMode == EGameMode::RpgTutorial;
	ASDKPlayerCharacter* SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (bCheckMode && (!IsValid(SDKPlayerCharacter) || SDKPlayerCharacter->IsDie()))
	{
		return;
	}

	if(bAllActorHidden)
	{
		SDKPlayerCharacter->SetCinematicPlaying(true);
	}

	ASDKLevelSequenceActor* SDKSequenceActor = Cast<ASDKLevelSequenceActor>(CurrentSequenceActor);
	if (IsValid(SDKSequenceActor))
	{
		IsPlaying = SDKSequenceActor->PlayLevelSequence();

		if (SDKSequenceActor->GetSequencePlayer()->OnFinished.Contains(this, FName("FinishLevelSequence")) == false)
		{
			SDKSequenceActor->GetSequencePlayer()->OnFinished.AddDynamic(this, &USDKLevelSequenceManager::FinishLevelSequence);
		}

		if (SDKSequenceActor->GetSequencePlayer()->OnStop.Contains(this, FName("FinishLevelSequence")) == false)
		{
			SDKSequenceActor->GetSequencePlayer()->OnStop.AddDynamic(this, &USDKLevelSequenceManager::FinishLevelSequence);
		}
	}
	else
	{
		CurrentSequenceActor->GetSequencePlayer()->Play();

		if (CurrentSequenceActor->GetSequencePlayer()->OnFinished.Contains(this, FName("FinishLevelSequence")) == false)
		{
			CurrentSequenceActor->GetSequencePlayer()->OnFinished.AddDynamic(this, &USDKLevelSequenceManager::FinishLevelSequence);
		}

		if (CurrentSequenceActor->GetSequencePlayer()->OnStop.Contains(this, FName("FinishLevelSequence")) == false)
		{
			CurrentSequenceActor->GetSequencePlayer()->OnStop.AddDynamic(this, &USDKLevelSequenceManager::FinishLevelSequence);
		}
	}
}

void USDKLevelSequenceManager::FinishLevelSequence()
{
	if (IsValid(CurrentSequenceActor) == false)
	{
		UE_LOG(LogSequence, Log, TEXT("[Level Sequenece Manager] CurrentSequenceActor is NULL."));
		return;
	}

	if (CurrentSequenceActor->GetSequencePlayer()->IsPlaying() == true)
	{
		CurrentSequenceActor->GetSequencePlayer()->GoToEndAndStop();
	}

	ASDKPlayerCharacter* SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (IsValid(SDKPlayerCharacter))
	{
		SDKPlayerCharacter->SetCinematicPlaying(false);
	}

	if (OnFinishedSequence.IsBound())
	{
		OnFinishedSequence.Broadcast();
	}

	ResetCurrentLevelSequence();
}

void USDKLevelSequenceManager::ResetCurrentLevelSequence()
{
	if (OnFinishedSequence.IsBound())
	{
		OnFinishedSequence.Clear();
	}

	if (IsValid(CurrentSequenceActor))
	{
		if (CurrentSequenceActor->GetSequencePlayer()->OnFinished.Contains(this, FName("FinishLevelSequence")))
		{
			CurrentSequenceActor->GetSequencePlayer()->OnFinished.RemoveDynamic(this, &USDKLevelSequenceManager::FinishLevelSequence);
		}

		if (CurrentSequenceActor->GetSequencePlayer()->OnStop.Contains(this, FName("FinishLevelSequence")))
		{
			CurrentSequenceActor->GetSequencePlayer()->OnStop.RemoveDynamic(this, &USDKLevelSequenceManager::FinishLevelSequence);
		}

		CurrentSequenceActor = nullptr;
	}
}

USDKLevelSequenceManager* USDKLevelSequenceManager::Get()
{
	return USDKGameInstanceSubsystem::Get()->GetLevelSequenceManager();
}
