// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Share/SDKEnum.h"
#include "Share/SDKData.h"
#include "Share/SDKDataStruct.h"
#include "SDKStruct.generated.h"


USTRUCT(BlueprintType)
struct FServerSendData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	APlayerController* NewPlayer;

	UPROPERTY()
	FString NickName;

	UPROPERTY()
	FString TeamID;

	UPROPERTY()
	int32 SelectHeroID;

	UPROPERTY()
	int32 SelectHairSkinID;

	UPROPERTY()
	int32 SelectBodySkinID;

	UPROPERTY()
	int32 EquipWeaponID;

	UPROPERTY()
	int64 SelectPetID;

	UPROPERTY()
	int32 SelectPetTableID;

	UPROPERTY()
	int32 PetLike;

	UPROPERTY()
	int32 SelectVehicleID;

	UPROPERTY()
	int32 UserID;

	TArray<class CItemData> UseItemData;

	// 특성
	UPROPERTY()
	TArray<FAttributeData> AttributeList;

	UPROPERTY()
	FPlayerHeroSkinData HeroSkinData;

	UPROPERTY()
	FPlayerHeroData HeroData;

	UPROPERTY()
	TArray<FPlayerArtifactData> ArtifactListData;

	UPROPERTY()
	TArray<int32> PetAbility;

	TMap<EParameter, FVector2D> mapAttAbility;
	TMap<EParameter, FVector2D> MapAbility;
	TMap<FString, TMap<EParameter, FVector2D>> mapMonsterAbility;
	TMap<int32, int32> WeaponPropertyData;

public:
	FServerSendData()
	{
		NewPlayer = nullptr;
		NickName.Empty();
		TeamID.Empty();
		SelectHeroID = 0;
		SelectHairSkinID = 0;
		SelectBodySkinID = 0;
		EquipWeaponID = 0;
		SelectPetID = 0;
		SelectPetTableID = 0;
		SelectVehicleID = 0;
		UserID = 0;
		PetLike = 0;

		UseItemData.Empty();

		HeroSkinData = FPlayerHeroSkinData();
		HeroData = FPlayerHeroData();
		ArtifactListData = TArray<FPlayerArtifactData>();

		mapAttAbility.Empty();
		MapAbility.Empty();
		mapMonsterAbility.Empty();
		WeaponPropertyData.Empty();
		PetAbility.Empty();
	}
};

/** 하우징 ***********************************/
USTRUCT(BlueprintType)
struct FTileData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, Category = Data)
	ETileType TileType;

	UPROPERTY(VisibleAnywhere, Category = Data)
	TArray<FString> Heights;

	FTileData()
	{
		TileType = ETileType::None;
		Heights.AddZeroed(MAX_ROOM_HEIGHT);
	}

	FTileData(ETileType newType)
	{
		TileType = newType;
		Heights.AddZeroed(MAX_ROOM_HEIGHT);
	}

	FTileData(int64 newUnique, FString newTable, ETileType newType)
	{
		TileType = newType;
		Heights.AddZeroed(MAX_ROOM_HEIGHT);
	}
};

USTRUCT(BlueprintType)
struct FFurnitureGroup
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, Category = Data)
	ASDKFurniture* BaseFurniture;

	UPROPERTY(VisibleAnywhere, Category = Data)
	TArray<ASDKFurniture*> Decorations;

	FFurnitureGroup()
	{
		Clear();
	}

	FFurnitureGroup(ASDKFurniture* newFurniture)
	{
		BaseFurniture = newFurniture;
		Decorations.Empty();
	}

	FFurnitureGroup(ASDKFurniture* newFurniture, TArray<ASDKFurniture*> arrDecoFurnitures)
	{
		BaseFurniture = newFurniture;
		Decorations = arrDecoFurnitures;
	}

	void Remove()
	{
		if (BaseFurniture != nullptr)
		{
			BaseFurniture->SetLifeSpan(0.01f);
		}

		for (auto& iter : Decorations)
		{
			iter->SetLifeSpan(0.01f);
		}

		BaseFurniture = nullptr;
		Decorations.Empty();
	}

	void Clear()
	{
		BaseFurniture = nullptr;
		Decorations.Empty();
	}

	friend bool operator==(const FFurnitureGroup& FurnitureGroup, const FFurnitureGroup& CompareFurniture)
	{
		return FurnitureGroup.BaseFurniture == CompareFurniture.BaseFurniture;
	}
};

