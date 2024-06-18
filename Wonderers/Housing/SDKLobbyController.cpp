// Fill out your copyright notice in the Description page of Project Settings.

#include "Housing/SDKLobbyController.h"

#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

#include "Character/SDKHUD.h"
#include "Character/SDKMyInfo.h"
#include "Character/SDKPlayerCharacter.h"

#include "Engine/SDKGameInstance.h"
#include "Housing/SDKLobbyMode.h"
#include "Manager/SDKTableManager.h"
#include "Manager/SDKCheatManager.h"
#include "Manager/SDKLobbyServerManager.h"

#include "Object/SDKFurniture.h"

#include "Share/SDKHelper.h"
#include "Share/SDKDataStruct.h"

#include "UI/SDKLobbyWidget.h"
#include "UI/SDKHousingWidget.h"
#include "UI/SDKMessageBoxWidget.h"


ASDKLobbyController::ASDKLobbyController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	CheatClass = USDKCheatManager::StaticClass();

	DefaultRotator = FRotator(-43.f, 0.f, 0.f);
	TopDownRotator = FRotator(-75.f, 0.f, 0.f);

}

bool ASDKLobbyController::InputTouch(uint32 Handle, ETouchType::Type Type, const FVector2D& TouchLocation, float Force, FDateTime DeviceTimestamp, uint32 TouchpadIndex)
{
	bool bResult = Super::InputTouch(Handle, Type, TouchLocation, Force, DeviceTimestamp, TouchpadIndex);

	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetHUD());
	if(IsValid(SDKHUD))
	{
		if(SDKHUD->IsOpenUI(EUI::Lobby_UIHousing))
		{
			InputTouchHousing(Type, TouchpadIndex);
		}
	}
	
	return bResult;
}

void ASDKLobbyController::ClientAttachHud()
{
	Super::ClientAttachHud();

	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetHUD());
	if(IsValid(SDKHUD))
	{
		SDKHUD->OpenUI(EUI::Top_TopBar);
	
		if(UGameplayStatics::GetCurrentLevelName(GetWorld()) == TEXT("Myroom"))
		{
			SDKHUD->OpenUIBackground(EUI::Lobby_Main);
		}
		else
		{
			SDKHUD->OpenUIBackground(EUI::CharacterSelect_Main);
		}
	}
}

void ASDKLobbyController::SetEnableClickEvent(bool bEnable)
{
	bEnableClickEvents = bEnable;
	bEnableTouchEvents = bEnable;
}

//*** Input ****************************************************************************************************************************************************************//
void ASDKLobbyController::SetInputGameMode(bool bGameOnly)
{
	if(bGameOnly)
	{
		FInputModeGameOnly tInputGameOnly;
		SetInputMode(tInputGameOnly);
	}
	else
	{
		bool bDuringCapture = false;

		auto GameInstance = Cast<USDKGameInstance>(GetWorld()->GetGameInstance());
		if (GameInstance != nullptr && GameInstance->GetPlatformName() != TEXT("Windows"))
		{
			bDuringCapture = true;
		}

		FInputModeGameAndUI tInputGameAndUI;
		tInputGameAndUI.SetHideCursorDuringCapture(bDuringCapture);

		SetInputMode(tInputGameAndUI);
	}
}

void ASDKLobbyController::InputTouchHousing(ETouchType::Type Type, uint32 TouchpadIndex)
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetHUD());
	if(IsValid(SDKHUD))
	{
		USDKUserWidget* MainWidget = SDKHUD->GetUI(EUI::Lobby_UIHousing);
		if(IsValid(MainWidget))
		{
			USDKHousingWidget* HousingWidget = Cast<USDKHousingWidget>(MainWidget);
			if(IsValid(HousingWidget))
			{
				ETouchIndex::Type TouchIndex = (ETouchIndex::Type)TouchpadIndex;
				EHousingState eState = HousingWidget->GetHousingState();
			
				switch(Type)
				{
				case ETouchType::Began:
				{
					if(eState != EHousingState::ModifyModeState && eState != EHousingState::PutModeState)
					{
						return;
					}
			
					bool bMovable = HousingWidget->StartMovablePreviewFurniture(TouchIndex);
					if(!bMovable)
					{
						return;
					}
			
					if(eState == EHousingState::ModifyModeState)
					{
						SetIsMovingFurniture(bMovable);
					}
			
					HousingWidget->UpdatePreviewFurniture(TouchIndex);
				}
				break;
				case ETouchType::Moved:
				{
					if(eState != EHousingState::ModifyMovableState && eState != EHousingState::PutMovableState)
					{
						return;
					}
			
					HousingWidget->UpdatePreviewFurniture(TouchIndex);
				}
				break;
				case ETouchType::Ended:
				{
					if(eState == EHousingState::ModifyMovableState)
					{
						HousingWidget->CompleteMoveModifyFurniture(IsPutableState);
					}
					else if(eState == EHousingState::PutMovableState)
					{
						EFurnitureType type = HousingWidget->GetPreviewFurniture()->GetFurnitureType();
						if(type == EFurnitureType::Wall || type == EFurnitureType::Floor)
						{
							ApplyWallpaperFlooring();
						}
						else
						{
							HousingWidget->CompletePutFurniture(IsPutableState);
						}
					}
				}
				break;
				default:
					break;
				}
			}
		}
	}
}

