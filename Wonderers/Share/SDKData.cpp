// Fill out your copyright notice in the Description page of Project Settings.

#include "Share/SDKData.h"

#include "GameFramework/Controller.h"
#include "Kismet/KismetMathLibrary.h"

#include "Character/SDKHUD.h"
#include "Character/SDKMyInfo.h"
#include "Manager/SDKTableManager.h"
#include "Share/SDKHelper.h"
#include "Share/SDKDataTable.h"


/*** 하우징 관련 정보 ***************************************************************************************************************************************/
/*** 가구 ***/
CFurnitureData::CFurnitureData()
	: RoomIndex(0), Location(FVector::ZeroVector), Rotator(FRotator::ZeroRotator)
{
	TableID.Empty();
	AttachID.Empty();
}

CFurnitureData::CFurnitureData(const FString strTableID, const FString strAttachID)
	: RoomIndex(0), Location(FVector::ZeroVector), Rotator(FRotator::ZeroRotator)
{
	TableID = strTableID;
	AttachID = strAttachID;
}

CFurnitureData::CFurnitureData(const int32 iRoomIndex, const FString strTableID, const FVector vLocation, const FRotator vRotator)
	: RoomIndex(iRoomIndex), Location(vLocation), Rotator(vRotator)
{
	TableID = strTableID;
	AttachID.Empty();
}

/*** 하우징 ***/
CHousingData::CHousingData()
	: HousingLevel(1)
{
	IsSavedData = true;
	IsModifiedData = false;

	ClearHousingData();
	ClearHousingOriginData();
}

CHousingData::~CHousingData()
{
	ClearHousingData();
	ClearHousingOriginData();
}

void CHousingData::SetHousingLevel(int32 newLevel)
{
	if(newLevel <= 0)
	{
		return;
	}

	HousingLevel = newLevel;
}

void CHousingData::SetIsSavedData(bool bSaved)
{
	IsSavedData = bSaved;
	if(IsSavedData)
	{
		SetIsModifiedData(!IsSavedData);
	}
}

void CHousingData::SetIsModifiedData(bool bModified)
{
	IsModifiedData = bModified;
	if(IsModifiedData)
	{
		SetIsSavedData(!IsModifiedData);
	}
}

void CHousingData::SetOriginServerData()
{
	mapOriginWall = mapWallpaper;
	mapOriginFloor = mapFlooring;
	mapOriginFurniture = mapFurnitures;
}

void CHousingData::ApplyWallpaper(const CFurnitureData FurnitureData)
{
	mapWallpaper.FindOrAdd(FurnitureData.AttachID) = FurnitureData;
}

void CHousingData::ApplyFlooring(const CFurnitureData FurnitureData)
{
	mapFlooring.FindOrAdd(FurnitureData.AttachID) = FurnitureData;
}

void CHousingData::ApplyFurniture(const FString strUniqueID, const CFurnitureData FurnitureData)
{
	mapFurnitures.FindOrAdd(strUniqueID) = FurnitureData;
}

void CHousingData::RemoveFurniture(const FString strUniqueID)
{
	if(!mapFurnitures.Contains(strUniqueID))
	{
		return;
	}

	mapFurnitures.Remove(strUniqueID);
	mapFurnitures.Shrink();
}

void CHousingData::ClearHousingData()
{
	SetIsValidData(false);

	mapWallpaper.Empty();
	mapFlooring.Empty();
	mapFurnitures.Empty();
}

void CHousingData::ClearHousingOriginData()
{
	SetIsValidData(false);

	mapOriginWall.Empty();
	mapOriginFloor.Empty();
	mapOriginFurniture.Empty();
}

void CHousingData::SaveServerData()
{
	SetIsSavedData(true);
	SetOriginServerData();
}

void CHousingData::RevertServerData()
{
	ClearHousingData();

	mapWallpaper = mapOriginWall;
	mapFlooring = mapOriginFloor;
	mapFurnitures = mapOriginFurniture;
}

