// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AITypes.h"
#include "AI/SDKBTTask_Base.h"
#include "SDKBTTask_MoveTo.generated.h"

class AActor;
class ACharacter;
class UBlackboardComponent;
class UBehaviorTreeComponent;
class ASDKAIController;

struct FSDKBTMoveToTaskMemory
{
	FAIRequestID MoveRequestID;
	FVector PreviousGoalLocation;
	uint8 bWaitingForPath : 1;

	bool bUseTime = false;
	float RemainTime = 0.0f;

	int32 FailedMoveCount = 0;
	float AttackableCheckRemainTime = 0.0f;
};


UCLASS()
class ARENA_API USDKBTTask_MoveTo : public USDKBTTask_Base
{
	GENERATED_BODY()
	
public:
	USDKBTTask_MoveTo(const FObjectInitializer& ObjectInitializer);

	// BEGIN UBTTaskNode
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult);

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR

	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void PostLoad() override;
	// END UBTTaskNode

private:
	EBTNodeResult::Type PerformMoveTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);

	void MoveToBlackboardKey(const UBlackboardComponent* BlackboardComp, FAIMoveRequest& OutMoveReq);
	void MoveToTarget(const UBlackboardComponent* BlackboardComp, FAIMoveRequest& OutMoveReq);
	void MoveToAroundTarget(const UBlackboardComponent* BlackboardComp, FAIMoveRequest& OutMoveReq);
	void MoveToAttackRangeToTarget(const UBlackboardComponent* BlackboardComp, const AActor* OwnerActor, FAIMoveRequest& OutMoveReq, float InRange);
	void MoveToForward(const UBlackboardComponent* BlackboardComp, const AActor* OwnerActor, FAIMoveRequest& OutMoveReq, float InRange);
	void MoveToPatrolPoint(const UBlackboardComponent* BlackboardComp, FAIMoveRequest& OutMoveReq);
	void MoveToRandom(const UBlackboardComponent* BlackboardComp, const AActor* OwnerActor, FAIMoveRequest& OutMoveReq);
	void MoveToSDKObjectByTag(const AActor* OwnerActor, FAIMoveRequest& OutMoveReq);
	void MoveToTime(const UBlackboardComponent* BlackboardComp, const AActor* OwnerActor, FAIMoveRequest& OutMoveReq, float InRange);
	void MoveToStepBack(const UBlackboardComponent* BlackboardComp, const AActor* OwnerActor, FAIMoveRequest& OutMoveReq);
	void MoveStrafe(const UBlackboardComponent* BlackboardComp, const AActor* OwnerActor, FAIMoveRequest& OutMoveReq);
	void MoveToLocation(const UBlackboardComponent* BlackboardComp, FAIMoveRequest& OutMoveReq);
	void MoveToTargetOppositeLocation(const UBlackboardComponent* BlackboardComp, const AActor* OwnerActor, FAIMoveRequest& OutMoveReq);

	AActor* GetTargetActor(const UBlackboardComponent* BlackboardComp);

	bool IsAttackable(ASDKAIController* InAIController, uint8* NodeMemory);

