// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/SDKBTTask_MoveTo.h"

#include "BrainComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "AI/SDKAIController.h"
#include "AISystem.h"
#include "NavigationSystem.h"
#include "AI/Navigation/NavigationTypes.h"
#include "Blueprint/AIAsyncTaskBlueprintProxy.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Logging/LogMacros.h"
#include "Navigation/PathFollowingComponent.h"

#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Object/SDKObject.h"

#include "Character/SDKNonePlayerCharacter.h"
#include "DrawDebugHelpers.h"

#include "Stats/Stats.h"
DECLARE_CYCLE_STAT(TEXT("USDKBTTask_MoveTo::TickTask"), STAT_USDKBTTask_MoveTo_TickTask, STATGROUP_AI);

USDKBTTask_MoveTo::USDKBTTask_MoveTo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("SDK Move To");
	bNotifyTick = true;
	bNotifyTaskFinished = true;
	BlackboardKey = FBlackboardKeySelector();

	bReceiveFinishTaskEvent = true;

	bAllowPartialPath = false;
	bUsePathfinding = true;
	bStopOnOverlapNeedsUpdate = true;

	bAllowStrafe = false;
	bAcceptanceRadius = true;

	bCustomAcceptanceRadius = false;
	CustomAcceptanceRadius = 0.0f;

	bTrackMovingGoal = true;
	bProjectGoalLocation = true;
	bReachTestIncludesAgentRadius = true;
	bReachTestIncludesGoalRadius = true;

	MinStride = 300.f;
	MovableStride = 500.f;

	bBlackboardType = false;

	bTargetType = false;
	bUseTrackingTime = false;
	TrackingMinTime = 0.0f;
	TrackingMaxTime = 0.0f;

	bAroundTargetType = false;
	AroundTargetRange = 0.0f;

	bAttackRangeToTarget = false;

	bForwardType = false;
	ForwardRange = 0.0f;

	bPatrolPointType = false;

	bRandomType = false;
	RandomRange = 0.0f;
	bRandomTimeType = false;
	RandomLimitTime = 0.0f;

	bSDKObjectTagType = false;
	TagName = FName("");

	bTimeType = false;
	TimeMin = 0.0f;
	TimeMax = 0.0f;

	bStepBackType = false;

	bStrafeType = false;
	bDestinationType = false;

	bTargetOppositeType = false;
	OppositeRange = 300.0f;
	MoveTime = 1.0f;

	bAttackableDestination = false;
	AttackableCheckCooltime = 0.3f;
	bUseBoxTrace = false;
	TraceBoxExtent = FVector(20.0f, 20.0f, 20.0f);

	FailedMoveMaxCount = 20;
}

EBTNodeResult::Type USDKBTTask_MoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);
	EBTNodeResult::Type NodeResult = EBTNodeResult::InProgress;

	FSDKBTMoveToTaskMemory* MyMemory = CastInstanceNodeMemory<FSDKBTMoveToTaskMemory>(NodeMemory);
	MyMemory->PreviousGoalLocation = FAISystem::InvalidLocation;
	MyMemory->MoveRequestID = FAIRequestID::InvalidRequest;

	ASDKAIController* SDKAIController = Cast<ASDKAIController>(OwnerComp.GetAIOwner());
	if(!IsValid(SDKAIController))
	{
		return EBTNodeResult::Failed;
	}

	if (!SDKAIController->IsPossibleMove())
	{
		return EBTNodeResult::Failed;
	}

	MyMemory->bWaitingForPath = SDKAIController->ShouldPostponePathUpdates();
	if (!MyMemory->bWaitingForPath)
	{
		NodeResult = PerformMoveTask(OwnerComp, NodeMemory);
	}
	else
	{
		UE_LOG(LogBehaviorTree, Log, TEXT("Pathfinding requests are freezed, waiting..."));
	}

	ASDKCharacter* OwnerCharacter = Cast<ASDKCharacter>(SDKAIController->GetPawn());
	if (IsValid(OwnerCharacter))
	{
		ECharacterState CurrentState = OwnerCharacter->GetCurrentState();
		if (CurrentState != ECharacterState::None &&
			CurrentState != ECharacterState::Spawn &&
			CurrentState != ECharacterState::Skill)
		{
			OwnerCharacter->SetCurrentState(ECharacterState::Move);
		}
	}

	return NodeResult;
}

