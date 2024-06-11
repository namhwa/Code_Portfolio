// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/SDKRpgGameMode.h"
#include "SDKRpgTutorialGameMode.generated.h"


UCLASS()
class ARENA_API ASDKRpgTutorialGameMode : public ASDKRpgGameMode
{
	GENERATED_BODY()

public:
	// Begin GameMode Interface
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	// End GameMode Interface

	// Begin SDKGameMode Interface
	virtual void FinishGame() override;
	// End SDKGameMode Interface

	// Begin SDKRpgGameMode Interface
	virtual void LoadLevel(const int InColumn);
	virtual void ClearCurrentRoom() override {};
	virtual void ActiveRoomReward(bool bHaveSector) override;
	virtual void CompletedLoadedRoguelikeSector() override;

protected:
	virtual FString CreateStreamInstance(UWorld* World, const FString& LongPackageName, const FName& LevelName, int32 Index, const FVector Location, const FRotator Rotation);
	virtual void CreateObjectStreamLevel(FString ObjectLevelPath, bool bIsSubRoomLevel = false);

	virtual void InitShuffleRoom() override;
	virtual FString GetMakedLevelID(FName RoomTypeName) override;
	// End SDKRpgGameMode Interface

	void LoadNextRoomLevel();

public:
	UFUNCTION(BlueprintCallable)
	void CompletedRpgTutorial();

protected:
	// 선택한 컬럼
	UPROPERTY()
	int32 SelectColumn;
};