USTRUCT(BlueprintType)
struct FPreviewGroup
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, Category = Data)
	ASDKPreviewFurniture* BaseFurniture;

	UPROPERTY(VisibleAnywhere, Category = Data)
	TArray<ASDKPreviewFurniture*> Decorations;

	FPreviewGroup()
	{
		BaseFurniture = nullptr;
		Decorations.Empty();
	}

	FPreviewGroup(ASDKPreviewFurniture* newFurniture)
	{
		BaseFurniture = newFurniture;
		Decorations.Empty();
	}

	FPreviewGroup(ASDKPreviewFurniture* newFurniture, TArray<ASDKPreviewFurniture*> arrDecoFurnitures)
	{
		BaseFurniture = newFurniture;
		Decorations = arrDecoFurnitures;
	}

	void Remove()
	{
		if (BaseFurniture != nullptr)
		{
			BaseFurniture->SetLifeSpan(0.01f);
		}

		for (auto& iter : Decorations)
		{
			iter->SetLifeSpan(0.01f);
		}

		BaseFurniture = nullptr;
		Decorations.Empty();
	}

	friend bool operator==(const FPreviewGroup& FurnitureGroup, const FPreviewGroup& CompareFurniture)
	{
		return FurnitureGroup.BaseFurniture == CompareFurniture.BaseFurniture;
	}
};

USTRUCT(BlueprintType)
struct FWallPreset
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = Data)
	TArray<int32> Walls;

	FWallPreset()
	{
		Walls.Empty();
	}
};

/** 싱글모드 *********************************/
USTRUCT(BlueprintType)
struct FRoomData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	ERoomType RoomType;

	UPROPERTY()
	int32 StageID;

	UPROPERTY()
	int32 Row;

	UPROPERTY()
	int32 Column;

	UPROPERTY()
	TMap<int32, ERoomType> NextRoomTypes;
};

USTRUCT(BlueprintType)
struct FRpgSectorData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString LevelID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RewardID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RoomIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERoomType RoomType;

	FRpgSectorData()
	{
		LevelID.Empty();
		RewardID.Empty();
		RoomIndex = -1;
		RoomType = ERoomType::None;
	}

	FRpgSectorData(FString level, FString reward, int32 index, ERoomType type)
	{
		LevelID = level;
		RewardID = reward;
		RoomIndex = index;
		RoomType = type;
	}
};

USTRUCT(BlueprintType)
struct FLevelSet
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	int32 Row;

	UPROPERTY()
	int32 Stage;

	UPROPERTY()
	TArray<int32> Columns;

	UPROPERTY()
	TArray<bool> HiddenStates;

	FLevelSet()
	{
		Row = -1;
		Stage = -1;
		Columns.Empty();
		HiddenStates.Empty();
	}

	FLevelSet(int32 irow)
	{
		Row = irow;
		Stage = -1;
		Columns.Empty();
		HiddenStates.Empty();
	}
};

USTRUCT(BlueprintType)
struct FRoomClearData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	int32 LevelID = 0;
	UPROPERTY()
	int32 CharacterLevel = 1;
	UPROPERTY()
	int32 ObjectCount = 0;
	UPROPERTY()
	TArray<int32> WeaponList;
	UPROPERTY()
	TArray<int32> ArtifactList;
	UPROPERTY()
	TArray<int32> LevelupBuffList;
	UPROPERTY()
	TMap<int32, int32> MapItem;
	UPROPERTY()
	TMap<FString, int32> MapMonster;
	UPROPERTY()
	int32 CurrentGold = 0;
	UPROPERTY()
	int32 ConsumeGold = 0;
	UPROPERTY()
	bool IsRoomClear = false;

	FRoomClearData() = default;
};