//*** MyRoom ***************************************************************************************************************************************************************//
ASDKFurniture* ASDKLobbyController::GetmapFurnitureByUniqueID(const FString strUniqueID) const
{
	if(mapFurnitures.Contains(strUniqueID))
	{
		return mapFurnitures[strUniqueID];
	}

	return nullptr;
}

FVector ASDKLobbyController::GetRoomStartLocation(int32 iRoomIndex) const
{
	if(mapBaseLocation.Contains(iRoomIndex))
	{
		return mapBaseLocation[iRoomIndex];
	}

	return FVector(-1.f);
}

void ASDKLobbyController::InitMyroomBuildingData()
{
	TArray<AActor*> LevelActor;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASDKFurniture::StaticClass(), LevelActor);

	if(LevelActor.Num() <= 0)
	{
		return;
	}

	for(auto& iter : LevelActor)
	{
		ASDKFurniture* LevelFurniture = Cast<ASDKFurniture>(iter);
		if(!IsValid(LevelFurniture))
		{
			continue;
		}

		FString strBaseID = LevelFurniture->GetBaseTableID();
		switch(LevelFurniture->GetFurnitureType())
		{
		case EFurnitureType::Floor:
			if(mapObjectFloors.FindOrAdd(strBaseID) == nullptr)
			{
				mapObjectFloors[strBaseID] = LevelFurniture;
			}
			break;
		case EFurnitureType::Wall:
			if(mapObjectWalls.FindOrAdd(strBaseID) == nullptr)
			{
				mapObjectWalls[strBaseID] = LevelFurniture;
			}
			break;
		default:
			break;
		}
	}
}

void ASDKLobbyController::InitMyroomTilesData(int32 RoomIndex)
{
	for(int32 iKey = 1; iKey <= RoomIndex; ++iKey)
	{
		if(mapTilesData.Find(iKey) != nullptr)
		{
			continue;
		}

		mapTilesData.Add(iKey, CRoomTilesData());
		mapTilesData[iKey].InitRoomTileData(iKey);
	}

	for(auto& iterTable : USDKTableManager::Get()->GetRowMapMyroomLevel())
	{
		int32 iKey = FCString::Atoi(*iterTable.Key.ToString());
		FS_MyroomLevel* MyroomLevelTable = reinterpret_cast<FS_MyroomLevel*>(iterTable.Value);
		if(MyroomLevelTable != nullptr)
		{
			mapBaseLocation.FindOrAdd(iKey) = MyroomLevelTable->StartLocation;
		}
	}
}

void ASDKLobbyController::SaveMyroomHousingData()
{
	CHousingData& HousingData = g_pMyInfo->GetHousingData();
	if(HousingData.GetArrangedAllItemCount() <= 0)
	{
		return;
	}

	TArray<CFurnitureData> SendData;
	if(HousingData.GetmapWallpaper().Num() > 0)
	{
		for(auto& itWall : HousingData.GetmapWallpaper())
		{
			SendData.Emplace(itWall.Value);
		}
	}

	if(HousingData.GetmapFlooring().Num() > 0)
	{
		for(auto& itFloor : HousingData.GetmapFlooring())
		{
			SendData.Emplace(itFloor.Value);
		}
	}

	if(HousingData.GetmapFurnitures().Num() > 0)
	{
		for(auto& itParts : HousingData.GetmapFurnitures())
		{
			SendData.Emplace(itParts.Value);
		}
	}

	// g_pLobbyServerManager->CQ_SetHousingData(SendData);
}

void ASDKLobbyController::LoadMyroomHousingData()
{
	// 서버에서 받은 정보 셋팅
	for(auto& iter : g_pMyInfo->GetHousingData().GetmapWallpaper())
	{
		if(!mapObjectWalls.Contains(iter.Key))
		{
			continue;
		}

		FS_MyroomParts* FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(iter.Value.TableID);
		if(FurnitureTable != nullptr)
		{
			auto FurnitureMaterial = LoadObject<UMaterialInterface>(nullptr, *FurnitureTable->MaterialPath.ToString());
			if (FurnitureMaterial)
			{
				mapObjectWalls[iter.Key]->SetStaticMeshMaterial(FurnitureMaterial);
			}
			else
			{
				FString Str = FString::Printf(TEXT("Invalid FurnitureTable MaterialPath : (%s)"), *iter.Value.TableID);
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Str, true, FVector2D(3.f, 3.f));
			}
		}
	}

	for(auto& iter : g_pMyInfo->GetHousingData().GetmapFlooring())
	{
		if(!mapObjectFloors.Contains(iter.Key))
		{
			continue;
		}

		FS_MyroomParts* FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(iter.Value.TableID);
		if(FurnitureTable != nullptr)
		{
			auto FurnitureMaterial = LoadObject<UMaterialInterface>(nullptr, *FurnitureTable->MaterialPath.ToString());
			if (FurnitureMaterial)
			{
				mapObjectFloors[iter.Key]->SetStaticMeshMaterial(FurnitureMaterial);
			}
			else
			{
				FString Str = FString::Printf(TEXT("Invalid FurnitureTable MaterialPath : (%s)"), *FurnitureTable->MaterialPath.ToString());
				UE_LOG(LogGame, Warning, TEXT("Invalid FurnitureTable MaterialPath : (%s)"), *iter.Value.TableID);
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Str, true, FVector2D(3.f, 3.f));
			}
		}
	}

	for(auto& iter : g_pMyInfo->GetHousingData().GetmapFurnitures())
	{
		SpawnLoadFurniture(iter.Key, iter.Value);
	}
}