EBTNodeResult::Type USDKBTTask_MoveTo::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const FSDKBTMoveToTaskMemory* MyMemory = CastInstanceNodeMemory<FSDKBTMoveToTaskMemory>(NodeMemory);
	if (MyMemory->bWaitingForPath == false)
	{
		if (MyMemory->MoveRequestID.IsValid())
		{
			const AAIController* AIController = OwnerComp.GetAIOwner();
			if (IsValid(AIController) && AIController->GetPathFollowingComponent())
			{
				AIController->GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::OwnerFinished, MyMemory->MoveRequestID);

				const auto OwnerCharacter = Cast<ASDKCharacter>(AIController->GetPawn());
				if (IsValid(OwnerCharacter))
				{
					OwnerCharacter->SetCurrentState(ECharacterState::Idle);
				}
			}
		}
	}

	return Super::AbortTask(OwnerComp, NodeMemory);
}

void USDKBTTask_MoveTo::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (IsValid(AIController))
	{
		AIController->StopMovement();

		const auto OwnerCharacter = Cast<ASDKCharacter>(AIController->GetPawn());
		if (IsValid(OwnerCharacter))
		{
			OwnerCharacter->SetCurrentState(ECharacterState::Idle);
		}
	}
}

void USDKBTTask_MoveTo::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_USDKBTTask_MoveTo_TickTask);
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("USDKBTTask_MoveTo::TickTask"));
	ASDKAIController* AIController = Cast<ASDKAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(AIController))
	{
		return;
	}

	FSDKBTMoveToTaskMemory* MyMemory = reinterpret_cast<FSDKBTMoveToTaskMemory*>(NodeMemory);
	if (MyMemory->bUseTime == true)
	{
		MyMemory->RemainTime -= DeltaSeconds;

		if (MyMemory->RemainTime <= -5.f)
		{
			// 혹시 루프 돌 경우를 위함
			AIController->StopMovement();
			FinishLatentAbort(OwnerComp);
			return;
		}

		if (MyMemory->RemainTime <= 0.f)
		{
			AIController->StopMovement();
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
	}

	if (bAttackableDestination)
	{
		MyMemory->AttackableCheckRemainTime -= DeltaSeconds;
		if (MyMemory->AttackableCheckRemainTime < 0.0f)
		{
			if (IsAttackable(AIController, NodeMemory))
			{
				AIController->StopMovement();
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
				return;
			}
			MyMemory->AttackableCheckRemainTime = AttackableCheckCooltime;
		}
	}

	if (MyMemory->bWaitingForPath && !OwnerComp.IsPaused())
	{
		if (!AIController->ShouldPostponePathUpdates())
		{
			UE_LOG(LogBehaviorTree, Log, TEXT("Pathfinding requests are unlocked!"));
			MyMemory->bWaitingForPath = false;

			const EBTNodeResult::Type NodeResult = PerformMoveTask(OwnerComp, NodeMemory);
			if (NodeResult != EBTNodeResult::InProgress)
			{
				FinishLatentTask(OwnerComp, NodeResult);
			}
		}
	}
}

uint16 USDKBTTask_MoveTo::GetInstanceMemorySize() const
{
	return sizeof(FSDKBTMoveToTaskMemory);
}

void USDKBTTask_MoveTo::PostLoad()
{
	Super::PostLoad();

	if (bStopOnOverlapNeedsUpdate)
	{
		bStopOnOverlapNeedsUpdate = false;
		bReachTestIncludesGoalRadius = false;
	}
}

