// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/SDKBTTask_Base.h"
#include "SDKBTTask_RotateToTarget.generated.h"

class AActor;
class UBlackboardComponent;

struct FSDKBTRotateToTaskMemory
{
	bool bIsLeft = false;
	FRotator MaxRotator = FAISystem::InvalidRotation;

	bool bAbortTask = false;
};

UCLASS()
class ARENA_API USDKBTTask_RotateToTarget : public USDKBTTask_Base
{
	GENERATED_BODY()
	
public:
	USDKBTTask_RotateToTarget(const FObjectInitializer& ObjectInitializer);

	// BEGIN UBTTaskNode
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult);
	virtual uint16 GetInstanceMemorySize() const override;
	// END UBTTaskNode

private:
	bool CheckRotateLeft(FRotator Current, FRotator Goal);

	FRotator GetNearyAxisRotator(const FRotator& InRotator);

private:
	UPROPERTY(EditAnywhere, Category = "Rotate", meta = (AllowPrivateAccess = "true"))
	float ErrorTolerance;

	UPROPERTY(EditAnywhere, Category = "Rotate", meta = (AllowPrivateAccess = "true"))
	bool bUseInterp;

	UPROPERTY(EditAnywhere, Category = "Rotate", meta = (EditCondition = "bUseInterp", AllowPrivateAccess = "true"))
	float RotateSpeed;

	UPROPERTY(EditAnywhere, Category = "Rotate", meta = (AllowPrivateAccess = "true"))
	bool bRotateNearyAxis;

	UPROPERTY(EditAnywhere, Category = "Rotate", meta = (AllowPrivateAccess = "true"))
	bool bRandomRotator;
};
