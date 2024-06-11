// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/SDKObjectArtifact.h"

#include "Kismet/GameplayStatics.h"

#include "Character/SDKHUD.h"
#include "Character/SDKPlayerState.h"
#include "Character/SDKPlayerCharacter.h"
#include "Character/SDKInGameController.h"

#include "Engine/SDKGameInstance.h"
#include "GameMode/SDKRpgState.h"
#include "Manager/SDKTableManager.h"

#include "Object/SDKObjectRewardPortal.h"
#include "Share/SDKHelper.h"
#include "Share/SDKEnum.h"

#include "UI/SDKUIRewardArtifactWidget.h"


void ASDKObjectArtifact::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if(HasAuthority() == true)
	{
		const ASDKPlayerCharacter* PlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
		if (IsValid(PlayerCharacter) == false)
		{
			return;
		}

		ASDKInGameController* IngameController = PlayerCharacter->GetSDKPlayerController();
		if (IsValid(IngameController) == false || IngameController != UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			return;
		}

		ASDKHUD* SDKHUD = IngameController->GetHUD<ASDKHUD>();
		if (IsValid(SDKHUD) == true)
		{
			USDKUIRewardArtifactWidget* RewardArtifactWidget = Cast<USDKUIRewardArtifactWidget>(SDKHUD->OpenUI(EUI::UI_RewardArtifact));
			if (IsValid(RewardArtifactWidget) == true)
			{
				RewardArtifactWidget->SetArtifactIDList(ArtifactList);
				RewardArtifactWidget->OnClosedUIEvent.AddDynamic(this, &ASDKObjectArtifact::SelectRewardArtifact);

				if (FSDKHelpers::IsGlitchTutorialMode(GetWorld()))
				{
					IngameController->ClientToggleNarrationTableID(false);
					IngameController->OpenContentsGuideNotSaveVersion(TEXT("30004"));
				}
			}
		}
	}
}

void ASDKObjectArtifact::NotifyActorEndOverlap(AActor* OtherActor)
{
	if (HasAuthority() == true)
	{
		const ASDKPlayerCharacter* PlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
		if (IsValid(PlayerCharacter) == false)
		{
			return;
		}

		const ASDKInGameController* IngameController = PlayerCharacter->GetSDKPlayerController();
		if (IsValid(IngameController) == false || IngameController != UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			return;
		}
	}
}

void ASDKObjectArtifact::InitObject()
{
	Super::InitObject();

	ASDKRpgState* SDKRpgState = GetWorld()->GetGameState<ASDKRpgState>();
	if (IsValid(SDKRpgState) == true)
	{
		FVector2D Ability = FVector2D::ZeroVector;
		int32 ArtifactCount = 0;

		FS_GlobalDefine* GDTable = USDKTableManager::Get()->FindTableGlobalDefine(EGlobalDefine::RogueLikeRelicDefault);
		if (nullptr != GDTable && GDTable->Value.IsValidIndex(0) == true)
		{
			ArtifactCount = FCString::Atoi(*GDTable->Value[0]);
		}
		else
		{
			ArtifactCount = 1;
		}

		Ability = SDKRpgState->GetPlayerAttAbility(EParameter::ArtifactSlotCount);
		ArtifactCount += Ability.X;
		ArtifactCount += ArtifactCount * Ability.Y;

		// 한 슬롯은 보유 중인 세트 중 한 개로 설정되도록
		int32 RandomChance = FMath::RandRange(0, ArtifactCount - 1);
		for (int32 i = 0; i < ArtifactCount; ++i)
		{
			FS_Drop* DropTable = USDKTableManager::Get()->FindTableRow<FS_Drop>(ETableType::tb_Drop, FString::FromInt(DropAritfactID));
			if (DropTable != nullptr)
			{
				for (int32 j = 0; j < DropTable->RepeatCount; j++)
				{
					int32 DropItemID = FSDKHelpers::GetDropItemID(GetWorld(), DropAritfactID, i == RandomChance);
					SDKRpgState->RemoveDropReadyArtifact(DropItemID);

					if(bSpawnUpgradedArtifact == true)
					{
						FS_Item* ItemTable = USDKTableManager::Get()->FindTableRow<FS_Item>(ETableType::tb_Item, FString::FromInt(DropItemID));
						if (ItemTable != nullptr)
						{
							if (ItemTable->Grade < EItemGrade::Legendary)
							{
								DropItemID++;
							}
						}
					}

					ArtifactList.Add(DropItemID);
				}
			}
			else
			{
				UE_LOG(LogGame, Warning, TEXT("Not Found Drop Table ID : %d"), DropAritfactID);
			}
		}
	}
}

void ASDKObjectArtifact::SelectRewardArtifact()
{
	SetActiveObject(false);

	SetLifeSpan(0.01f);

	ASDKObjectRewardPortal* SDKRewardPortal = GetOwner<ASDKObjectRewardPortal>();
	if (IsValid(SDKRewardPortal) == true)
	{
		FTimerHandle RewardHandle;
		GetWorld()->GetTimerManager().SetTimer(RewardHandle, SDKRewardPortal, &ASDKObjectRewardPortal::ActiveRewardPortal, 2.f, false);
	}
}