#if WITH_EDITOR
FName USDKBTTask_MoveTo::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Task.MoveTo.Icon");
}
#endif // WITH_EDITOR

EBTNodeResult::Type USDKBTTask_MoveTo::PerformMoveTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!IsValid(BlackboardComponent))
	{
		return EBTNodeResult::Failed;
	}

	FSDKBTMoveToTaskMemory* MyMemory = CastInstanceNodeMemory<FSDKBTMoveToTaskMemory>(NodeMemory);
	if (!MyMemory)
	{
		return EBTNodeResult::Failed;
	}

	ASDKAIController* AIController = Cast<ASDKAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(AIController))
	{
		return EBTNodeResult::Failed;
	}
	ASDKCharacter* OwnerActor = AIController->GetPawn<ASDKCharacter>();
	if (!IsValid(OwnerActor))
	{
		return EBTNodeResult::Failed;
	}

	EBTNodeResult::Type NodeResult = EBTNodeResult::Failed;
	FAIMoveRequest MoveReq;
	MoveReq.SetNavigationFilter(*FilterClass ? FilterClass : AIController->GetDefaultNavigationFilterClass());
	MoveReq.SetAllowPartialPath(bAllowPartialPath);
	MoveReq.SetCanStrafe(bAllowStrafe);
	MoveReq.SetReachTestIncludesAgentRadius(bReachTestIncludesAgentRadius);
	MoveReq.SetReachTestIncludesGoalRadius(bReachTestIncludesGoalRadius);
	MoveReq.SetProjectGoalLocation(bProjectGoalLocation);
	MoveReq.SetUsePathfinding(bUsePathfinding);

	float AcceptRadius = 0.0f;
	if (bAcceptanceRadius)
	{
		AcceptRadius = OwnerActor->GetCapsuleComponent()->GetScaledCapsuleRadius();
	}
	else
	{
		if (bCustomAcceptanceRadius)
		{
			AcceptRadius = CustomAcceptanceRadius;
		}
	}
	MoveReq.SetAcceptanceRadius(AcceptRadius);

	if (OwnerActor->GetCurrentState() == ECharacterState::Confusion)
	{
		MoveToRandom(BlackboardComponent, OwnerActor, MoveReq);
	}
	else if (bBlackboardType == true)
	{
		MoveToBlackboardKey(BlackboardComponent, MoveReq);
	}
	else if (bTargetType == true)
	{
		MoveToTarget(BlackboardComponent, MoveReq);

		if (bUseTrackingTime)
		{
			MyMemory->bUseTime = true;
			MyMemory->RemainTime = FMath::RandRange(TrackingMinTime, TrackingMaxTime);
		}
	}
	else if (bAroundTargetType == true)
	{
		MoveToAroundTarget(BlackboardComponent, MoveReq);
	}
	else if (bAttackRangeToTarget == true)
	{
		MoveToAttackRangeToTarget(BlackboardComponent, OwnerActor, MoveReq, OwnerActor->GetPawnData()->AttackDistance);
	}
	else if (bForwardType == true)
	{
		float Range = ForwardRange;
		if (IsValid(OwnerActor))
		{
			Range += OwnerActor->GetCapsuleComponent()->GetScaledCapsuleRadius() + AcceptRadius;
		}

		MoveToForward(BlackboardComponent, OwnerActor, MoveReq, Range);
	}
	else if (bPatrolPointType == true)
	{
		MoveToPatrolPoint(BlackboardComponent, MoveReq);
	}
	else if (bRandomType == true)
	{
		MoveToRandom(BlackboardComponent, OwnerActor, MoveReq);
		if (bRandomTimeType == true)
		{
			MyMemory->bUseTime = true;
			MyMemory->RemainTime = RandomLimitTime;
		}
	}
	else if (bSDKObjectTagType == true)
	{
		MoveToSDKObjectByTag(OwnerActor, MoveReq);
	}
	else if (bTimeType == true)
	{
		MyMemory->bUseTime = true;
		MyMemory->RemainTime = FMath::RandRange(TimeMin, TimeMax);

		MoveToTime(BlackboardComponent, OwnerActor, MoveReq, OwnerActor->GetPawnData()->AttackDistance);
	}
	else if (bStepBackType == true)
	{
		MoveToStepBack(BlackboardComponent, OwnerActor, MoveReq);
	}
	else if (bStrafeType == true)
	{
		MoveStrafe(BlackboardComponent, OwnerActor, MoveReq);
	}
	else if (bDestinationType == true)
	{
		MoveToLocation(BlackboardComponent, MoveReq);
	}
	else if (bTargetOppositeType == true)
	{
		MoveToTargetOppositeLocation(BlackboardComponent, OwnerActor, MoveReq);
		MyMemory->bUseTime = true;
		MyMemory->RemainTime = MoveTime;
	}
	else if (bAttackableDestination == true)
	{
		MoveToTarget(BlackboardComponent, MoveReq);
		if (IsAttackable(AIController, NodeMemory))
		{
			return EBTNodeResult::Succeeded;
		}
	}
	else
	{
		return EBTNodeResult::Failed;
	}

	//초기화
	if (MoveReq.IsValid() == false)
	{
		MoveReq.SetGoalLocation(AIController->GetPawn() ? AIController->GetPawn()->GetActorLocation() : FVector::ZeroVector);
	}
	else
	{
		if (!IsValid(MoveReq.GetGoalActor()))
		{
			MyMemory->PreviousGoalLocation = MoveReq.GetGoalLocation();
		}
	}

	if (MoveReq.IsValid() == true)
	{
		const FPathFollowingRequestResult RequestResult = AIController->MoveTo(MoveReq);
		if (RequestResult.Code == EPathFollowingRequestResult::RequestSuccessful)
		{
			MyMemory->MoveRequestID = RequestResult.MoveId;
			WaitForMessage(OwnerComp, UBrainComponent::AIMessage_MoveFinished, RequestResult.MoveId);
			WaitForMessage(OwnerComp, UBrainComponent::AIMessage_RepathFailed);

			NodeResult = EBTNodeResult::InProgress;
			MyMemory->FailedMoveCount = 0;
		}
		else if (RequestResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			NodeResult = EBTNodeResult::Succeeded;
			MyMemory->FailedMoveCount = 0;
		}
		else
		{
			NodeResult = EBTNodeResult::Failed;

			if (!bAttackableDestination)
			{
				MyMemory->FailedMoveCount++;
				if (MyMemory->FailedMoveCount > FailedMoveMaxCount)
				{
					MyMemory->FailedMoveCount = 0;
					if (bTargetType)
					{
						AActor* TargetActor = GetTargetActor(BlackboardComponent);
						if (IsValid(TargetActor))
						{
							AIController->AddIgnoreTarget(TargetActor, EAITargetIgnoreReason::FailedMove);
						}
					}
				}
			}
		}
	}
	return NodeResult;
}