FVector CHousingData::GetIndex(FVector vStartLocation, FVector vLocation, FVector vSize, EFurnitureType eType)
{
	FVector vIndex = vLocation - vStartLocation;
	vIndex = vIndex.RotateAngleAxis(-45.f, FVector::UpVector);
	vIndex = FVector(vIndex.X / -FMath::Sqrt(2.f), vIndex.Y / FMath::Sqrt(2.f), vIndex.Z / FMath::Sqrt(2.f)) / TILE_INTERVAL;
	vIndex -= ((vSize - 1.f) * 0.5f);

	if(eType == EFurnitureType::Decoration)
	{
		return FVector((int32)vIndex.X, (int32)vIndex.Y, FMath::CeilToInt(vIndex.Z));
	}

	return FVector((int32)vIndex.X, (int32)vIndex.Y, (int32)vIndex.Z);
}

int32 CHousingData::GetArrangedItemCount(const FString strTableID)
{
	if(strTableID.IsEmpty())
	{
		return -1;
	}

	int32 iReturnCount = 0;

	for(auto& iterWall : mapWallpaper)
	{
		if(iterWall.Value.TableID == strTableID)
			iReturnCount++;
	}

	for(auto& iterFloor : mapFlooring)
	{
		if(iterFloor.Value.TableID == strTableID)
			iReturnCount++;
	}

	for(auto& iter : mapFurnitures)
	{
		if(iter.Value.TableID == strTableID)
			iReturnCount++;
	}

	return iReturnCount;
}

int32 CHousingData::GetArrangedAllItemCount() const
{
	return mapWallpaper.Num() + mapFlooring.Num() + mapFurnitures.Num();
}

CFurnitureData CHousingData::GetWallpaperByKey(const FString strKey) const
{
	if(mapWallpaper.Contains(strKey))
	{
		return mapWallpaper[strKey];
	}

	return CFurnitureData();
}

CFurnitureData CHousingData::GetFlooringByKey(const FString strKey) const
{
	if(mapFlooring.Contains(strKey))
	{
		return mapFlooring[strKey];
	}

	return CFurnitureData();
}

CFurnitureData CHousingData::GetFurnitureByKey(const FString strKey) const
{
	if(mapFurnitures.Contains(strKey))
	{
		return mapFurnitures[strKey];
	}

	return CFurnitureData();
}

/*** 하우징 타일 ***/
void CRoomTilesData::GetSavedTilesData(FString strUniqueID, FVector vStartIndex, FVector vSize, TArray<FString>& arrReturn)
{
	for(int32 idxX = vStartIndex.X; idxX < vStartIndex.X + vSize.X; ++idxX)
	{
		for(int32 idxY = vStartIndex.Y; idxY < vStartIndex.Y + vSize.Y; ++idxY)
		{
			if(idxX >= RoomSize.X || idxY >= RoomSize.Y)
			{
				return;
			}

			int32 iIndex = idxX * RoomSize.Y + idxY;
			for(int32 idxZ = 0; idxZ < MAX_ROOM_HEIGHT; ++idxZ)
			{
				if(Tiles[iIndex].Heights[idxZ].IsEmpty() || Tiles[iIndex].Heights[idxZ] == strUniqueID)
				{
					continue;
				}

				arrReturn.AddUnique(Tiles[iIndex].Heights[idxZ]);
			}
		}
	}
}

void CRoomTilesData::InitRoomTileData(int32 RoomLevel)
{
	RoomIndex = RoomLevel;

	// 방 크기
	FString strIndex = FString::FromInt(RoomLevel);
	auto MyroomLevelTable = USDKTableManager::Get()->FindTableMyroomLevel(strIndex);
	if(MyroomLevelTable != nullptr)
	{
		RoomSize = MyroomLevelTable->RoomScale;
	}

	// 타일 정보
	for(auto& iter : USDKTableManager::Get()->GetRowMapMyroomTile())
	{
		if(iter.Key.ToString().Left(1) != strIndex)
		{
			continue;
		}

		FS_MyroomTile* TileTable = reinterpret_cast<FS_MyroomTile*>(iter.Value);
		if(TileTable == nullptr)
		{
			continue;
		}

		Tiles.Add(FTileData(TileTable->TileType));
	}
}

