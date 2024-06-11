// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Share/SDKEnum.h"
#include "Share/SDKStruct.h"
#include "Share/SDKDataStruct.h"
#include "flatbuffers/lobby_generated.h"


class CData
{
public:
	CData() {}
	virtual ~CData() {}

	bool GetIsValidData() const { return bIsValid; }
	void SetIsValidData(bool bValid) { bIsValid = bValid; }

	virtual void Clear() { SetIsValidData(false); }

protected:
	bool bIsValid = false;
};

/** 하우징 관련 정보 ********************************************************************************************************************/
/** 가구 */
class  CFurnitureData
{
public:
	CFurnitureData();
	CFurnitureData(const FString strTableID, const FString strAttachID);
	CFurnitureData(const int32 iRoomIndex, const FString strTableID, const FVector vLocation, const FRotator vRotator);
	virtual ~CFurnitureData() { TableID.Empty(); AttachID.Empty(); }
public:
	int32 RoomIndex;
	FString TableID;
	FString AttachID;
	FVector Location;
	FRotator Rotator;
};

/*** 하우징 ***/
class CHousingData : public CData
{
public:
	CHousingData();
	virtual ~CHousingData();

public:
	int32 GetHousingLevel() { return HousingLevel; }
	void SetHousingLevel(int32 newLevel);

	bool GetIsSavedData() { return IsSavedData; }
	void SetIsSavedData(bool bSaved);

	bool GetIsModifiedData() { return IsModifiedData; }
	void SetIsModifiedData(bool bModified);

	void SetOriginServerData();

	void ApplyWallpaper(const CFurnitureData FurnitureData);
	void ApplyFlooring(const CFurnitureData FurnitureData);
	void ApplyFurniture(const FString strUniqueID, const CFurnitureData FurnitureData);

	void RemoveFurniture(const FString strUniqueID);
	void ClearHousingData();
	void ClearHousingOriginData();

	void SaveServerData();
	void RevertServerData();

	FVector GetIndex(FVector vStartLocation, FVector vLocation, FVector vSize, EFurnitureType eType);
	int32 GetArrangedItemCount(const FString strTableID);
	int32 GetArrangedAllItemCount() const;

	// 변경된 정보
	TMap<FString, CFurnitureData>& GetmapWallpaper() { return mapWallpaper; }
	TMap<FString, CFurnitureData>& GetmapFlooring() { return mapFlooring; }
	TMap<FString, CFurnitureData>& GetmapFurnitures() { return mapFurnitures; }

	// 기존 정보
	TMap<FString, CFurnitureData>& GetmapOriginWall() { return mapOriginWall; }
	TMap<FString, CFurnitureData>& GetmapOriginFloor() { return mapOriginFloor; }
	TMap<FString, CFurnitureData>& GetmapOriginFurniture() { return mapOriginFurniture; }

	CFurnitureData GetWallpaperByKey(const FString strKey) const;
	CFurnitureData GetFlooringByKey(const FString strKey) const;
	CFurnitureData GetFurnitureByKey(const FString strKey) const;

private:
	// 서버에서 받은 기존 데이터
	TMap<FString, CFurnitureData> mapOriginWall;
	TMap<FString, CFurnitureData> mapOriginFloor;
	TMap<FString, CFurnitureData> mapOriginFurniture;

	// 바닥재 및 벽지
	TMap<FString, CFurnitureData> mapWallpaper;
	TMap<FString, CFurnitureData> mapFlooring;

	// 배치된 가구 정보
	TMap<FString, CFurnitureData> mapFurnitures;

	// 하우징 레벨 (방 언락 형태)
	int32 HousingLevel;

	// 저장 여부
	bool IsSavedData;

	// 수정 여부
	bool IsModifiedData;
};

/** 하우징 타일 */
class CRoomTilesData
{
public:
	CRoomTilesData() { Tiles.Empty(); }
	virtual ~CRoomTilesData() { Tiles.Empty(); }

public:
	/** 반환 : 현재 방에 대한 타일 정보 */
	TArray<struct FTileData>& GetRoomTilesData() { return Tiles; }

	/** 반환 : 현재 타일에 있는 아이템들 반환 */
	void GetSavedTilesData(FString strUniqueID, FVector vStartIndex, FVector vSize, TArray<FString>& arrReturn);

	/** 기본 타일 정보 초기화 */
	void InitRoomTileData(int32 RoomLevel);

	/** 검사 : 둘 수 있는 타일인지 */
	bool CheckEnableTileData(FVector vStartIndex, FVector vSize, FString strUniqueID = TEXT(""));

	/** 검사 : 타일 타입에 따른 z값 */
	bool CheckEnableTileDataZ(int32 iTileIndex, int32 iZindex, float fZSive, FString strUniqueID = TEXT(""));