void USDKBTTask_MoveTo::MoveToBlackboardKey(const UBlackboardComponent* BlackboardComp, FAIMoveRequest& OutMoveReq)
{
	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
	{
		UObject* KeyValue = BlackboardComp->GetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID());
		const AActor* TargetActor = Cast<AActor>(KeyValue);
		if (IsValid(TargetActor))
		{
			if (bTrackMovingGoal)
			{
				OutMoveReq.SetGoalActor(TargetActor);
			}
			else
			{
				OutMoveReq.SetGoalLocation(TargetActor->GetActorLocation());
			}
		}
		else
		{
			UE_LOG(LogBehaviorTree, Warning, TEXT("UBTTask_MoveTo::ExecuteTask tried to go to actor while BB %s entry was empty"), *BlackboardKey.SelectedKeyName.ToString());
		}
	}
	else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		const FVector TargetLocation = BlackboardComp->GetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID());
		OutMoveReq.SetGoalLocation(TargetLocation);
	}
}

void USDKBTTask_MoveTo::MoveToTarget(const UBlackboardComponent* BlackboardComp, struct FAIMoveRequest& OutMoveReq)
{
	const AActor* TargetActor = GetTargetActor(BlackboardComp);
	if (IsValid(TargetActor))
	{
		if (bTrackMovingGoal)
		{
			OutMoveReq.SetGoalActor(TargetActor);
		}
		else
		{
			OutMoveReq.SetGoalLocation(TargetActor->GetActorLocation());
		}
	}
	else
	{
		UE_LOG(LogBehaviorTree, Warning, TEXT("UBTTask_MoveTo::ExecuteTask tried to go to actor while BB %s entry was empty"), *BlackboardKey.SelectedKeyName.ToString());
	}
}

