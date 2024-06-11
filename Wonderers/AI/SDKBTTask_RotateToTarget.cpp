// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/SDKBTTask_RotateToTarget.h"

#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/KismetMathLibrary.h"

#include "AI/SDKAIController.h"
#include "Character/SDKNonePlayerCharacter.h"

#include "Stats/Stats.h"
DECLARE_CYCLE_STAT(TEXT("USDKBTTask_RotateToTarget::TickTask"), STAT_USDKBTTask_RotateToTarget_TickTask, STATGROUP_AI);


USDKBTTask_RotateToTarget::USDKBTTask_RotateToTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("SDK Rotate To Target");
	bNotifyTick = true;
	bNotifyTaskFinished = true;
	BlackboardKey = FBlackboardKeySelector();

	ErrorTolerance = 15.f;

	bUseInterp = true;
	RotateSpeed = 5.f;
	bRotateNearyAxis = false;
	bRandomRotator = false;
}

EBTNodeResult::Type USDKBTTask_RotateToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);
	EBTNodeResult::Type Result = EBTNodeResult::Failed;
	FSDKBTRotateToTaskMemory* MyMemory = CastInstanceNodeMemory<FSDKBTRotateToTaskMemory>(NodeMemory);

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	ASDKCharacter* SDKCharacter = OwnerComp.GetAIOwner()->GetPawn<ASDKCharacter>();

	if (BlackboardComponent == nullptr || SDKCharacter == nullptr)
	{
		return EBTNodeResult::Succeeded;
	}

	if (SDKCharacter->GetCurrentMontage() != nullptr)
	{
		return EBTNodeResult::Succeeded;
	}

	AActor* TargetActor = nullptr;
	if (BlackboardComponent->GetKeyID(BlackboardKey.SelectedKeyName) != FBlackboard::InvalidKey)
	{
		TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(BlackboardKey.SelectedKeyName));
	}

	FRotator CurRotator = SDKCharacter->GetActorRotation();

	FRotator DirRotator = FRotator::ZeroRotator;

	if (bRandomRotator)
	{
		DirRotator.Yaw = FMath::RandRange(0.0f, 360.0f);
	}
	else
	{
		if (IsValid(TargetActor))
		{
			DirRotator = (TargetActor->GetActorLocation() - SDKCharacter->GetActorLocation()).GetSafeNormal2D().Rotation();
		}
		else
		{
			DirRotator = SDKCharacter->GetActorRotation();
		}
		DirRotator = FRotator(0.f, DirRotator.Yaw, 0.f);
	}

	if (bRotateNearyAxis)
	{
		DirRotator = GetNearyAxisRotator(DirRotator);
	}

	if (DirRotator.Equals(FRotator(0.f, CurRotator.Yaw, 0.f), ErrorTolerance) == false)
	{
		if (bUseInterp == true)
		{
			MyMemory->bIsLeft = CheckRotateLeft(CurRotator, DirRotator);
			MyMemory->MaxRotator = DirRotator;

			Result = EBTNodeResult::InProgress;
		}
		else
		{
			SDKCharacter->SetActorRotation(DirRotator);
			Result = EBTNodeResult::Succeeded;
		}
	}
	else
	{
		Result = EBTNodeResult::Succeeded;
	}
	return Result;
}

EBTNodeResult::Type USDKBTTask_RotateToTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::AbortTask(OwnerComp, NodeMemory);
	EBTNodeResult::Type ReturnType = EBTNodeResult::Aborted;
	FSDKBTRotateToTaskMemory* MyMemory = CastInstanceNodeMemory<FSDKBTRotateToTaskMemory>(NodeMemory);

	AActor* OwnerActor = OwnerComp.GetAIOwner()->GetPawn();
	if (OwnerActor != nullptr)
	{
		if (OwnerActor->GetActorRotation().Equals(MyMemory->MaxRotator, RotateSpeed) == false)
		{
			MyMemory->bAbortTask = true;
			ReturnType = EBTNodeResult::InProgress;
		}
	}

	return ReturnType;
}