USTRUCT(BlueprintType)
struct FRoguelikeResultData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = Data)
	int32 Floor;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	int32 Gold;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	int32 DreamJewel;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	int32 KillCount;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	int32 PlayingTime;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	float ItemConvertRate;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	float GoldConvertRate;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	int32 GoldCalcRate;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	int32 DiamondCalcRate;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	TArray<FString> RewardItems;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	TMap<FString, int32> RewardItemCount;

	FRoguelikeResultData()
	{
		Floor = 0;
		Gold = 0;
		DreamJewel = 0;
		KillCount = 0;
		PlayingTime = 0;
		ItemConvertRate = 1.f;
		GoldConvertRate = 0.05f;
		GoldCalcRate = 20;
		DiamondCalcRate = 1;
		RewardItems.Empty();
		RewardItemCount.Empty();
	}

	FRoguelikeResultData(int32 newGold, int32 newJewel, int32 newKill, int32 newTime)
	{
		Gold = newGold;
		DreamJewel = newJewel;
		KillCount = newKill;
		PlayingTime = newTime;
		ItemConvertRate = 1.f;
		GoldConvertRate = 0.05f;
		GoldCalcRate = 20;
		DiamondCalcRate = 1;
		RewardItems.Empty();
		RewardItemCount.Empty();
	}

	void AddRewardItemID(FString newItemID)
	{
		RewardItems.Add(newItemID);
	}
};

/** 멀티모드 *********************************/
USTRUCT(BlueprintType)
struct FPartyMemberData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, Category = Data)
	int32 SpawnNumber;

	UPROPERTY(VisibleAnywhere, Category = Data)
	FString NickName;

	UPROPERTY(VisibleAnywhere, Category = Data)
	int32 TableID;

	UPROPERTY(VisibleAnywhere, Category = Data)
	int32 RespawnTime;

	UPROPERTY(VisibleAnywhere, Category = Data)
	int32 Level;

	UPROPERTY(VisibleAnywhere, Category = Data)
	float Hp;

	UPROPERTY(VisibleAnywhere, Category = Data)
	float MaxHp;

	UPROPERTY(VisibleAnywhere, Category = Data)
	float Sp;

	UPROPERTY(VisibleAnywhere, Category = Data)
	float MaxSp;

	UPROPERTY(VisibleAnywhere, Category = Data)
	ECharacterState State;

	UPROPERTY(VisibleAnywhere, Category = Data)
	FVector Location;

	FPartyMemberData()
	{
		SpawnNumber = 0;
		NickName = TEXT("");
		TableID = 0;
		RespawnTime = 0;
		Level = 0;
		Hp = 0.f;
		MaxHp = 0.f;
		Sp = 0.f;
		MaxSp = 0;
		State = ECharacterState::None;
		Location = FVector::ZeroVector;
	}

	friend bool operator==(const FPartyMemberData& Lhs, const FPartyMemberData& Rhs)
	{
		return Lhs.SpawnNumber == Rhs.SpawnNumber;
	}
};

USTRUCT(BlueprintType)
struct FPlayerGameInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	FString Nickname;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	int32 TeamNumber;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	int32 SpawnNumber;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	int32 HeroID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	int32 HeroSkinID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	int32 WeaponID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	int32 Level;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	int32 Exp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	int32 Coin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	int32 NumOfKill;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	int32 NumOfMonsterKill;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	int32 NumOfDeath;

	FPlayerGameInfo()
	{
		Nickname = TEXT("");
		TeamNumber = 0;
		SpawnNumber = 0;
		HeroID = 0;
		HeroSkinID = 0;
		WeaponID = 0;
		Level = 1;
		Exp = 0;
		Coin = 0;
		NumOfKill = 0;
		NumOfMonsterKill = 0;
		NumOfDeath = 0;
	}

	FPlayerGameInfo(FString NewName, int32 NewTeamNum, int32 NewSpawnNum, int32 NewHeroID, int32 NewSkinID, int32 NewWeaponID)
	{
		Nickname = NewName;
		TeamNumber = NewTeamNum;
		SpawnNumber = NewSpawnNum;
		HeroID = NewHeroID;
		HeroSkinID = NewSkinID;
		WeaponID = NewWeaponID;
		Level = 1;
		Exp = 0;
		Coin = 0;
		NumOfKill = 0;
		NumOfMonsterKill = 0;
		NumOfDeath = 0;
	}
};

USTRUCT(BlueprintType)
struct FTeamMemberInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = Data)
	int32 Level;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	int32 RankScore;

	UPROPERTY(BlueprintReadOnly, Category = Data)
	int32 SelectHeroID;

	FTeamMemberInfo()
	{
		Level = 1;
		RankScore = 0;
		SelectHeroID = 1;
	}
};