bool CRoomTilesData::CheckEnableTileData(FVector vStartIndex, FVector vSize, FString strUniqueID /*= TEXT("")*/)
{
	for(int32 idxX = vStartIndex.X; idxX < vStartIndex.X + vSize.X; ++idxX)
	{
		for(int32 idxY = vStartIndex.Y; idxY < vStartIndex.Y + vSize.Y; ++idxY)
		{
			if(idxX >= RoomSize.X || idxY >= RoomSize.Y)
			{
				return false;
			}

			int32 index = idxX * RoomSize.Y + idxY;
			if(!Tiles.IsValidIndex(index))
			{
				return false;
			}

			// Z축 검사 : 위 → 아래
			if(CheckEnableTileDataZ(index, vStartIndex.Z, vSize.Z, strUniqueID))
			{
				continue;
			}
			else
			{
				return false;
			}
		}
	}

	return true;
}

bool CRoomTilesData::CheckEnableTileDataZ(int32 iTileIndex, int32 iZindex, float fZSive, FString strUniqueID /*= TEXT("")*/)
{
	if(Tiles[iTileIndex].TileType == ETileType::Floor || Tiles[iTileIndex].TileType == ETileType::YardFloor)
	{
		for(int32 idxZ = iZindex; idxZ < iZindex + fZSive; ++idxZ)
		{
			if(idxZ < 0 || idxZ >= MAX_ROOM_HEIGHT)
			{
				return false;
			}

#if WITH_EDITOR
			GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Red, Tiles[iTileIndex].Heights[idxZ]);
#endif

			if(!Tiles[iTileIndex].Heights[idxZ].IsEmpty() && Tiles[iTileIndex].Heights[idxZ] != strUniqueID)
			{
				return false;
			}
		}
	}

	if(Tiles[iTileIndex].TileType == ETileType::Wall || Tiles[iTileIndex].TileType == ETileType::YardWall)
	{
		for(int32 idxZ = iZindex; idxZ > iZindex - fZSive; --idxZ)
		{
			if(idxZ < 0 || idxZ >= MAX_ROOM_HEIGHT)
			{
				return false;
			}

			if(!Tiles[iTileIndex].Heights[idxZ].IsEmpty() && Tiles[iTileIndex].Heights[idxZ] != strUniqueID)
			{
				return false;
			}
		}
	}

	return true;
}

bool CRoomTilesData::CheckCompareTileType(FVector vStartIndex, FVector vSize, FString TableID)
{
	FS_MyroomParts* FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(TableID);
	if(FurnitureTable == nullptr)
	{
		return false;
	}

	for(int32 idxX = vStartIndex.X; idxX < vStartIndex.X + vSize.X; ++idxX)
	{
		for(int32 idxY = vStartIndex.Y; idxY < vStartIndex.Y + vSize.Y; ++idxY)
		{
			if(idxX >= RoomSize.X || idxY >= RoomSize.Y)
			{
				return false;
			}

			int32 index = idxX * RoomSize.Y + idxY;
			if(!Tiles.IsValidIndex(index))
			{
				return false;
			}

			switch(Tiles[index].TileType)
			{
			case ETileType::None:
				return false;

			case ETileType::Wall:
				if(FurnitureTable->Type != EFurnitureType::Wall && FurnitureTable->Type != EFurnitureType::WallHangings)
				{
					return false;
				}
				break;

			case ETileType::Floor:
				if(FurnitureTable->Type == EFurnitureType::Wall || FurnitureTable->Type == EFurnitureType::WallHangings)
				{
					return false;
				}
				break;

			case ETileType::YardWall:
				if(FurnitureTable->Category != EFurnitureCategory::Yard || FurnitureTable->Type != EFurnitureType::WallHangings)
				{
					return false;
				}
				break;

			case ETileType::YardFloor:
				if(FurnitureTable->Category != EFurnitureCategory::Yard || FurnitureTable->Type == EFurnitureType::WallHangings)
				{
					return false;
				}
				break;

			default:
				return false;
			}
		}
	}

	return true;
}