void ASDKLobbyController::RevertMyroomHousignData()
{
	// 전체 아이템 지우기
	ClearMyroomAllFurnitureData();

	// 기존 서버 데이터로 원복
	g_pMyInfo->GetHousingData().RevertServerData();

	// 다시 로드
	LoadMyroomHousingData();
}

void ASDKLobbyController::ApplayMyroomPresetData(FString PresetID)
{
	TMap<FName, uint8*> mapBasicTableRow = USDKTableManager::Get()->GetRowMapMyroomBasic();
	FS_MyroomPreset* PresetTable = USDKTableManager::Get()->FindTableMyroomPreset(PresetID);
	if(PresetTable == nullptr)
	{
		return;
	}

	int32 iRoomLevel = g_pMyInfo->GetHousingData().GetHousingLevel();

	// Wall & Floor
	for(auto& iterBasic : mapBasicTableRow)
	{
		FS_MyroomBasic* BasicTable = reinterpret_cast<FS_MyroomBasic*>(iterBasic.Value);
		if(BasicTable == nullptr)
		{
			continue;
		}

		if(BasicTable->RoomIndex > iRoomLevel)
		{
			continue;
		}

		FString strTableID = FString();
		if(BasicTable->Type)
		{
			strTableID = FString::FromInt(PresetTable->Floors[BasicTable->RoomIndex - 1]);
			const CItemData* pFloorItemData = g_pMyInfo->GetInvenDataList().GetItemData(strTableID);
			g_pMyInfo->GetHousingData().ApplyFlooring(CFurnitureData(strTableID, iterBasic.Key.ToString()));
		}
		else
		{
			int32 structidx = (int32)BasicTable->Direction;
			strTableID = FString::FromInt(PresetTable->Walls[BasicTable->RoomIndex - 1].Walls[structidx]);
			const CItemData* pWallItemData = g_pMyInfo->GetInvenDataList().GetItemData(strTableID);
			g_pMyInfo->GetHousingData().ApplyWallpaper(CFurnitureData(strTableID, iterBasic.Key.ToString()));
		}
	}

	// Furnitures
	for(int32 idx = 0; idx < PresetTable->PropID.Num(); ++idx)
	{
		FString strTableID = FString::FromInt(PresetTable->PropID[idx]);
		FS_MyroomParts* FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(strTableID);
		if(FurnitureTable == nullptr)
		{
			continue;
		}

		FVector vSize = FurnitureTable->Size;
		float fYaw = PresetTable->PropRotate[idx].Yaw;
		if(fYaw > 90.f || (fYaw > -90.f && fYaw < 0.f))
		{
			vSize = FVector(vSize.Y, vSize.X, vSize.Z);
		}

		FVector vSavedIndex = CalcPickingTileIndex(PresetTable->PropRoom[idx], PresetTable->PropPosition[idx], false) - ((vSize - 1.f) * 0.5f);
		if(FurnitureTable->Type == EFurnitureType::Decoration)
		{
			vSavedIndex = FVector((int32)vSavedIndex.X, (int32)vSavedIndex.Y, FMath::CeilToInt(vSavedIndex.Z));
		}
		else
		{
			vSavedIndex = FVector((int32)vSavedIndex.X, (int32)vSavedIndex.Y, (int32)vSavedIndex.Z);
		}

		FString UniqueID = FSDKHelpers::GetNewUniqueID();
		const CItemData* pItemData = g_pMyInfo->GetInvenDataList().GetItemData(strTableID);
		if(pItemData == nullptr)
		{	
			continue;
		}

		CFurnitureData tData = CFurnitureData(PresetTable->PropRoom[idx], strTableID, PresetTable->PropPosition[idx], PresetTable->PropRotate[idx]);
		g_pMyInfo->GetHousingData().ApplyFurniture(UniqueID, tData);
	}
}

