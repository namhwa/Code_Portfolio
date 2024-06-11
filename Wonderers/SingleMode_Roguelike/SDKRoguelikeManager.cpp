#include "SingleMode_Roguelike/SDKRoguelikeManager.h"

#include "Share/SDKHelper.h"
#include "Manager/SDKTableManager.h"
#include "GameMode/SDKRpgState.h"
#include "GameMode/SDKRpgGameMode.h"


ASDKRoguelikeManager::ASDKRoguelikeManager()
	: CurrentSector(nullptr), CurrentIndex(-1)
{
	PrimaryActorTick.bCanEverTick = false;

	SectorList.Empty();
}

void ASDKRoguelikeManager::BeginPlay()
{
	Super::BeginPlay();
}

bool ASDKRoguelikeManager::GetIsNonePhaseSector() const
{
	if (SectorList.Num() <= 1)
	{
		if(CurrentSector != nullptr)
		{
			return CurrentSector->GetObjectPhaseCount() <= 0;
		}
		else
		{
			return true;
		}
	}

	return false;
}

ASDKRoguelikeSector* ASDKRoguelikeManager::GetRoguelikeSector(int32 Index) const
{
	if (SectorList.Num() > 0)
	{
		if (SectorList.IsValidIndex(Index))
		{
			return SectorList[Index];
		}
	}

	return nullptr;
}

void ASDKRoguelikeManager::SetCurrentSector(ASDKRoguelikeSector* pSector)
{
	CurrentSector = pSector;
	CurrentIndex = SectorList.Find(pSector);
}

void ASDKRoguelikeManager::AddRoguelikeSector(ASDKRoguelikeSector* newSector, FRpgSectorData Data)
{
	if(newSector == nullptr)
		return;

	newSector->SetActorEnableCollision(false);
	newSector->SetLevelData(Data);
	
	if(SectorList.Contains(newSector) == true)
		return;

	SectorList.Emplace(newSector);
}

void ASDKRoguelikeManager::ClearRoguelikeSectorList()
{
	if (SectorList.Num() <= 0)
	{
		CurrentIndex = -1;
		return;
	}

	for(auto& iter : SectorList)
	{
		if(iter == nullptr)
			continue;

		iter->ClearObjectPhases();

		iter->SetLifeSpan(0.01f);
	}

	CurrentIndex = -1;
	SectorList.Empty();
}