USTRUCT()
struct FRankPointData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	int32 TeamNumber = 0;
	UPROPERTY()
	int32 CurrentRankPoint = 0;
	UPROPERTY()
	int32 WinningStreak = 0;
	UPROPERTY()
	int32 KillCount = 0;
	UPROPERTY()
	int32 DeathCount = 0;

public:
	FRankPointData() = default;
	FRankPointData(int32 InTeamNum, int32 InRankPoint, int32 InWinningStreak, int32 InKill, int32 InDeath)
	{
		TeamNumber = InTeamNum;
		CurrentRankPoint = InRankPoint;
		WinningStreak = InWinningStreak;
		KillCount = InKill;
		DeathCount = InDeath;
	}
};

USTRUCT(BlueprintType)
struct FGoldClashResultInfo
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	EGameMode GameMode;

	UPROPERTY()
	int32 UserID;

	UPROPERTY()
	int32 TeamNumber;

	UPROPERTY()
	int32 CurrentCoin;

	UPROPERTY()
	int32 HeroID;

	UPROPERTY()
	FString Nickname;

	UPROPERTY()
	TArray<int32> InfoList;

	UPROPERTY()
	FPlayerHeroData HeroData;

	UPROPERTY()
	FPlayerHeroSkinData HeroSkinData;

	UPROPERTY()
	int32 RankPoint;

	UPROPERTY()
	int32 TotalRankPoint;

	UPROPERTY()
	bool bCustomMatch;

	UPROPERTY()
	bool bDodge;

	UPROPERTY()
	float LimitTime;

	UPROPERTY()
	TArray<int32> ExpressionAutoPlayList;

	UPROPERTY()
	int32 Ranking;

public:
	FGoldClashResultInfo()
	{
		GameMode = EGameMode::None;
		UserID = 0;
		TeamNumber = 0;
		CurrentCoin = 0;
		HeroID = 0;
		Nickname.Empty();

		InfoList.Empty();
		RankPoint = 0;
		TotalRankPoint = 0;
		bCustomMatch = false;
		bDodge = false;
		LimitTime = 0.f;
		Ranking = 0;
	}

	FGoldClashResultInfo(int32 InUserID, int32 TeamNum, int32 InCurCoin, FString NickName)
	{
		UserID = InUserID;
		TeamNumber = TeamNum;
		CurrentCoin = InCurCoin;
		Nickname = NickName;

		HeroID = 0;
		InfoList.Empty();
		RankPoint = 0;
		bCustomMatch = false;
		bDodge = false;
		LimitTime = 0.f;
	}

	FGoldClashResultInfo(
		EGameMode InGameMode,
		FPlayerGameInfo Info,
		int32 Level,
		int32 InCurCoin,
		int32 InGetCoin,
		int32 InUseCoin,
		int32 InItemCnt,
		EItemGrade InRune,
		const FPlayerHeroSkinData& InSkinData,
		const FPlayerHeroData& InHeroData,
		int32 InRankPoint,
		int32 InTotalRankPoint,
		bool bInCustomMatchID,
		TArray<int32> InExpressionAutoPlayList,
		int32 InRanking)
	{
		GameMode = InGameMode;

		UserID = Info.SpawnNumber;
		TeamNumber = Info.TeamNumber;
		CurrentCoin = InCurCoin;
		HeroID = Info.HeroID;
		Nickname = Info.Nickname;
		HeroData = InHeroData;
		HeroSkinData = InSkinData;
		RankPoint = InRankPoint;
		TotalRankPoint = InTotalRankPoint;
		bCustomMatch = bInCustomMatchID;
		bDodge = false;
		LimitTime = 0.f;

		ExpressionAutoPlayList = InExpressionAutoPlayList;

		InfoList.Empty();

		InfoList.Add(Level);
		InfoList.Add(InGetCoin);
		InfoList.Add(InUseCoin);
		InfoList.Add(InItemCnt);
		InfoList.Add(static_cast<int32>(InRune));

		Ranking = InRanking;
	}
};