void USDKBTTask_RotateToTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_USDKBTTask_RotateToTarget_TickTask);
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("USDKBTTask_RotateToTarget::TickTask"));
	FSDKBTRotateToTaskMemory* MyMemory = CastInstanceNodeMemory<FSDKBTRotateToTaskMemory>(NodeMemory);
	if (!MyMemory)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	ASDKCharacter* SDKCharacter = OwnerComp.GetAIOwner()->GetPawn<ASDKCharacter>();

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = nullptr;
	if (BlackboardComponent->GetKeyID(BlackboardKey.SelectedKeyName) != FBlackboard::InvalidKey)
	{
		TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(BlackboardKey.SelectedKeyName));
	}

	if (MyMemory->MaxRotator == FAISystem::InvalidRotation)
	{
		if (MyMemory->bAbortTask == true)
		{
			FinishLatentAbort(OwnerComp);
		}
		else
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		return;
	}

	FRotator NewRotator = SDKCharacter->GetActorRotation();
	NewRotator.Yaw = NewRotator.Yaw + ( MyMemory->bIsLeft ? -RotateSpeed : RotateSpeed);

	if (NewRotator.Equals(MyMemory->MaxRotator, RotateSpeed) == true)
	{
		NewRotator.Yaw = MyMemory->MaxRotator.Yaw;
		SDKCharacter->SetActorRotation(NewRotator);

		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		if (MyMemory->bAbortTask)
		{
			FinishLatentAbort(OwnerComp);
		}

		return;
	}
	else
	{
		if (SDKCharacter->GetCurrentMontage() == nullptr)
		{
			ASDKNonePlayerCharacter* SDKNpc = Cast<ASDKNonePlayerCharacter>(SDKCharacter);
			if (IsValid(SDKNpc))
			{
				SDKNpc->PlayAnimMontageRotator(MyMemory->bIsLeft);
			}
		}

		SDKCharacter->SetActorRotation(NewRotator);
	}
}	

void USDKBTTask_RotateToTarget::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

	FSDKBTRotateToTaskMemory* MyMemory = CastInstanceNodeMemory<FSDKBTRotateToTaskMemory>(NodeMemory);
	if (MyMemory)
	{
		MyMemory->bIsLeft = false;
		MyMemory->MaxRotator = FAISystem::InvalidRotation;
		MyMemory->bAbortTask = false;
	}

	ASDKNonePlayerCharacter* SDKNpc = OwnerComp.GetAIOwner()->GetPawn<ASDKNonePlayerCharacter>();
	if (SDKNpc != nullptr && SDKNpc->IsRotateAnim() == true)
	{
		SDKNpc->MultiStopAnimMontage();
	}
}

uint16 USDKBTTask_RotateToTarget::GetInstanceMemorySize() const
{
	return sizeof(FSDKBTRotateToTaskMemory);
}

bool USDKBTTask_RotateToTarget::CheckRotateLeft(FRotator Current, FRotator Goal)
{
	FRotator Direction = Goal - Current;
	if (Direction.Yaw < 0)
	{
		Direction.Yaw += 360.f;
	}
	else if (Direction.Yaw > 360.f)
	{
		Direction.Yaw -= 360.f;
	}

	return Direction.Yaw > 180.f;
}

FRotator USDKBTTask_RotateToTarget::GetNearyAxisRotator(const FRotator& InRotator)
{
	FRotator ResultRotator = InRotator;
	
	ResultRotator.Yaw = FMath::Fmod(ResultRotator.Yaw, 360.0f);
	if (ResultRotator.Yaw < 0.0f)
	{
		ResultRotator.Yaw += 360.0f;
	}

	if (ResultRotator.Yaw >= 315.0f || ResultRotator.Yaw < 45.0f)
	{
		ResultRotator.Yaw = 0.0f;
	}
	else if (ResultRotator.Yaw >= 45.0f && ResultRotator.Yaw < 135.0f)
	{
		ResultRotator.Yaw = 90.0f;
	}
	else if (ResultRotator.Yaw >= 135.0f && ResultRotator.Yaw < 225.0f)
	{
		ResultRotator.Yaw = 180.0f;
	}
	else if (ResultRotator.Yaw >= 225.0f && ResultRotator.Yaw < 315.0f)
	{
		ResultRotator.Yaw = 270.0f;
	}
	return ResultRotator;
}
