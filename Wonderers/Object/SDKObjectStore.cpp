// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/SDKObjectStore.h"

#include "GameMode/SDKRpgState.h"
#include "Character/SDKPlayerCharacter.h"
#include "Character/SDKPlayerController.h"
#include "GameMode/SDKRpgState.h"
#include "Item/SDKItem.h"
#include "Manager/SDKTableManager.h"
#include "Object/SDKSpawnComponent.h"
#include "Object/SDKSwitchComponent.h"


ASDKObjectStore::ASDKObjectStore()
	: StoreItemID(0), DropStoreID(0)
{

}

void ASDKObjectStore::BeginPlay()
{
	Super::BeginPlay();

	ASDKRpgState* SDKRpgState = Cast<ASDKRpgState>(GetWorld()->GetGameState());
	if (nullptr != SDKRpgState)
	{
		if (ERPGRefreshType::Dice == RefreshType)
		{
			SDKRpgState->SetDiceSpawn(false);
		}
	}
}

void ASDKObjectStore::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (GetNetMode() != NM_DedicatedServer)
	{
		if(Cast<ASDKCharacter>(OtherActor))
		{
			OnBeginOverlapDelegate.Broadcast(OtherActor);
		}
	}
}

void ASDKObjectStore::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (GetNetMode() != NM_DedicatedServer)
	{
		if(Cast<ASDKCharacter>(OtherActor))
		{
			OnEndOverlapDelegate.Broadcast(OtherActor);
		}
	}
}

void ASDKObjectStore::InitObject()
{
	if(!USDKTableManager::Get())
		return;

	// 초기 위치 설정
	if (ERPGRefreshType::Store == RefreshType)
	{
		InitialLocation = GetActorLocation();

		auto TableObject = USDKTableManager::Get()->FindTableRow<FS_Object>(ETableType::tb_Object, FString::FromInt(TableID));
		if (TableObject)
		{
			ObjectData.DeathIconPath = TableObject->DeathIconPath;
		}

		if (GetWorld() != nullptr)
		{
			ASDKRpgState* SDKRpgState = GetWorld()->GetGameState<ASDKRpgState>();
			if (SDKRpgState != nullptr)
			{
				TInlineComponentArray<USDKSpawnComponent*> SDKSpawnComponents;
				GetComponents(SDKSpawnComponents);
				if (SDKSpawnComponents.Num() > 0)
				{
					auto itSpawnComp = SDKSpawnComponents.CreateIterator();
					if ((*itSpawnComp)->GetStoreDropTableID() > 0)
					{
						DropStoreID = (*itSpawnComp)->GetStoreDropTableID();
						SDKRpgState->SetDroppableStoreItemIDs(DropStoreID, (*itSpawnComp)->GetIsBlackMarket());
					}
				}
			}
		}
	}
}

void ASDKObjectStore::UseObject(AActor* UseActor)
{
	if(GetOperatedBySwitch() == true)
	{
		// 스위치에 의해 조작받는 오브젝트
		MultiSetRefreshStoreItem(true);
		OnActiveObject();
	}
	else
	{
		TInlineComponentArray<USDKSwitchComponent*> SwitchComponents;
		GetComponents(SwitchComponents);
		if (SwitchComponents.Num() > 0)
		{
			auto itSwitchComp = SwitchComponents.CreateIterator();
			(*itSwitchComp)->ToggleSwitch(UseActor);
		}

		if (ERPGRefreshType::Store == RefreshType)
		{
			OnUpdateRefreshCostWidget();
		}
		else if (ERPGRefreshType::Dice == RefreshType)
		{
			ASDKRpgState* SDKRpgState = Cast<ASDKRpgState>(GetWorld()->GetGameState());
			if (nullptr != SDKRpgState)
			{
				if (!SDKRpgState->GetDiceSpawn())
				{
					OnSpawnDice();
					SDKRpgState->SetDiceSpawn(true);
					
					//auto SDKCharacter = Cast<ASDKPlayerCharacter>(UseActor);
					//if (nullptr != SDKCharacter)
					//{
					//	auto SDKPlayerController = Cast<ASDKPlayerController>(SDKCharacter->GetController());
					//	if (nullptr != SDKPlayerController)
					//	{
					//		SDKPlayerController->SetDeActiveObject(this);
					//	}
					//}
				}
			}
		}
	}
}

void ASDKObjectStore::SetStoreItemID(int32 ItemID, bool bIsBlackMarket, bool bDarkArtifact)
{
	StoreItemID = ItemID;

	ASDKRpgState* SDKRpgState = GetWorld()->GetGameState<ASDKRpgState>();
	if (SDKRpgState != nullptr)
	{
		if (SDKRpgState->SetDroppedStoreItemID(DropStoreID, ItemID, bIsBlackMarket, bDarkArtifact) == false)
		{
			RespawnStoreDropItem();
		}
	}
}

void ASDKObjectStore::RespawnStoreDropItem()
{
	for(auto& itChild : Children)
	{
		if(itChild == nullptr)
			continue;

		ASDKItem* pShopItem = Cast<ASDKItem>(itChild);
		if(pShopItem != nullptr)
		{
			itChild->SetLifeSpan(0.01f);
			itChild = nullptr;
		}
	}

	Children.Empty();

	TInlineComponentArray<USDKSpawnComponent*> SpawnComponents;
	GetComponents(SpawnComponents);

	for(auto& SpawnComp : SpawnComponents)
	{
		SpawnComp->SpawnStoreDropTableItem();
	}
}

void ASDKObjectStore::MultiSetRefreshStoreItem_Implementation(bool bActive)
{
	// 상점 아이템 제거 밑 가격 조정
	if(GetOperatedBySwitch() == true)
	{
		for(auto& itChild : Children)
		{
			if(itChild == nullptr)
				continue;

			ASDKItem* pShopItem = Cast<ASDKItem>(itChild);
			if(pShopItem != nullptr)
			{
				itChild->SetLifeSpan(0.01f);
				itChild = nullptr;
			}
		}

		Children.Empty();

		TInlineComponentArray<USDKSpawnComponent*> SpawnComponents;
		GetComponents(SpawnComponents);

		for(auto& SpawnComp : SpawnComponents)
		{
			if(bActive == true)
			{
				SpawnComp->SpawnStoreDropTableItem();
			}
		}
	}
}