void USDKBTTask_MoveTo::MoveToAroundTarget(const UBlackboardComponent* BlackboardComp, struct FAIMoveRequest& OutMoveReq)
{
	const AActor* TargetActor = GetTargetActor(BlackboardComp);
	const FVector Location = IsValid(TargetActor) ? TargetActor->GetActorLocation() : OutMoveReq.GetGoalLocation();

	const UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (IsValid(NavSystem))
	{
		FNavLocation RandomResult;
		if (NavSystem->GetRandomPointInNavigableRadius(Location, AroundTargetRange, RandomResult))
		{
			OutMoveReq.SetGoalLocation(RandomResult.Location);
		}
	}
}

void USDKBTTask_MoveTo::MoveToAttackRangeToTarget(const UBlackboardComponent* BlackboardComp, const AActor* OwnerActor, struct FAIMoveRequest& OutMoveReq, float InRange)
{
	const AActor* TargetActor = GetTargetActor(BlackboardComp);
	if (IsValid(TargetActor))
	{
		const float AttackRange = InRange;
		const float Distance = FVector::Dist2D(TargetActor->GetActorLocation(), OwnerActor->GetActorLocation());

		FVector Direction = (TargetActor->GetActorLocation() - OwnerActor->GetActorLocation()).GetSafeNormal();
		if (Distance >= AttackRange)
		{
			Direction *= Distance - OutMoveReq.GetAcceptanceRadius();
		}
		else
		{
			Direction *= Distance - (OutMoveReq.GetAcceptanceRadius() * FMath::RandRange(0.5f, 2.0f));
		}

		OutMoveReq.SetGoalLocation(Direction + OwnerActor->GetActorLocation());
	}
}

void USDKBTTask_MoveTo::MoveToForward(const UBlackboardComponent* BlackboardComp, const AActor* OwnerActor, struct FAIMoveRequest& OutMoveReq, float InRange)
{
	OutMoveReq.SetGoalLocation(OwnerActor->GetActorLocation() + (OwnerActor->GetActorForwardVector() * InRange));
}

void USDKBTTask_MoveTo::MoveToPatrolPoint(const UBlackboardComponent* BlackboardComp, struct FAIMoveRequest& OutMoveReq)
{
	if (IsValid(BlackboardComp))
	{
		if (BlackboardComp->GetKeyID(FName("PatrolLocation")) != FBlackboard::InvalidKey)
		{
			OutMoveReq.SetGoalLocation(BlackboardComp->GetValueAsVector(FName("PatrolLocation")));
		}
	}
}

void USDKBTTask_MoveTo::MoveToRandom(const UBlackboardComponent* BlackboardComp, const AActor* OwnerActor, struct FAIMoveRequest& OutMoveReq)
{
	if (IsValid(OwnerActor))
	{
		const FVector OwnerLoccation = OwnerActor->GetActorLocation();
		const UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if (IsValid(NavSystem))
		{
			FNavLocation RandomResult;
			if (NavSystem->GetRandomPointInNavigableRadius(OwnerLoccation, RandomRange, RandomResult) == true)
			{
				OutMoveReq.SetGoalLocation(RandomResult.Location);
			}
		}
	}
}

