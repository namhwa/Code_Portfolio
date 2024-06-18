// Fill out your copyright notice in the Description page of Project Settings.

#include "Housing/SDKSpringArmComponent.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"

USDKSpringArmComponent::USDKSpringArmComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bDoCollisionTest = false;
	bEnableCameraLag = true;

	CameraLagSpeed = 2.f;
	CameraRotationLagSpeed = 2.f;
	
	TargetArmLength = 2500.f;

	CameraLagMinDistance = 0.f;
	CameraLagMaxDistance = 1000.f;

	ProbeChannel = ECC_Camera;
}

void USDKSpringArmComponent::BeginPlay()
{
	Super::BeginPlay();

	if(CameraType != EFurnitureType::None)
	{
		SetSpringArmRotationByType();
	}
}

void USDKSpringArmComponent::UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime)
{
	if(CameraLagMinDistance > 0.f)
	{
		CheckCameraLagMinDistance(bDoTrace, bDoLocationLag, bDoRotationLag, DeltaTime);
	}
	else
	{
		Super::UpdateDesiredArmLocation(bDoTrace, bDoLocationLag, bDoRotationLag, DeltaTime);
	}
}

void USDKSpringArmComponent::CheckCameraLagMinDistance(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime)
{
	FRotator DesiredRotation = GetTargetRotation();
	FVector ArmOrigin = GetComponentLocation() + TargetOffset;
	FVector DesiredLocation = ArmOrigin;

	if(bDoRotationLag)
	{
		if(bUseCameraLagSubstepping && DeltaTime > CameraLagMaxTimeStep && CameraRotationLagSpeed > 0.f)
		{
			const FRotator ArmRotStep = (DesiredRotation - PreviousDesiredRot).GetNormalized() * (1.f / DeltaTime);
			FRotator LerpTarget = PreviousDesiredRot;
			
			float RemainingTime = DeltaTime;
			while(RemainingTime > KINDA_SMALL_NUMBER)
			{
				const float LerpAmount = FMath::Min(CameraLagMaxTimeStep, RemainingTime);
				LerpTarget += ArmRotStep * LerpAmount;
				RemainingTime -= LerpAmount;

				DesiredRotation = FRotator(FMath::QInterpTo(FQuat(PreviousDesiredRot), FQuat(LerpTarget), LerpAmount, CameraRotationLagSpeed));
				PreviousDesiredRot = DesiredRotation;
			}
		}
		else
		{
			DesiredRotation = FRotator(FMath::QInterpTo(FQuat(PreviousDesiredRot), FQuat(DesiredRotation), DeltaTime, CameraRotationLagSpeed));
		}
	}

	PreviousDesiredRot = DesiredRotation;
	float ChangedArmLength = TargetArmLength;

	if(bDoLocationLag)
	{
		if(bUseCameraLagSubstepping && DeltaTime > CameraLagMaxTimeStep && CameraLagSpeed > 0.f)
		{
			const FVector ArmMovementStep = (DesiredLocation - PreviousDesiredLoc) * (1.f / DeltaTime);
			FVector LerpTarget = PreviousDesiredLoc;

			float RemainingTime = DeltaTime;
			while(RemainingTime > KINDA_SMALL_NUMBER)
			{
				const float LerpAmount = FMath::Min(CameraLagMaxTimeStep, RemainingTime);
				LerpTarget += ArmMovementStep * LerpAmount;
				RemainingTime -= LerpAmount;

				DesiredLocation = FMath::VInterpTo(PreviousDesiredLoc, LerpTarget, LerpAmount, CameraLagSpeed);
				PreviousDesiredLoc = DesiredLocation;
			}
		}
		else
		{
			DesiredLocation = FMath::VInterpTo(PreviousDesiredLoc, DesiredLocation, DeltaTime, CameraLagSpeed);
		}

		bool bClampedDist = false;
		const FVector FromOrigin = DesiredLocation - ArmOrigin;
		ChangedArmLength = FromOrigin.SizeSquared() <= FMath::Square(CameraLagMaxDistance) ? CameraLagMinDistance : TargetArmLength;

		if(FromOrigin.SizeSquared() < FMath::Square(CameraLagMaxDistance) || FromOrigin.SizeSquared() > FMath::Square(CameraLagMaxDistance))
		{
			DesiredLocation = ArmOrigin + FromOrigin.GetClampedToMaxSize(CameraLagMaxDistance);
			bClampedDist = true;
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if(bDrawDebugLagMarkers)
		{
			DrawDebugSphere(GetWorld(), ArmOrigin, 5.f, 8, FColor::Green);
			DrawDebugSphere(GetWorld(), DesiredLocation, 5.f, 8, FColor::Yellow);

			const FVector ToOrigin = ArmOrigin - DesiredLocation;
			DrawDebugDirectionalArrow(GetWorld(), DesiredLocation, DesiredLocation + ToOrigin * 0.5f, 7.5f, bClampedDist ? FColor::Red : FColor::Green);
			DrawDebugDirectionalArrow(GetWorld(), DesiredLocation + ToOrigin * 0.5f, ArmOrigin, 7.5f, bClampedDist ? FColor::Red : FColor::Green);
		}
#endif
	}

	PreviousArmOrigin = ArmOrigin;
	PreviousDesiredLoc = DesiredLocation;

	DesiredLocation -= DesiredRotation.Vector() * ChangedArmLength;
	DesiredLocation += FRotationMatrix(DesiredRotation).TransformVector(SocketOffset);

	FVector ResultLocation;
	if(bDoTrace && (TargetArmLength != 0.0f))
	{
		bIsCameraFixed = true;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpringArm), false, GetOwner());

		FHitResult Result;
		GetWorld()->SweepSingleByChannel(Result, ArmOrigin, DesiredLocation, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeSize), QueryParams);

		UnfixedCameraPosition = DesiredLocation;

		ResultLocation = BlendLocations(DesiredLocation, Result.Location, Result.bBlockingHit, DeltaTime);

		if (ResultLocation == DesiredLocation)
		{
			bIsCameraFixed = false;
		}
	}
	else
	{
		ResultLocation = DesiredLocation;
		bIsCameraFixed = false;
		UnfixedCameraPosition = ResultLocation;
	}

	FTransform WorldCameraTransform(DesiredRotation, DesiredLocation);
	FTransform RelativeCameraTransform = WorldCameraTransform.GetRelativeTransform(GetComponentTransform());

	RelativeSocketLocation = RelativeCameraTransform.GetLocation();
	RelativeSocketRotation = RelativeCameraTransform.GetRotation();

	UpdateChildTransforms();
}