void CRoomTilesData::SaveNewRoomTileData(FVector vStartIndex, FVector vSize, FString strUniqueID)
{
	for(int32 idxX = vStartIndex.X; idxX < vStartIndex.X + vSize.X; ++idxX)
	{
		for(int32 idxY = vStartIndex.Y; idxY < vStartIndex.Y + vSize.Y; ++idxY)
		{
			if(idxX >= RoomSize.X || idxY >= RoomSize.Y)
			{
				continue;
			}

			int32 index = idxX * RoomSize.Y + idxY;

			if(Tiles[index].TileType == ETileType::Floor || Tiles[index].TileType == ETileType::YardFloor)
			{
				for(int32 idxZ = vStartIndex.Z; idxZ < vStartIndex.Z + vSize.Z; ++idxZ)
				{
					if(idxZ >= MAX_ROOM_HEIGHT)
					{
						continue;
					}

					Tiles[index].Heights[idxZ] = strUniqueID;
				}
			}
			else
			{
				for(int32 idxZ = vStartIndex.Z; idxZ > vStartIndex.Z - vSize.Z; --idxZ)
				{
					if(idxZ < 0)
						continue;

					Tiles[index].Heights[idxZ] = strUniqueID;
				}
			}
		}
	}
}

void CRoomTilesData::RemoveRoomTileData(FVector vStartIndex, FVector vSize)
{
	for(int32 idxX = vStartIndex.X; idxX < vStartIndex.X + vSize.X; ++idxX)
	{
		for(int32 idxY = vStartIndex.Y; idxY < vStartIndex.Y + vSize.Y; ++idxY)
		{
			if(idxX >= RoomSize.X || idxY >= RoomSize.Y)
			{
				continue;
			}

			int32 index = idxX * RoomSize.Y + idxY;

			if(Tiles[index].TileType == ETileType::Floor || Tiles[index].TileType == ETileType::YardFloor)
			{
				for(int32 idxZ = vStartIndex.Z; idxZ < vStartIndex.Z + vSize.Z; ++idxZ)
				{
					if(idxZ >= MAX_ROOM_HEIGHT)
					{
						continue;
					}

					Tiles[index].Heights[idxZ].Empty();
				}
			}
			else
			{
				for(int32 idxZ = vStartIndex.Z; idxZ > vStartIndex.Z - vSize.Z; --idxZ)
				{
					if(idxZ < 0)
					{
						continue;
					}

					Tiles[index].Heights[idxZ].Empty();
				}
			}
		}
	}
}

/*** 퀘스트 관련 정보 ***************************************************************************************************************************************/
CQuestData::CQuestData(const FQuestData& Data)
{
	QuestID = Data.m_QuestID;
	bIsComplete = Data.m_IsComplete;
	bIsReceivedReward = Data.m_IsRewardReceived;
	
	MissionType = Data.m_MissionType;
	MissionList = Data.m_MissionList;

	SetAllMissionCount();
}

/** 싱글모드 정보 ***************************************************************************************************************************************/
void CRPGModeData::Clear()
{
	CData::Clear();

	bEnterCamp = false;

	ClearPlayingData();

	CurrentChapterID = 0;
	CurrentStageID = 0;
}

void CRPGModeData::SetChapterID(int32 InID)
{
	SetIsValidData(true);

	CurrentChapterID = InID;

	FS_Chapter* ChapterTable = USDKTableManager::Get()->FindTableRow<FS_Chapter>(ETableType::tb_Chapter, FString::FromInt(InID));
	if (ChapterTable != nullptr)
	{
		int32 index = FMath::RandRange(0, ChapterTable->StageID.Num() - 1);
		SetStageID(ChapterTable->StageID[index]);
	}
}

void CRPGModeData::SetSpecialLevelData(const TArray<FSpecialLevelData>& InLevelList)
{
	if (InLevelList.Num() == 0)
	{
		EnableSpecialLevelIndex = 0;
		return;
	}

	TArray<FString> IDList = USDKTableManager::Get()->GetSpecialLevelIDsByChapter(CurrentChapterID);
	TMap<FString, bool> CurrentLevelIDs;

	// 현재 챕터에 맞는 ID랑 플레이 유무 저장
	for (auto& iter : InLevelList)
	{
		FS_SpecialLevel* SpecialTable = USDKTableManager::Get()->FindTableRow<FS_SpecialLevel>(ETableType::tb_SpecialLevel, FString::FromInt(iter.SpecialLevelID));
		if (SpecialTable != nullptr && SpecialTable->Chapter == CurrentChapterID)
		{
			CurrentLevelIDs.Add(FString::FromInt(iter.SpecialLevelID), iter.IsCompleted);
		}
	}


	if (CurrentLevelIDs.Num() <= 0)		// 현재 잽터에서 퍼즐을 한 적이 없는 경우
	{
		EnableSpecialLevelIndex = 0;
	}
	else if (CurrentLevelIDs.Num() > 0 && IDList.Num() > 0)	// 값이 비어있지 않은 경우
	{
		for (int32 i = 0; i < IDList.Num(); ++i)
		{
			// 현재 챕터 아이디 목록에 없거나 깨지 않은 경우
			if (!CurrentLevelIDs.Contains(IDList[i]) || !CurrentLevelIDs[IDList[i]])
			{
				EnableSpecialLevelIndex = i;
				break;
			}
		}
	}

	//가능한 레벨이 있는지 확인
	if (!IDList.IsValidIndex(EnableSpecialLevelIndex))
	{
		EnableSpecialLevelIndex = -1;
	}
}