void USDKBTTask_MoveTo::MoveToSDKObjectByTag(const AActor* OwnerActor, struct FAIMoveRequest& OutMoveReq)
{
	if (!IsValid(OwnerActor))
	{
		return;
	}

	TArray<AActor*> FindActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASDKObject::StaticClass(), FindActors);
	if (FindActors.Num() > 0)
	{
		TArray<AActor*> TargetActors;
		TArray<float> TargetDist;

		for (const auto FindActor : FindActors)
		{
			ASDKObject* SDKObject = Cast<ASDKObject>(FindActor);
			if (!IsValid(SDKObject) || !SDKObject->Tags.Contains(TagName))
			{
				continue;
			}

			TargetActors.Add(SDKObject);
			TargetDist.Add(UKismetMathLibrary::Vector_Distance(OwnerActor->GetActorLocation(), SDKObject->GetActorLocation()));
		}

		if (TargetActors.Num() > 0)
		{
			int32 Index = INDEX_NONE;
			float MinValue = 0.f;
			UKismetMathLibrary::MinOfFloatArray(TargetDist, Index, MinValue);

			if (TargetActors.IsValidIndex(Index) == true)
			{
				if (bTrackMovingGoal)
				{
					OutMoveReq.SetGoalActor(TargetActors[Index]);
				}
				else
				{
					OutMoveReq.SetGoalLocation(TargetActors[Index]->GetActorLocation());
				}
			}
		}
	}
}

void USDKBTTask_MoveTo::MoveToTime(const UBlackboardComponent* BlackboardComp, const AActor* OwnerActor, FAIMoveRequest& OutMoveReq, float InRange)
{
	MoveToAttackRangeToTarget(BlackboardComp, OwnerActor, OutMoveReq, InRange);
}

void USDKBTTask_MoveTo::MoveToStepBack(const UBlackboardComponent* BlackboardComp, const AActor* OwnerActor, FAIMoveRequest& OutMoveReq)
{
	const AActor* TargetActor = GetTargetActor(BlackboardComp);
	if (IsValid(TargetActor))
	{
		FVector Direction = (TargetActor->GetActorLocation() - OwnerActor->GetActorLocation()).GetSafeNormal();
		Direction = Direction.RotateAngleAxis(180.0f, FVector::UpVector);
		Direction *= FMath::RandRange(MinStride, MovableStride);

		OutMoveReq.SetGoalLocation(Direction + OwnerActor->GetActorLocation());
	}
}

void USDKBTTask_MoveTo::MoveStrafe(const UBlackboardComponent* BlackboardComp, const AActor* OwnerActor, FAIMoveRequest& OutMoveReq)
{
	const AActor* TargetActor = GetTargetActor(BlackboardComp);
	if (IsValid(TargetActor))
	{
		FVector Direction = OwnerActor->GetActorRightVector();
		if (FMath::RandRange(0, 1) == 0)
		{
			// 왼쪽
			Direction = Direction.RotateAngleAxis(180.0f, FVector::UpVector);
		}

		Direction *= MovableStride;
		OutMoveReq.SetGoalLocation(Direction + OwnerActor->GetActorLocation());
	}
}

void USDKBTTask_MoveTo::MoveToLocation(const UBlackboardComponent* BlackboardComp, FAIMoveRequest& OutMoveReq)
{
	if (BlackboardComp->GetKeyID(FName("Destination")) == FBlackboard::InvalidKey)
	{
		return;
	}

	FVector DestLocation = BlackboardComp->GetValueAsVector(FName("Destination"));
	OutMoveReq.SetGoalLocation(DestLocation);
}