void ASDKLobbyController::SpawnLoadFurniture(const FString strUniqueID, const CFurnitureData tFurnitureData)
{
	FS_MyroomParts* FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(tFurnitureData.TableID);
	if(FurnitureTable == nullptr)
	{
		return;
	}

	UClass* PartsClass = LoadClass<ASDKFurniture>(nullptr, *FurnitureTable->PartsClass.ToString());
	if(IsValid(PartsClass))
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// 콜리전 회전값 보정
		FVector vLocation = tFurnitureData.Location;
		FRotator tRotator = tFurnitureData.Rotator;
		tRotator.Yaw -= 45.f;

		ASDKFurniture* ObjectFurniture = GetWorld()->SpawnActor<ASDKFurniture>(PartsClass, vLocation, tRotator, SpawnInfo);
		if(IsValid(ObjectFurniture))
		{
			FVector vSize = FurnitureTable->Size;
			float fYaw = ObjectFurniture->GetActorRotation().Yaw;
			if(fYaw > 90.f || (fYaw > -90.f && fYaw < 0.f))
			{
				vSize = FVector(vSize.Y, vSize.X, vSize.Z);
			}

			FVector vSavedIndex = CalcPickingTileIndex(tFurnitureData.RoomIndex, vLocation, false) - ((vSize - 1.f) * 0.5f);
			if(FurnitureTable->Type == EFurnitureType::Decoration)
			{
				vSavedIndex = FVector((int32)vSavedIndex.X, (int32)vSavedIndex.Y, FMath::CeilToInt(vSavedIndex.Z));
			}
			else
			{
				vSavedIndex = FVector((int32)vSavedIndex.X, (int32)vSavedIndex.Y, (int32)vSavedIndex.Z);
			}

			ObjectFurniture->SetFurnitureID(tFurnitureData.TableID);
			ObjectFurniture->SetArrangedRoomIndex(tFurnitureData.RoomIndex);

			ObjectFurniture->SetUniqueID(strUniqueID);
			ObjectFurniture->SetSavedIndex(vSavedIndex);
			ObjectFurniture->SetFurnitureSize(vSize);

			mapFurnitures.FindOrAdd(strUniqueID, ObjectFurniture);

			SaveMyroomTileData(tFurnitureData.RoomIndex, vSavedIndex, vSize, strUniqueID);
		}
	}
}

//*** Housing **************************************************************************************************************************************************************//
void ASDKLobbyController::CheckDecorationOnFurniture(int32 iRoomKey, FString strUniqueID, FVector vIndex, FVector vSize, TArray<ASDKFurniture*>& ReturnDecorations)
{
	if(mapTilesData.Find(iRoomKey) == nullptr)
	{
		return;
	}

	TArray<FString> UniqueIDs;
	mapTilesData[iRoomKey].GetSavedTilesData(strUniqueID, vIndex, vSize, UniqueIDs);

	for(auto& iterID : UniqueIDs)
	{
		if(!mapFurnitures.Contains(iterID) || !IsValid(mapFurnitures[iterID]))
		{
			continue;
		}

		if(mapFurnitures[iterID]->GetFurnitureType() != EFurnitureType::Decoration)
		{
			continue;
		}

		ReturnDecorations.AddUnique(mapFurnitures[iterID]);
	}
}

FVector ASDKLobbyController::CalcPickingTileIndex(int32 iRoomKey, FVector vMousePoint, bool bIsInt /*= true*/)
{
	if(mapBaseLocation.Find(iRoomKey) == nullptr)
	{
		return FVector(-1.f);
	}

	FVector vPoint = vMousePoint - mapBaseLocation[iRoomKey];
	vPoint = vPoint.RotateAngleAxis(-45.f, FVector::UpVector);
	vPoint = FVector(vPoint.X / -FMath::Sqrt(2.f), vPoint.Y / FMath::Sqrt(2.f), vPoint.Z / FMath::Sqrt(2.f)) / TILE_INTERVAL;

	if(!bIsInt)
	{
		return vPoint;
	}

	return FVector((int32)vPoint.X, (int32)vPoint.Y, (int32)vPoint.Z);
}

FVector ASDKLobbyController::CalcDecorationPickingTileIndex(int32 iRoomKey, FVector vMousePoint)
{
	if(mapBaseLocation.Find(iRoomKey) == nullptr)
	{
		return FVector(-1.f);
	}

	FVector vPoint = vMousePoint - mapBaseLocation[iRoomKey];
	vPoint = vPoint.RotateAngleAxis(-45.f, FVector::UpVector);
	vPoint = FVector(vPoint.X / -FMath::Sqrt(2.f), vPoint.Y / FMath::Sqrt(2.f), vPoint.Z / FMath::Sqrt(2.f));
	vPoint /= TILE_INTERVAL;

	return FVector((int32)vPoint.X, (int32)vPoint.Y, FMath::CeilToInt(vPoint.Z));
}

FVector ASDKLobbyController::CalcFurnitureCenterLocation(int32 iRoomKey, FVector vIndex, FVector vSize)
{
	if(mapBaseLocation.Find(iRoomKey) == nullptr)
	{
		return FVector(-1.f);
	}

	FVector vInterval = vSize * 0.5f;
	FVector vPosition = FVector(vIndex.X + vInterval.X, vIndex.Y + vInterval.Y, vIndex.Z + vInterval.Z);
	FVector vResult = FVector(-TILE_INTERVAL * (vPosition.X + vPosition.Y), TILE_INTERVAL * (-vPosition.X + vPosition.Y), vPosition.Z * TILE_INTERVAL * FMath::Sqrt(2.f));

	return vResult + mapBaseLocation[iRoomKey];
}