void CRPGModeData::SetRpgChallengeMode(FName SeasonID, EContentsType ContentType, TMap<int32, int32> mapChallengeData)
{
	RpgContentsType = ContentType;
	ChallengeSeasonID = SeasonID == TEXT("") ? TEXT("10001") : SeasonID;

	PlayerChallengeData.Empty();
	MonsterCallengeData.Empty();

	for (auto Iter : mapChallengeData)
	{
		auto RpgChallengeTable = USDKTableManager::Get()->FindTableRow<FS_ChallengeRpgParameter>(ETableType::tb_ChallengeRpgParameter, FString::FromInt(Iter.Key));
		if (RpgChallengeTable != nullptr)
		{
			int32 Index = Iter.Value - 1 >= 0 ? Iter.Value - 1 : 0;
			EParameter Paramtype = RpgChallengeTable->ParameterType;
			FVector2D Data = FVector2D(RpgChallengeTable->Value[Index], RpgChallengeTable->Rate[Index]);
			if (RpgChallengeTable->TargetType == ERPGTargetType::Player)
			{
				if (!PlayerChallengeData.Contains(Paramtype))
				{
					PlayerChallengeData.Add(Paramtype, Data);
				}
			}
			else if (RpgChallengeTable->TargetType == ERPGTargetType::NPC)
			{
				if (!MonsterCallengeData.Contains(Paramtype))
				{
					MonsterCallengeData.Add(Paramtype, Data);
				}
			}
		}

	}
}

void CRPGModeData::ClearPlayingData()
{
	CurrentDifficulty = EDifficulty::None;

	bEnterCamp = false;
	CampArtifactID = 0;
	EnableSpecialLevelIndex = -1;
	EnableSpecialLevelData = FSpecialLevelData();
}

void CGlitchStoreData::Clear()
{
	RestoredList.Empty();
}

TArray<int32> CGlitchStoreData::GetRestoredList(EProductType InType) const
{
	if (RestoredList.Contains(InType))
	{
		return RestoredList[InType];
	}

	return TArray<int32>();
}

bool CGlitchStoreData::GetIsHaveProduct(EProductType InType, int32 InID) const
{
	if (RestoredList.Num() > 0)
	{
		if (RestoredList.Contains(InType))
		{
			return RestoredList[InType].Contains(InID);
		}
	}

	return false;
}

void CGlitchStoreData::SetRestoredList(const TArray<int32>& InList)
{
	if (InList.Num() > 0)
	{
		for (auto& itID : InList)
		{
			FS_GlitchStore* StoreTable = USDKTableManager::Get()->FindTableRow<FS_GlitchStore>(ETableType::tb_GlitchStore, FString::FromInt(itID));
			if (StoreTable != nullptr)
			{
				if (!RestoredList.Contains(StoreTable->ProductType))
				{
					RestoredList.Add(StoreTable->ProductType);
				}

				RestoredList[StoreTable->ProductType].Add(itID);
			}
		}
	}
}

void CGlitchStoreData::AddRestoreData(const int32& InID)
{
	FS_GlitchStore* StoreTable = USDKTableManager::Get()->FindTableRow<FS_GlitchStore>(ETableType::tb_GlitchStore, FString::FromInt(InID));
	if (StoreTable != nullptr)
	{
		if (!RestoredList.Contains(StoreTable->ProductType))
		{
			RestoredList.Add(StoreTable->ProductType);
		}

		RestoredList[StoreTable->ProductType].Add(InID);
	}
}
