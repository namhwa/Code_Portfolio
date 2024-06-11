// Fill out your copyright notice in the Description page of Project Settings.


#include "Engine/SDKLevelSequenceActor.h"

#include "Kismet/GameplayStatics.h"

#include "Character/SDKInGameController.h"
#include "Character/SDKPlayerCharacter.h"
#include "Character/SDKSkillDirector.h"
#include "Character/SDKProjectile.h"
#include "Engine/SDKGameTask.h"


ASDKLevelSequenceActor::ASDKLevelSequenceActor(const FObjectInitializer& Init)
	: Super(Init)
	, bHideUI(true)
	, bHideWeapon(true)
	, bHideArrow(true)
	, bHidePet(true)
	, bDisableInput(true)
	, bInvincible(true)
	, bRemoveProjectile(true)
{
}

void ASDKLevelSequenceActor::BeginPlay()
{
	// 시퀀스 플레이어 바인딩
	if (SequencePlayer->OnStop.Contains(this, FName("StopLevelSequence")) == false)
	{
		SequencePlayer->OnStop.AddDynamic(this, &ASDKLevelSequenceActor::StopLevelSequence);
	}

	if (SequencePlayer->OnFinished.Contains(this, FName("StopLevelSequence")) == false)
	{
		SequencePlayer->OnFinished.AddDynamic(this, &ASDKLevelSequenceActor::StopLevelSequence);
	}

	if (PlaybackSettings.bAutoPlay)
	{
		UMovieSceneSequenceTickManager* TickManager = SequencePlayer->GetTickManager();
		if (ensure(TickManager))
		{
			TickManager->RegisterSequenceActor(this);
		}

		AActor::BeginPlay();

		RefreshBurnIn();

		PlayLevelSequence();
	}
	else
	{
		Super::BeginPlay();
	}
}

bool ASDKLevelSequenceActor::PlayLevelSequence()
{
	if (SequencePlayer->IsPlaying())
	{
		SequencePlayer->Stop();
	}

	SequencePlayer->Play();

	TaskToNextTick(this, [this]
	{
		AfterSequencePlay();
	});

	return true;
}

void ASDKLevelSequenceActor::AfterSequencePlay()
{
	ASDKPlayerController* SDKPlayerController = Cast<ASDKPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKPlayerController))
	{
		if (bHideUI)
		{
			SDKPlayerController->ClientToggleWidgetBySequence(false);
		}

		if (bHidePet)
		{
			SDKPlayerController->ClientSetVisiblePet(false);
		}

		ASDKPlayerCharacter* SDKPlayerCharacter = SDKPlayerController->GetPawn<ASDKPlayerCharacter>();
		if (IsValid(SDKPlayerCharacter))
		{
			if (SDKPlayerCharacter->GetCurrentState() == ECharacterState::Skill)
			{
				SDKPlayerCharacter->EndSkill();
			}

			if (bHideWeapon)
			{
				SDKPlayerCharacter->ClientSetVisibilityShowWeapon(false);
			}

			if (bHideArrow)
			{
				SDKPlayerCharacter->ClientSetVisibilityArrow(false);
			}

			if (bDisableInput)
			{
				SDKPlayerCharacter->ClientInputModeChange(false);
			}

			if (bInvincible)
			{
				USDKSkillDirector* SkillDirector = SDKPlayerCharacter->GetSkillDirector();
				if (IsValid(SkillDirector))
				{
					// 무적 버프
					SkillDirector->UseSkill(1367, false);
				}
			}
		}
	}

	if (bRemoveProjectile)
	{
		TArray<AActor*> Projectiles;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASDKProjectile::StaticClass(), Projectiles);

		for (int32 Projectile = 0; Projectile < Projectiles.Num(); Projectile++)
		{
			if (!IsValid(Projectiles[Projectile]))
			{
				continue;
			}
			Projectiles[Projectile]->Destroy();
		}
	}

	if (!InvisibleActorTag.IsEmpty())
	{
		TArray<AActor*> InvisibleActors;
		UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), AActor::StaticClass(), FName(InvisibleActorTag), InvisibleActors);
		for (int32 Invisible = 0; Invisible < InvisibleActors.Num(); Invisible++)
		{
			InvisibleActors[Invisible]->SetActorHiddenInGame(true);
		}
	}

	if (!DeleteActorTag.IsEmpty())
	{
		TArray<AActor*> DestroyActors;
		UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), AActor::StaticClass(), FName(DeleteActorTag), DestroyActors);
		for (int32 Destroy = 0; Destroy < DestroyActors.Num(); Destroy++)
		{
			DestroyActors[Destroy]->Destroy();
		}
	}
}

void ASDKLevelSequenceActor::StopLevelSequence()
{
	ASDKPlayerCharacter* SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (IsValid(SDKPlayerCharacter))
	{
		if (bHideWeapon)
		{
			SDKPlayerCharacter->ClientSetVisibilityShowWeapon(true);
		}

		if (bHideArrow)
		{
			SDKPlayerCharacter->ClientSetVisibilityArrow(true);
		}

		if (bDisableInput)
		{
			SDKPlayerCharacter->ClientInputModeChange(true);
		}
	}

	ASDKPlayerController* SDKPlayerController = Cast<ASDKPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKPlayerController))
	{
		if (bHideUI)
		{
			SDKPlayerController->ClientToggleWidgetBySequence(true);
		}

		if (bHidePet)
		{
			SDKPlayerController->ClientSetVisiblePet(true);
		}
	}

	ASDKInGameController* SDKIngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKIngameController))
	{
		if (bInvincible)
		{
			TArray<int32> SkillIDs;
			SkillIDs.AddUnique(1367);
			USDKPawnBuffComponent* PawnBuffComp = SDKIngameController->GetPawnBuffComponent();
			if (IsValid(PawnBuffComp))
			{
				PawnBuffComp->RemoveBuffAtSkillID(SkillIDs);
			}
		}
	}

	if (!InvisibleActorTag.IsEmpty())
	{
		TArray<AActor*> InvisibleActors;
		UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), AActor::StaticClass(), FName(InvisibleActorTag), InvisibleActors);
		for (int32 Invisible = 0; Invisible < InvisibleActors.Num(); Invisible++)
		{
			InvisibleActors[Invisible]->SetActorHiddenInGame(false);
		}
	}
}