FVector ASDKLobbyController::CalcDecorationCenterLocation(int32 iRoomKey, FVector vIndex, FVector vSize, float fHitActorHeight)
{
	if(mapBaseLocation.Find(iRoomKey) == nullptr)
	{
		return FVector(-1.f);
	}

	FVector vInterval = vSize * 0.5f;
	FVector vPosition = FVector(vIndex.X + vInterval.X, vIndex.Y + vInterval.Y, vIndex.Z + vInterval.Z);
	FVector vResult = FVector(-TILE_INTERVAL * (vPosition.X + vPosition.Y), TILE_INTERVAL * (-vPosition.X + vPosition.Y), vPosition.Z * TILE_INTERVAL * FMath::Sqrt(2.f));
	vResult.Z += fHitActorHeight;

	return FVector(vResult.X + mapBaseLocation[iRoomKey].X, vResult.Y + mapBaseLocation[iRoomKey].Y, vResult.Z - mapBaseLocation[iRoomKey].Z);
}

bool ASDKLobbyController::CalcPossibleArrangeTile(int32 iRoomKey, FVector vIndex, FVector vSize, FString strUniqueID /*= TEXT("")*/)
{
	if(mapTilesData.Find(iRoomKey) == nullptr)
	{
		return false;
	}

	if(strUniqueID.IsEmpty() == false)
	{
		return mapTilesData[iRoomKey].CheckEnableTileData(vIndex, vSize, strUniqueID);
	}

	return mapTilesData[iRoomKey].CheckEnableTileData(vIndex, vSize);
}

bool ASDKLobbyController::CalcPossibleArrangeType(int32 iRoomKey, FVector vIndex, FVector vSize, FString TableID)
{
	if(mapTilesData.Find(iRoomKey) == nullptr)
	{
		return false;
	}

	return mapTilesData[iRoomKey].CheckCompareTileType(vIndex, vSize, TableID);
}

void ASDKLobbyController::SaveMyroomTileData(int32 iRoomIndex, FVector vIndex, FVector vSize, FString strUniqueID)
{
	if(mapTilesData.Find(iRoomIndex) == nullptr)
	{
		return;
	}

	mapTilesData[iRoomIndex].SaveNewRoomTileData(vIndex, vSize, strUniqueID);
}

void ASDKLobbyController::RemoveMyroomTileData(int32 iRoomIndex, FVector vIndex, FVector vSize)
{
	if(mapTilesData.Find(iRoomIndex) == nullptr)
	{
		return;
	}

	mapTilesData[iRoomIndex].RemoveRoomTileData(vIndex, vSize);
}

ASDKFurniture* ASDKLobbyController::PutFurniture(ASDKFurniture* PreviewObject)
{
	if(!IsValid(PreviewObject) || PreviewObject->GetFurnitureID().IsEmpty())
	{
		return nullptr;
	}

	FString TableID = PreviewObject->GetFurnitureID();
	FVector vSize = PreviewObject->GetFurnitureSize();
	FVector vIndex = PreviewObject->GetSavedIndex();

	FS_MyroomParts* FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(TableID);
	if(FurnitureTable != nullptr)
	{
		UClass* PartsClass = LoadClass<ASDKFurniture>(nullptr, *FurnitureTable->PartsClass.ToString());
		if(IsValid(PartsClass))
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			ASDKFurniture* newFurniture = GetWorld()->SpawnActor<ASDKFurniture>(PartsClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
			if(IsValid(newFurniture))
			{
				FString strNewUniqueID = FSDKHelpers::GetNewUniqueID();
				newFurniture->SetUniqueID(strNewUniqueID);

				newFurniture->SetFurnitureID(TableID);
				newFurniture->SetActorTransform(PreviewObject->GetActorTransform());
				newFurniture->SetSavedIndex(vIndex);
				newFurniture->SetArrangedRoomIndex(PreviewObject->GetArrangedRoomIndex());
				newFurniture->SetFurnitureArrangeEffect();

				mapFurnitures.FindOrAdd(strNewUniqueID) = newFurniture;

				// 타일 정보에 추가
				SaveMyroomTileData(newFurniture->GetArrangedRoomIndex(), vIndex, vSize, strNewUniqueID);

				// 서버 정보에 저장
				g_pMyInfo->GetHousingData().ApplyFurniture(strNewUniqueID, CFurnitureData(newFurniture->GetArrangedRoomIndex(), newFurniture->GetFurnitureID(), newFurniture->GetActorTransform().GetLocation(), newFurniture->GetActorTransform().Rotator()));
				g_pMyInfo->GetHousingData().SetIsModifiedData(true);

				return newFurniture;
			}
		}
	}

	return nullptr;
}