USTRUCT(BlueprintType)
struct FGoldClashResultUIInfo
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	int32 WinTeam;

	UPROPERTY()
	EGameMode GameMode;

	UPROPERTY()
	int32 PlayingTime;

	UPROPERTY()
	bool bCustomGame;

	UPROPERTY()
	FGoldClashResultInfo MyResultInfo;

	UPROPERTY()
	TArray<FGoldClashResultInfo> ResultInfoList;

	UPROPERTY()
	TArray<FGoldClashMatchInfo> MatchInfoList;

	UPROPERTY()
	TArray<FGoldClashBoardStatisticsInfo> StatisticList;

public:
	FGoldClashResultUIInfo()
	{
		WinTeam = 0;
		GameMode = EGameMode::None;
		PlayingTime = 0;
		bCustomGame = false;

		MyResultInfo = FGoldClashResultInfo();
		ResultInfoList.Empty();
		MatchInfoList.Empty();
		StatisticList.Empty();
	}

	void Clear()
	{
		WinTeam = 0;
		GameMode = EGameMode::None;
		PlayingTime = 0;
		bCustomGame = false;

		MyResultInfo = FGoldClashResultInfo();
		ResultInfoList.Empty();
		MatchInfoList.Empty();
		StatisticList.Empty();
	}
};

USTRUCT(BlueprintType)
struct FResultGoldData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	int32 UserID = 0;

	UPROPERTY()
	int32 Gold = 0;

	UPROPERTY()
	float GainGoldTime = 0.f;

public:
	FResultGoldData()
	{
		UserID = 0;
		Gold = 0;
		GainGoldTime = 0.f;
	}
	FResultGoldData(int32 InUserID, int32 InGold, float InGainGoldTime)
	{
		UserID = InUserID;
		Gold = InGold;
		GainGoldTime = InGainGoldTime;
	}
};

USTRUCT(BlueprintType)
struct FSendGoldClashResult
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	int32 UID = 0;

	UPROPERTY()
	bool bWinTeam = false;

	UPROPERTY()
	int32 Ranking = 0;

	UPROPERTY()
	int32 RankingPoint = 0;

	UPROPERTY()
	int32 HeroID = 0;

	UPROPERTY()
	int32 PartyID = 0;	// 팀 넘버

	UPROPERTY()
	FString GoldClashMapID;

	UPROPERTY()
	TArray<EGuideMissionType> GuideMissionCompleteList;

	UPROPERTY()
	TArray<FSendMissionData> MissionList;

	UPROPERTY()
	TArray<FBiskitLogData> BiskitLog;

	UPROPERTY()
	bool bCustomMatch = false;

	UPROPERTY()
	bool bDodge = false;


	UPROPERTY()
	int32 Kill = 0;
	UPROPERTY()
	int32 Death = 0;
	UPROPERTY()
	int32 Gold = 0; // 1분당 평균 골드 획득량
	UPROPERTY()
	int32 Deal = 0; // 1분당 평균 딜량
	UPROPERTY()
	int32 Heal = 0; // 1분당 평균 힐량
	UPROPERTY()
	int32 BlockDamage = 0; // 1분당 평균 받은 피해량
	UPROPERTY()
	bool IsMvp = false; // Mvp 획득 여부
};

UCLASS()
class ARENA_API UGoldClashUserData : public UUserData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString TeamID;

	UPROPERTY()
	FString MatchID;

	UPROPERTY()
	int64 PetUniqueID;

	UPROPERTY()
	int32 PetTableID;

	UPROPERTY()
	int32 PetLike;

	UPROPERTY()
	TArray<int32> PetAbility;

	UPROPERTY()
	int32 RankingPoint;

	UPROPERTY()
	int32 WinningStreak;

	UPROPERTY()
	FPlayerHeroData HeroData;

	UPROPERTY()
	FPlayerHeroSkinData HeroSkinData;

	UPROPERTY()
	TArray<FPlayerArtifactData> DeckArtifactList;

	UPROPERTY()
	TArray<FPlayerArtifactData> ArtifactList;

	UPROPERTY()
	TArray<int32> AutoPlayExpressionList;

public:
	UGoldClashUserData() : UUserData()
	{
		TeamID = FString();
		MatchID = FString();

		PetUniqueID = 0;
		PetTableID = 0;
		PetLike = 0;
		RankingPoint = 0;
		WinningStreak = 0;

		HeroData = FPlayerHeroData();
		HeroSkinData = FPlayerHeroSkinData();

		ArtifactList.Empty();
		DeckArtifactList.Empty();
	}
};