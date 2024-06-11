// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/SDKObjectElemental.h"

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

#include "UI/SDKUIRewardWeaponWidget.h"


void ASDKObjectElemental::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if (HasAuthority() == true)
	{
		ASDKPlayerCharacter* PlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
		if (IsValid(PlayerCharacter) == false)
		{
			return;
		}

		ASDKInGameController* IngameController = PlayerCharacter->GetSDKPlayerController();
		if (IsValid(IngameController) == false || IngameController != UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			return;
		}

		TArray<int32> WeaponIDs;
		TArray<FItemSlotData> InvenWeapon = IngameController->GetAllInventoryItem(EItemType::Weapon);
		if (InvenWeapon.Num() > 0)
		{
			for (auto& iter : InvenWeapon)
			{
				WeaponIDs.AddUnique(iter.TableID);
			}
		}

		SetWeaponElemental(WeaponIDs);
	}
}

void ASDKObjectElemental::NotifyActorEndOverlap(AActor* OtherActor)
{
	if (HasAuthority() == true)
	{
		const ASDKPlayerCharacter* SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
		if (IsValid(SDKPlayerCharacter) == false)
		{
			return;
		}

		const ASDKInGameController* SDKIngameController = SDKPlayerCharacter->GetSDKPlayerController();
		if (IsValid(SDKIngameController) == false)
		{
			return;
		}
	}
}

void ASDKObjectElemental::SelectRewardElemental()
{
	SetActiveObject(false);

	SetLifeSpan(0.01f);

	// 튜토리얼일 때는 수동으로 발판이 나오도록
	if (FSDKHelpers::IsGlitchTutorialMode(GetWorld()))
	{
		ASDKRpgState* SDKRpgState = GetWorld()->GetGameState<ASDKRpgState>();
		if (IsValid(SDKRpgState) && SDKRpgState->GetCurrentMapType() == FName("Start"))
		{
			return;
		}
	}

	ASDKObjectRewardPortal* SDKRewardPortal = GetOwner<ASDKObjectRewardPortal>();
	if (IsValid(SDKRewardPortal) == true)
	{
		FTimerHandle RewardHandle;
		GetWorld()->GetTimerManager().SetTimer(RewardHandle, SDKRewardPortal, &ASDKObjectRewardPortal::ActiveRewardPortal, 2.f, false);
	}
}

void ASDKObjectElemental::SetWeaponElemental(const TArray<int32>& InWeapons)
{
	if (InWeapons.Num() <= 0)
	{
		SelectRewardElemental();
		return;
	}

	EItemCategory CategoryType = EItemCategory::None;

	for (auto& itID : InWeapons)
	{
		if (itID <= 0)
		{
			continue;
		}

		FS_Item* ItemTable = USDKTableManager::Get()->FindTableRow<FS_Item>(ETableType::tb_Item, FString::FromInt(itID));
		if (ItemTable != nullptr)
		{
			CategoryType = ItemTable->Category;

			if (ItemTable->Elemental != EElementalType::None && mapHaveElementals.Contains(ItemTable->Elemental) == false)
			{
				mapHaveElementals.Add(ItemTable->Elemental, ItemTable->Grade);
			}
		}
	}

	InitWeaponElementalIDList(CategoryType);

	SetWeaponGrade();
	SetSpecialWeaponUpgrade();

	TArray<int32> WeaponIDs;
	mapElementalWeaponID.GenerateValueArray(WeaponIDs);

	ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
	if (IsValid(SDKHUD))
	{
		USDKUIRewardWeaponWidget* RewardWeaponWidget = Cast<USDKUIRewardWeaponWidget>(SDKHUD->OpenUI(EUI::UI_RewardWeapon));
		if (IsValid(RewardWeaponWidget))
		{
			RewardWeaponWidget->SetWeaponElemntalList(WeaponIDs);
			RewardWeaponWidget->OnClosedUIEvent.AddDynamic(this, &ASDKObjectElemental::SelectRewardElemental);

			ASDKInGameController* IngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
			if (IsValid(IngameController))
			{
				if (FSDKHelpers::IsGlitchTutorialMode(GetWorld()))
				{
					IngameController->OpenContentsGuide(TEXT("30002"));
				}
				else
				{
					IngameController->OpenContentsGuideNotSaveVersion(TEXT("30002"));
				}
			}
		}
	}
}

void ASDKObjectElemental::InitWeaponElementalIDList(EItemCategory InCategory)
{
	TArray<FString> WeaponIDs = USDKTableManager::Get()->GetWeaponTypeIDs(EItemType::Weapon, InCategory);
	if (WeaponIDs.Num() > 0)
	{
		for (auto& itID : WeaponIDs)
		{
			FS_Item* ItemTable = USDKTableManager::Get()->FindTableRow<FS_Item>(ETableType::tb_Item, itID);
			if (ItemTable != nullptr)
			{
				if (ItemTable->Grade > EItemGrade::Normal || ItemTable->Elemental == EElementalType::None)
				{
					continue;
				}

				if (mapElementalWeaponID.Contains(ItemTable->Elemental) == false)
				{
					if (mapHaveElementals.Contains(ItemTable->Elemental) && mapHaveElementals[ItemTable->Elemental] >= EItemGrade::Legendary)
					{
						continue;
					}

					mapElementalWeaponID.Add(ItemTable->Elemental, FCString::Atoi(*itID));
				}
			}
		}
	}

	TArray<EElementalType> TypeList;
	for (int32 i = static_cast<int32>(EElementalType::Fire); i <= static_cast<int32>(EElementalType::Max); ++i)
	{
		TypeList.Add(static_cast<EElementalType>(i));
	}

	for (int32 j = 0; j < TypeList.Num(); ++j)
	{
		int32 idx = FMath::RandRange(j, TypeList.Num() - 1);
		if (j != idx)
		{
			TypeList.Swap(j, idx);
		}
	}

	if (mapElementalWeaponID.Num() > 3)
	{
		for (int32 Count = 0; Count < 2; ++Count)
		{
			int32 Index = FMath::RandRange(0, TypeList.Num() - 1);
			if (mapElementalWeaponID.Contains(TypeList[Index]))
			{
				mapElementalWeaponID.Remove(TypeList[Index]);
			}
		}
	}
}

void ASDKObjectElemental::SetWeaponGrade()
{
	if (mapElementalWeaponID.Num() > 0)
	{
		EItemGrade MinGrade = EItemGrade::Normal;
		if (mapHaveElementals.Num() >= 2)
		{
			TArray<EItemGrade> GradeList;
			mapHaveElementals.GenerateValueArray(GradeList);

			MinGrade = FMath::Min(GradeList[0], GradeList[1]);
		}

		for (auto& itElemental : mapElementalWeaponID)
		{
			if (mapHaveElementals.Contains(itElemental.Key))
			{
				itElemental.Value += (static_cast<int32>(mapHaveElementals[itElemental.Key]) - 1) * 100;
			}
			else
			{
				itElemental.Value += (static_cast<int32>(MinGrade) - 1) * 100;
			}
		}
	}
}

void ASDKObjectElemental::SetSpecialWeaponUpgrade()
{
	for (auto& iter : mapElementalWeaponID)
	{
		if (mapHaveElementals.Num() > 0 && mapHaveElementals.Contains(iter.Key))
		{
			iter.Value += 100;
		}
		else
		{
			if (bUpgradeReward)
			{
				iter.Value += 100;
			}
		}
	}
}
