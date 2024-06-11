// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Object/SDKObjectWithBeginPlay.h"
#include "SDKLevelSequenceManager.generated.h"

class ALevelSequenceActor;

DECLARE_LOG_CATEGORY_EXTERN(LogSequence, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFinishedLevelSequenceDelegate);


UCLASS(Blueprintable, BlueprintType)
class ARENA_API USDKLevelSequenceManager : public USDKObjectWithBeginPlay
{
	GENERATED_BODY()

public:
	bool IsValidPlay();

	// 플레이 : UI 숨김 처리 / 시퀀스 UI 활성화 / 인풋 막기 / 무기 숨기기 여부 / 캐릭터 숨기기 여부 / 바인딩 처리 관련
	UFUNCTION(BlueprintCallable)
	void SetLevelSequence(ALevelSequenceActor* InSequenceActor);

	UFUNCTION(BlueprintCallable)
	void PlayLevelSequence(bool bAllActorHidden = false);
	UFUNCTION()
	void FinishLevelSequence();

protected:
	void ResetCurrentLevelSequence();

public:
	UPROPERTY(BlueprintReadWrite, BlueprintAssignable)
	FOnFinishedLevelSequenceDelegate OnFinishedSequence;

public:
	static USDKLevelSequenceManager* Get();

private:
	UPROPERTY()
	ALevelSequenceActor* CurrentSequenceActor;

	UPROPERTY()
	bool IsPlaying;
};