void ASDKLobbyController::CompleteModifyFurniture(ASDKFurniture* pModifyFurniture, ASDKFurniture* pPreviewFurniture, bool bIsDeco)
{
	// 전체 수정 
	if(!IsValid(pModifyFurniture) || mapFurnitures.FindKey(pModifyFurniture) == nullptr)
	{
		return;
	}

	// 이전 인덱스의 타일 값 제거
	RemoveMyroomTileData(pModifyFurniture->GetArrangedRoomIndex(), pModifyFurniture->GetSavedIndex(), pModifyFurniture->GetFurnitureSize());

	// 데이터 저장
	FString strPrevUniqueID = *mapFurnitures.FindKey(pModifyFurniture);

	// 위지 설정
	pModifyFurniture->SetArrangedRoomIndex(pPreviewFurniture->GetArrangedRoomIndex());
	pModifyFurniture->SetActorRelativeRotation(pPreviewFurniture->GetActorRotation());
	if(pModifyFurniture->GetStaticMeshLocation() != pPreviewFurniture->GetStaticMeshLocation())
	{
		FVector vLocation = CalcFurnitureCenterLocation(pPreviewFurniture->GetArrangedRoomIndex(), pPreviewFurniture->GetSavedIndex(), pPreviewFurniture->GetFurnitureSize());
		pModifyFurniture->SetActorRelativeLocation(vLocation);
	}
	else
	{
		pModifyFurniture->SetActorRelativeLocation(pPreviewFurniture->GetActorLocation());
	}

	// 새 정보로 저장
	if(bIsDeco)
	{
		FVector vSize = pPreviewFurniture->GetFurnitureSize();
		float fRotatorYaw = pPreviewFurniture->GetActorRotation().Yaw;
		if((fRotatorYaw > -90.f && fRotatorYaw < 0.f) || fRotatorYaw > 90.f)
		{
			vSize = FVector(vSize.Y, vSize.X, vSize.Z);
		}

		FVector vSavedIndex = CalcPickingTileIndex(pPreviewFurniture->GetArrangedRoomIndex(), pPreviewFurniture->GetActorLocation(), false) - ((vSize - 1.f) * 0.5f);
		if(pPreviewFurniture->GetFurnitureType() == EFurnitureType::Decoration)
		{
			vSavedIndex = FVector((int32)vSavedIndex.X, (int32)vSavedIndex.Y, FMath::CeilToInt(vSavedIndex.Z));
		}

		pModifyFurniture->SetSavedIndex(vSavedIndex);
		pModifyFurniture->SetFurnitureSize(vSize);
	}
	else
	{
		pModifyFurniture->SetSavedIndex(pPreviewFurniture->GetSavedIndex());
		pModifyFurniture->SetFurnitureSize(pPreviewFurniture->GetFurnitureSize());
	}

	FString strNewUniqueID = FSDKHelpers::GetNewUniqueID();
	pModifyFurniture->SetUniqueID(strNewUniqueID);

	// 컨트롤러에 저장
	mapFurnitures.Remove(strPrevUniqueID);
	mapFurnitures.Shrink();
	mapFurnitures.Emplace(strNewUniqueID, pModifyFurniture);

	// 타일에 저장
	SaveMyroomTileData(pModifyFurniture->GetArrangedRoomIndex(), pModifyFurniture->GetSavedIndex(), pModifyFurniture->GetFurnitureSize(), pModifyFurniture->GetUniqueID());

	// 서버 정보에 저장
	g_pMyInfo->GetHousingData().RemoveFurniture(strPrevUniqueID);
	g_pMyInfo->GetHousingData().ApplyFurniture(strNewUniqueID, CFurnitureData(pModifyFurniture->GetArrangedRoomIndex(), pModifyFurniture->GetFurnitureID(), pModifyFurniture->GetActorTransform().GetLocation(), pModifyFurniture->GetActorTransform().Rotator()));
	g_pMyInfo->GetHousingData().SetIsModifiedData(true);
}