void USDKBTTask_MoveTo::MoveToTargetOppositeLocation(const UBlackboardComponent* BlackboardComp, const AActor* OwnerActor, FAIMoveRequest& OutMoveReq)
{
	const UNavigationSystemV1* NavSystem = Cast<UNavigationSystemV1>(GetWorld()->GetNavigationSystem());
	if (!IsValid(NavSystem))
	{ 
		return;
	}
	if (!IsValid(OwnerActor))
	{
		return;
	}

	const AActor* TargetActor = GetTargetActor(BlackboardComp);
	if (IsValid(TargetActor))
	{
		FVector OppositeDir = (OwnerActor->GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal2D();
		FVector RandomLocation = OwnerActor->GetActorLocation() + OppositeDir * OppositeRange;

		FNavLocation ResultLocation;
		if (NavSystem->GetRandomPointInNavigableRadius(RandomLocation, OppositeRange, ResultLocation))
		{
			FVector ResultDir = (ResultLocation.Location - OwnerActor->GetActorLocation()).GetSafeNormal2D();
			FVector Destination = OwnerActor->GetActorLocation() + ResultDir * OppositeRange;
			OutMoveReq.SetGoalLocation(Destination);
		}
		else
		{
			OutMoveReq.SetGoalLocation(RandomLocation);
		}
	}
	else
	{
		UE_LOG(LogBehaviorTree, Warning, TEXT("UBTTask_MoveTo::ExecuteTask tried to go to actor while BB %s entry was empty"), *BlackboardKey.SelectedKeyName.ToString());
	}
}

AActor* USDKBTTask_MoveTo::GetTargetActor(const UBlackboardComponent* BlackboardComp)
{
	if (IsValid(BlackboardComp))
	{
		if (BlackboardComp->GetKeyID(FName("TargetObject")) != FBlackboard::InvalidKey)
		{
			return Cast<AActor>(BlackboardComp->GetValueAsObject(FName("TargetObject")));
		}
	}

	return nullptr;
}

bool USDKBTTask_MoveTo::IsAttackable(ASDKAIController* InAIController, uint8* NodeMemory)
{
	if (!IsValid(InAIController))
	{
		return false;
	}

	ASDKCharacter* OwnerCharacter = InAIController->GetPawn<ASDKCharacter>();
	if (!IsValid(OwnerCharacter))
	{
		return false;
	}

	AActor* CurrentTarget = InAIController->GetCurrentTarget();
	if (!IsValid(CurrentTarget))
	{
		return false;
	}

	FVector MyLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = CurrentTarget->GetActorLocation();

	float CapsuleRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	float TargetCollisionRange = 0.0f;
	ASDKCharacter* TargetCharacter = Cast<ASDKCharacter>(CurrentTarget);
	if (IsValid(TargetCharacter))
	{
		TargetCollisionRange = TargetCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	}

	float Dist = FVector::Dist2D(MyLocation, TargetLocation);
	float AttackDist = OwnerCharacter->GetSightDistance();

	float CheckDist = AttackDist + CapsuleRadius;
	if (Dist > CheckDist + TargetCollisionRange)
	{
		return false;
	}

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = { OBJECT_CUBE, ObjectTypeQuery1 };
	TArray<AActor*> IgnoreActors;

	FHitResult HitResult;
	bool bHit = false;
	bool bResult = false;

	if (bUseBoxTrace)
	{
		bHit = UKismetSystemLibrary::BoxTraceSingleForObjects(GetWorld(), MyLocation, TargetLocation, TraceBoxExtent, OwnerCharacter->GetActorRotation(), ObjectTypes, false, IgnoreActors, EDrawDebugTrace::None, HitResult, true);
	}
	else
	{
		bHit = UKismetSystemLibrary::LineTraceSingleForObjects(GetWorld(), MyLocation, TargetLocation, ObjectTypes, false, IgnoreActors, EDrawDebugTrace::None, HitResult, true);
	}

	if (!bHit)
	{
		bResult = true;
	}

	return bResult;
}