private:
	UPROPERTY()
	bool bAllowPartialPath;

	UPROPERTY()
	bool bUsePathfinding;

	UPROPERTY()
	bool bStopOnOverlapNeedsUpdate;

	UPROPERTY(Category = Node, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	bool bAllowStrafe;

	UPROPERTY(Category = Node, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UNavigationQueryFilter> FilterClass;

	UPROPERTY(Category = Node, EditAnywhere, AdvancedDisplay, meta = (AllowPrivateAccess = "true"))
	bool bAcceptanceRadius;

	UPROPERTY(Category = Node, EditAnywhere, AdvancedDisplay, meta = (AllowPrivateAccess = "true", EditCondition = "bAcceptanceRadius == false", EditConditionHides))
	bool bCustomAcceptanceRadius;

	UPROPERTY(Category = Node, EditAnywhere, AdvancedDisplay, meta = (AllowPrivateAccess = "true", EditCondition = "bCustomAcceptanceRadius == true && bAcceptanceRadius == false", EditConditionHides))
	float CustomAcceptanceRadius;
	
	UPROPERTY(Category = Node, EditAnywhere, AdvancedDisplay, meta = (AllowPrivateAccess = "true"))
	bool bTrackMovingGoal;

	UPROPERTY(Category = Node, EditAnywhere, AdvancedDisplay, meta = (AllowPrivateAccess = "true"))
	bool bProjectGoalLocation;

	UPROPERTY(Category = Node, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	bool bReachTestIncludesAgentRadius;
	
	UPROPERTY(Category = Node, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	bool bReachTestIncludesGoalRadius;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Init", meta = (AllowPrivateAccess = "true"))
	float MinStride;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Init", meta = (AllowPrivateAccess = "true"))
	float MovableStride;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Blackboard", meta = (AllowPrivateAccess = "true"))
	bool bBlackboardType;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Target", meta = (AllowPrivateAccess = "true"))
	bool bTargetType;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Target", meta = (AllowPrivateAccess = "true"))
	bool bUseTrackingTime;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Target", meta = (AllowPrivateAccess = "true"))
	float TrackingMinTime;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Target", meta = (AllowPrivateAccess = "true"))
	float TrackingMaxTime;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Around Target", meta = (AllowPrivateAccess = "true"))
	bool bAroundTargetType;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Around Target", meta = (EditCondition = "bAroundTargetType", AllowPrivateAccess = "true"))
	float AroundTargetRange;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Attack Range To Target", meta = (AllowPrivateAccess = "true"))
	bool bAttackRangeToTarget;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Forward", meta = (AllowPrivateAccess = "true"))
	bool bForwardType;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Forward", meta = (EditCondition = "bForwardType", AllowPrivateAccess = "true"))
	float ForwardRange;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Patrol Point", meta = (AllowPrivateAccess = "true"))
	bool bPatrolPointType;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Random", meta = (AllowPrivateAccess = "true"))
	bool bRandomType;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Random", meta = (EditCondition = "bRandomType", AllowPrivateAccess = "true"))
	float RandomRange;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Random", meta = (EditCondition = "bRandomType", AllowPrivateAccess = "true"))
	bool bRandomTimeType;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Random", meta = (EditCondition = "bRandomType", AllowPrivateAccess = "true"))
	float RandomLimitTime;

	UPROPERTY(EditAnywhere, Category = "MoveTo|SDKObject Tag", meta = (AllowPrivateAccess = "true"))
	bool bSDKObjectTagType;

	UPROPERTY(EditAnywhere, Category = "MoveTo|SDKObject Tag", meta = (EditCondition = "bSDKObjectTagType", AllowPrivateAccess = "true"))
	FName TagName;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Time", meta = (AllowPrivateAccess = "true"))
	bool bTimeType;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Time", meta = (EditCondition = "bTimeType", AllowPrivateAccess = "true"))
	float TimeMin;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Time", meta = (EditCondition = "bTimeType", AllowPrivateAccess = "true"))
	float TimeMax;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Step Back", meta = (AllowPrivateAccess = "true"))
	bool bStepBackType;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Strafe", meta = (AllowPrivateAccess = "true"))
	bool bStrafeType;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Location", meta = (AllowPrivateAccess = "true"))
	bool bDestinationType;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Opposite", meta = (AllowPrivateAccess = "true"))
	bool bTargetOppositeType;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Attackable", meta = (AllowPrivateAccess = "true"))
	bool bAttackableDestination;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Attackable", meta = (EditCondition = "bAttackableDestination == true"))
	float AttackableCheckCooltime;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Attackable", meta = (EditCondition = "bAttackableDestination == true"))
	bool bUseBoxTrace;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Attackable", meta = (EditCondition = "bAttackableDestination == true && bUseBoxTrace == true"))
	FVector TraceBoxExtent;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Opposite", meta = (EditCondition = "bTargetOppositeType", AllowPrivateAccess = "true"))
	float OppositeRange;

	UPROPERTY(EditAnywhere, Category = "MoveTo|Opposite", meta = (EditCondition = "bTargetOppositeType", AllowPrivateAccess = "true"))
	float MoveTime;

	int32 FailedMoveMaxCount;
};