void ASDKLobbyController::ApplyWallpaperFlooring()
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetHUD());
	if(!IsValid(SDKHUD) || !SDKHUD->IsOpenUI(EUI::Lobby_UIHousing))
	{
		ClientApplyWallpaperFlooring(false);
		return;
	}

	USDKUserWidget* MainWidget = SDKHUD->GetUI(EUI::Lobby_UIHousing);
	if(!IsValid(MainWidget))
	{
		ClientApplyWallpaperFlooring(false);
		return;
	}

	USDKHousingWidget* HousingWidget = Cast<USDKHousingWidget>(MainWidget);
	if(IsValid(HousingWidget))
	{
		ASDKFurniture* PrevObject = HousingWidget->GetPreviewFurniture();
		FHitResult HitResult = HousingWidget->GetPreviewHitResult();
	
		if(!IsValid(PrevObject) || !IsValid(HitResult.Actor))
		{
			ClientApplyWallpaperFlooring(false);
			return;
		}
	
		// 충돌 오브젝트 유무 검사
		ASDKFurniture* HitActor = Cast<ASDKFurniture>(HitResult.Actor);
		auto FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(PrevObject->GetFurnitureID());
		if(!IsValid(HitActor) || FurnitureTable == nullptr)
		{
			ClientApplyWallpaperFlooring(false);
			return;
		}
	
		// 벽지나 바닥재 적용
		bool bResult = false;
		FString HitActorName;
		FString PreTableID = TEXT("");
		EFurnitureType eType = FurnitureTable->Type;
	
		if(eType == EFurnitureType::Wall)
		{
			if(mapObjectWalls.FindKey(HitActor) != nullptr && HitResult.GetComponent()->GetCollisionObjectType() == COLLISION_WALL)
			{
				UMaterialInterface* FurnitureMaterial = LoadObject<UMaterialInterface>(nullptr, *FurnitureTable->MaterialPath.ToString());
				if (FurnitureMaterial)
				{
					bResult = true;
					HitActorName = *mapObjectWalls.FindKey(HitActor);
					CFurnitureData WallData = g_pMyInfo->GetHousingData().GetWallpaperByKey(HitActorName);
	
					HitActor->SetStaticMeshMaterial(FurnitureMaterial);
				}
				else
				{
					FString Str = FString::Printf(TEXT("Invalid FurnitureTable MaterialPath : (%s)"), *FurnitureTable->MaterialPath.ToString());
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Str, true, FVector2D(3.f, 3.f));
				}
			}
		}
		else if(eType == EFurnitureType::Floor)
		{
			if(mapObjectFloors.FindKey(HitActor) != nullptr && HitResult.GetComponent()->GetCollisionObjectType() == COLLISION_FLOOR)
			{
				UMaterialInterface* FurnitureMaterial = LoadObject<UMaterialInterface>(nullptr, *FurnitureTable->MaterialPath.ToString());
				if (FurnitureMaterial)
				{
					bResult = true;
					HitActorName = *mapObjectFloors.FindKey(HitActor);
					CFurnitureData FlooringData = g_pMyInfo->GetHousingData().GetFlooringByKey(HitActorName);

					HitActor->SetStaticMeshMaterial(FurnitureMaterial);
				}
				else
				{
					FString Str = FString::Printf(TEXT("Invalid FurnitureTable MaterialPath : (%s)"), *FurnitureTable->MaterialPath.ToString());
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Str, true, FVector2D(3.f, 3.f));
				}
			}
		}
	
		g_pMyInfo->GetHousingData().ApplyFurniture(HitActorName, CFurnitureData(PrevObject->GetFurnitureID(), HitActorName));
		g_pMyInfo->GetHousingData().SetIsModifiedData(true);
	
		ClientApplyWallpaperFlooring(bResult, PrevObject->GetFurnitureID(), PreTableID);
	}
}

void ASDKLobbyController::SaveFurniture(ASDKFurniture* SavedObject)
{
	if(!IsValid(SavedObject) || mapFurnitures.FindKey(SavedObject) == nullptr)
	{
		return;
	}

	// 이전 인덱스의 타일 값 제거
	RemoveMyroomTileData(SavedObject->GetArrangedRoomIndex(), SavedObject->GetSavedIndex(), SavedObject->GetFurnitureSize());

	// 서버 정보 제거
	g_pMyInfo->GetHousingData().RemoveFurniture(SavedObject->GetUniqueID());
	g_pMyInfo->GetHousingData().SetIsModifiedData(true);

	mapFurnitures.Remove(SavedObject->GetUniqueID());
	mapFurnitures.Shrink();

	// 저장하려는 가구 제거
	SavedObject->SetLifeSpan(0.01f);
	SavedObject = nullptr;
}

void ASDKLobbyController::ClearMyroomAllFurnitureData()
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetHUD());
	if(IsValid(SDKHUD))
	{
		USDKUserWidget* MainWidget = SDKHUD->GetUI(EUI::Lobby_UIHousing);
		if(IsValid(MainWidget))
		{
			USDKHousingWidget* HousingWidget = Cast<USDKHousingWidget>(MainWidget);
			if(IsValid(HousingWidget))
			{
				for(auto& iter : mapFurnitures)
				{
					if(IsValid(iter.Value))
					{
						continue;
					}
			
					// 이전 인덱스의 타일 값 제거
					RemoveMyroomTileData(iter.Value->GetArrangedRoomIndex(), iter.Value->GetSavedIndex(), iter.Value->GetFurnitureSize());
			
					// 하우징 가구 리스트에 추가
					HousingWidget->SetFurnitureCount(iter.Value->GetFurnitureID(), 1);
			
					// 서버 정보 제거
					g_pMyInfo->GetHousingData().RemoveFurniture(iter.Value->GetUniqueID());
			
					// 저장하려는 가구 제거
					iter.Value->SetLifeSpan(0.01f);
					iter.Value = nullptr;
				}
			
				mapFurnitures.Empty();
				g_pMyInfo->GetHousingData().SetIsModifiedData(true);
			}
		}
	}
}

void ASDKLobbyController::InitHousingCamera(bool bOpen)
{
	ASDKPlayerCharacter* PlayerCharater = Cast<ASDKPlayerCharacter>(GetPawn());
	if(IsValid(PlayerCharater))
	{
		if(bOpen)
		{
			if(!IsValid(HousingViewCamera))
			{
				SpawnHousingViewCamera();
			}
	
			HousingViewCamera->SetActorRelativeLocation(PlayerCharater->GetActorLocation());
			SetViewTargetWithBlend(HousingViewCamera, 0.5f);
		}
		else
		{
			if(IsValid(HousingViewCamera))
			{
				HousingViewCamera->RotateCamera(DefaultRotator);
				SetViewTargetWithBlend(PlayerCharater, 0.5f);
			}
		}
	}
}