void USDKSpringArmComponent::SetCameraType(EFurnitureType eType)
{
	CameraType = eType;

	SetSpringArmLagSpeedByType();
	SetSpringArmRotationByType();
}

void USDKSpringArmComponent::SetSpringArmLagSpeedByType()
{
	if(CameraType == EFurnitureType::Wall || CameraType == EFurnitureType::WallHangings)
	{
		CameraLagSpeed = WallTypeLagSpeed;
	}
	else
	{
		CameraLagSpeed = GenerateTypeLagSpeed;
	}
}

void USDKSpringArmComponent::SetSpringArmRotationByType()
{
	if(CameraType == EFurnitureType::None)
	{	
		return;
	}

	if(!IsValid(GetOwner()))
	{
		return;
	}

	FRotator tTypeRotator = GetOwner()->GetActorRotation();
	tTypeRotator.Yaw -= 45.f;

	switch(CameraType)
	{
	case EFurnitureType::Wall:
		tTypeRotator = FRotator(-30.f, 0.f, 0.f);
		break;
	case EFurnitureType::Floor:
		tTypeRotator = FRotator(-75.f, 0.f, 0.f);
		break;
	case EFurnitureType::WallHangings:
		tTypeRotator = FRotator(-30.f, -60.f, 0.f);
		break;
	default:
		tTypeRotator += FRotator(-75.f, -45.f, 0.f);
		if((tTypeRotator.Yaw <= -130.f && tTypeRotator.Yaw > -180.f) || (tTypeRotator.Yaw < 60.f && tTypeRotator.Yaw > 0.f))
		{
			tTypeRotator.Yaw -= 180.f;
		}
		break;
	}

	SetRelativeRotation(tTypeRotator);
}

void USDKSpringArmComponent::SetWallTypesForwardRotator(bool bForward)
{
	if(CameraType != EFurnitureType::WallHangings)
	{
		return;
	}

	FRotator tRotator = bForward ? FRotator(-30.f, -60.f, 0.f) : FRotator(-30.f, -110, 0.f);
	SetRelativeRotation(tRotator);
}