	/** 검사 : 타일 타입 */
	bool CheckCompareTileType(FVector vStartIndex, FVector vSize, FString TableID);

	/** 새로운 타일 정보 저장 */
	void SaveNewRoomTileData(FVector vStartIndex, FVector vSize, FString strUniqueID);

	/** 타일 정보 제거 */
	void RemoveRoomTileData(FVector vStartIndex, FVector vSize);

private:
	TArray<struct FTileData> Tiles;

	// 방 인덱스
	int32 RoomIndex;

	// 방 크기
	FVector2D RoomSize;
};

/** 퀘스트 관련 정보 ********************************************************************************************************************/
class CQuestData
{
public:
	CQuestData() {}
	CQuestData(const FQuestData& Data);
	virtual ~CQuestData() {}

public:
	void SetAllMissionCount()
	{
		AllMissionCount = 0;

		if(MissionList.Num() > 0)
		{
			for(auto& iter : MissionList)
			{
				AllMissionCount += iter.m_MissionCount;
			}
		}
	}

public:
	uint32 QuestID;
	bool bIsComplete;							// 퀘스트 조건 완료
	bool bIsReceivedReward;						// 퀘스트 완료 (보상받음)

	EMissionType  MissionType;
	TArray<FMissionCountData> MissionList;		// 미션 정보

public:
	uint32 AllMissionCount;
};

/** 싱글모드 정보 **********************************************************************************************************************/
class CRPGModeData : public CData
{
public:
	CRPGModeData() { Clear(); };
	virtual ~CRPGModeData() { Clear(); };

public:
	virtual void Clear() override;

	/** 글리치 던전 입장 상태 체크 */
	void SetIsEnterCamp(bool bInEnterCamp) { bEnterCamp = bInEnterCamp; }
	bool IsEnterCamp() { return bEnterCamp; }

	/** 챕터 ID */
	FString GetChapterIDByString() const { return CurrentChapterID > 0 ? FString::FromInt(CurrentChapterID) : TEXT(""); }
	int32 GetChapterID() const { return CurrentChapterID; }
	void SetChapterID(int32 InID);

	/** 스테이지 ID */
	FString GetStageIDByString() const { return CurrentStageID > 0 ? FString::FromInt(CurrentStageID) : TEXT(""); }
	int32 GetStageID() const { return CurrentStageID; }
	void SetStageID(int32 InStageID) { CurrentStageID = InStageID; }

	/** 캠프에서 얻은 유물 정보 설정 */
	int32 GetCampArtifactID() const { return CampArtifactID; }
	void SetCampArtifactID(int32 InID) { CampArtifactID = InID; }

	/** 스페셜 레벨 */
	void SetSpecialLevelData(const TArray<FSpecialLevelData>& InLevelList);
	/** 현재 가능한 스페셜 레벨 인덱스 : -1인 경우, 사용 가능한 레벨X */
	int32 GetEnableSpecialLevelIndex() { return EnableSpecialLevelIndex; }

	/** 글리치 던전 모드 */
	void SetContentModeType(EContentsType ContentsType) { RpgContentsType = ContentsType; }
	EContentsType GetContentModeType() { return RpgContentsType; }

	/** 도전모드 데이터 세팅 */
	void SetRpgChallengeMode(FName SeasonID, EContentsType ContentType, TMap<int32, int32> mapChallengeData);

	FName GetChallengeSeasonID() { return ChallengeSeasonID; }
	TMap<EParameter, FVector2D> GetPlayerChallengeData() { return PlayerChallengeData; }
	TMap<EParameter, FVector2D> GetMonsterChallengeData() { return MonsterCallengeData; }

	/** 제거 : 플레이 정보 */
	void ClearPlayingData();

private:
	// 캠프로 입장 여부 (신규 플로우) 
	bool bEnterCamp;

	// 캠프에서 받은 유물ID
	int32 CampArtifactID;

	// 스테이지 관련
	int32 CurrentChapterID;
	int32 CurrentStageID;

	// 스페셜 레벨 정보
	int32 EnableSpecialLevelIndex;
	FSpecialLevelData EnableSpecialLevelData;

	//RPG 도전모드 데이터 관련
	EContentsType RpgContentsType;
	TMap<EParameter, FVector2D> PlayerChallengeData;
	TMap<EParameter, FVector2D> MonsterCallengeData;
	FName ChallengeSeasonID;
};

class CGlitchStoreData : public CData
{
public:
	CGlitchStoreData() { Clear(); };
	virtual ~CGlitchStoreData() { Clear(); };

public:
	virtual void Clear() override;

	TArray<int32> GetRestoredList(EProductType InType) const;
	bool GetIsHaveProduct(EProductType InType, int32 InID) const;

	void SetRestoredList(const TArray<int32>& InList);
	void AddRestoreData(const int32& InID);

private:
	TMap<EProductType, TArray<int32>> RestoredList;
};