void ASDKLobbyController::SpawnHousingViewCamera()
{
	if(IsValid(HousingViewCamera))
	{
		return;
	}

	FS_ObjectBlueprint* ObjectBPTable = USDKTableManager::Get()->FindTableObjectBlueprint(EObjectBlueprint::SDKCamera);
	if(ObjectBPTable != nullptr)
	{
		UClass* CameraClass = LoadClass<AActor>(nullptr, *ObjectBPTable->ObjectBlueprintPath.ToString());
		if (IsValid(CameraClass))
		{
			FActorSpawnParameters tSpawnParameter;
			tSpawnParameter.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
			HousingViewCamera = GetWorld()->SpawnActor<ASDKCamera>(CameraClass, FVector::ZeroVector, FRotator::ZeroRotator, tSpawnParameter);
			if (IsValid(HousingViewCamera))
			{
				HousingViewCamera->GetSpringArmComponent()->bEnableCameraRotationLag = true;
				HousingViewCamera->GetSpringArmComponent()->CameraRotationLagSpeed = 3.f;
				HousingViewCamera->GetSpringArmComponent()->CameraLagSpeed = 3.5f;
				HousingViewCamera->GetSpringArmComponent()->CameraLagMaxDistance = 2800.f;
			}
		}
	}
}

void ASDKLobbyController::SetLocationHousingCamera(FVector vLocation)
{
	if (IsValid(HousingViewCamera))
	{
		HousingViewCamera->SetActorRelativeLocation(vLocation);
	}
}

void ASDKLobbyController::MoveHousingViewCamera(FVector2D vMouseDelta)
{
	if (IsValid(HousingViewCamera))
	{

		HousingViewCamera->MoveCamera(FVector(vMouseDelta.Y, -vMouseDelta.X, 0.f));
	}
}

void ASDKLobbyController::RotateHousingViewCamera(bool bIsTop)
{
	if (IsValid(HousingViewCamera))
	{
		HousingViewCamera->RotateCamera(bIsTop ? TopDownRotator : DefaultRotator);
	}
}

//*** HUD ******************************************************************************************************************************************************************//
void ASDKLobbyController::ClientToggleLobbyUIMenu(EUI eMenuType, bool bOpen)
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetHUD());
	if(IsValid(SDKHUD))
	{
		if(bOpen)
		{
			SDKHUD->OpenUI(eMenuType);
		}
		else
		{
			SDKHUD->CloseUI(eMenuType);
		}
	}
}

void ASDKLobbyController::ClientApplyWallpaperFlooring(bool bResult, FString ID /*= TEXT("")*/, FString PreID /*= TEXT("")*/)
{
	// UI 적용
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetHUD());
	if(IsValid(SDKHUD))
	{
		USDKUserWidget* MainWidget = SDKHUD->GetUI(EUI::Lobby_Main);
		if(IsValid(MainWidget))
		{
			USDKLobbyWidget* LobbyWidget = Cast<USDKLobbyWidget>(SDKHUD->GetUI(EUI::Lobby_Main));
			if(IsValid(LobbyWidget))
			{
				LobbyWidget->NotifyAppliedWallpaperFlooring(bResult, ID, PreID);
			}
		}
	}
}

void ASDKLobbyController::ClientySuccessedSaveHounsingData()
{
	g_pMyInfo->GetHousingData().SaveServerData();

	auto SDKHUD = Cast<ASDKHUD>(GetHUD());
	if(IsValid(SDKHUD))
	{
		FS_GlobalDefine* GlobalTable = USDKTableManager::Get()->FindTableGlobalDefine(EGlobalDefine::MsgString_SavedHousing);
		if(GlobalTable != nullptr)
		{
			USDKMessageBoxWidget* MessagBoxWidget = SDKHUD->MessageBoxOK(GlobalTable->Value[0], GlobalTable->Value[1], "", GlobalTable->Value[2]);
			if(IsValid(MessagBoxWidget))
			{
				if(IsValid(MessagBoxWidget->GetButtonOk()))
				{
					MessagBoxWidget->GetButtonOk()->OnClicked.Clear();
					MessagBoxWidget->GetButtonOk()->OnClicked.AddDynamic(SDKHUD, &ASDKHUD::CloseMessageBox);
				}
			}
		}
	}
}

void ASDKLobbyController::ClientToggleMyroomUpgradePopup(bool bOpen, int32 iRoomIndex /*= 0*/)
{
	auto SDKHUD = Cast<ASDKHUD>(GetHUD());
	if(IsValid(SDKHUD))
	{
		USDKUserWidget* MainWidget = SDKHUD->GetUI(EUI::Lobby_Main);
		if(IsValid(MainWidget))
		{
			USDKLobbyWidget* LobbyWidget = Cast<USDKLobbyWidget>(SDKHUD->GetUI(EUI::Lobby_Main));
			if(IsValid(LobbyWidget))
			{
				LobbyWidget->OnToggleMyroomUpgradePopup(bOpen, iRoomIndex);
			}
		}
	}
}
