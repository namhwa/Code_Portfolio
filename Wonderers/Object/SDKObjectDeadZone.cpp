// Fill out your copyright notice in the Description page of Project Settings.

#include "Object/SDKObjectDeadZone.h"
#include "Components/BrushComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NavigationSystem.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Engine/SDKAssetManager.h"
#include "Manager/SDKTableManager.h"
#include "Character/SDKSkillDirector.h"
#include "Character/SDKProjectile.h"
#include "Character/SDKInGameController.h"
#include "Character/SDKNonePlayerCharacter.h"
#include "Character/SDKPlayerCharacter.h"
#include "Character/SDKCharacter.h"

ASDKObjectDeadZone::ASDKObjectDeadZone()
{
	bPainCausing = false;
	bEntryPain = false;
	bIsActive = false;

	ProjectileID = TEXT("116");

	MaxMeteorCount = 20;
	DamagePerSec = 100.f;
	PainInterval = 1.f;
	DefaultDamage = DamagePerSec;

	GetBrushComponent()->SetRelativeScale3D(FVector(1.f, 1.f, 20.f));
	GetBrushComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetBrushComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(COLLISION_CHARACTER, ECR_Overlap);
	GetBrushComponent()->SetSimulatePhysics(false);
	GetBrushComponent()->SetEnableGravity(false);

	// Create Skill Director
	SkillDirector = CreateDefaultSubobject<USDKSkillDirector>(TEXT("SkillDirector"));
}

void ASDKObjectDeadZone::ActorEnteredVolume(class AActor* Other)
{
	return;
}

void ASDKObjectDeadZone::PainTimer()
{
	if(bPainCausing == false || bIsActive == false)
	{
		return;
	}

	TSet<AActor*> TouchingActors;
	GetOverlappingActors(TouchingActors, APawn::StaticClass());

	for(AActor* const A : TouchingActors)
	{
		if(A && A->CanBeDamaged() && !A->IsPendingKill())
		{
			APawn* PawnA = Cast<APawn>(A);
			if(PawnA && PawnA->GetPawnPhysicsVolume() == this)
			{
				CausePainTo(A);
			}
		}
	}
}

void ASDKObjectDeadZone::CausePainTo(class AActor* Other)
{
	if(bIsActive == false || bPainCausing == false)
	{
		return;
	}
	
	if(DamagePerSec > 0.f)
	{
		Super::CausePainTo(Other);
	}
}

TArray<AActor*> ASDKObjectDeadZone::GetOverlapActiveActors()
{
	TSet<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, ASDKPlayerCharacter::StaticClass());

	TArray<AActor*> OverlapActors;
	OverlapActors.Reserve(OverlappingActors.Num());
	
	for(auto& iter : OverlappingActors)
	{
		OverlapActors.Add(iter);
	}

	return OverlapActors;
}

void ASDKObjectDeadZone::InitDeadZone()
{
	bIsActive = false;
	bPainCausing = false;

	MaxHeight = 2000.f;
}

void ASDKObjectDeadZone::SetDeadZoneIndex(int32 Index)
{
	if(HasAuthority() == false || Index < 0)
	{
		return;
	}

	DeadZoneIndex = Index;
}

void ASDKObjectDeadZone::ActiveDaedZone()
{
	if(HasAuthority() == false)
	{
		return;
	}

	bPainCausing = true;
	PainInterval = 1.f;
	DamagePerSec = DefaultDamage;

	bIsActive = true;

	GetWorldTimerManager().ClearTimer(TimerHandle_PainTimer);
	GetWorldTimerManager().SetTimer(TimerHandle_PainTimer, this, &ASDKObjectDeadZone::PainTimer, PainInterval, true, 0.f);
}

void ASDKObjectDeadZone::ActiveMeteor()
{
	if(HasAuthority() == false)
	{
		return;
	}

	DamagePerSec = 0.f;
	PainInterval = 5.f;

	bIsActive = true;

	GetWorldTimerManager().ClearTimer(TimerHandle_PainTimer);
	GetWorldTimerManager().SetTimer(TimerHandle_PainTimer, this, &ASDKObjectDeadZone::CausePainMeteor, PainInterval, true, 0.f);
}

void ASDKObjectDeadZone::DeActiveDeadZone()
{
	if(HasAuthority() == false)
	{
		return;
	}

	bPainCausing = false;
	bIsActive = false;

	GetWorldTimerManager().ClearTimer(TimerHandle_PainTimer);
}

void ASDKObjectDeadZone::DeActiveMeteorZone()
{
	if(HasAuthority() == false)
	{
		return;
	}

	bPainCausing = false;
	bIsActive = false;

	GetWorldTimerManager().ClearTimer(TimerHandle_PainTimer);
	GetWorldTimerManager().ClearTimer(TimerHadle_LocationInterval);
	GetWorldTimerManager().ClearTimer(TimerHandle_DropInterval);
}

void ASDKObjectDeadZone::SetMeteorIntervalTime(float NewTime)
{
	if(HasAuthority() == false)
	{
		return;
	}

	if(NewTime <= 0.f || PainInterval == NewTime)
	{
		return;
	}

	PainInterval = NewTime;
	MeteorInterval = (PainInterval / (float)MaxMeteorCount);
}

void ASDKObjectDeadZone::CausePainMeteor()
{
	LocationIndex = FMath::RandRange(0, MaxMeteorCount - 1);

	GetWorldTimerManager().SetTimer(TimerHadle_LocationInterval, this, &ASDKObjectDeadZone::SetMeteorLocations, MeteorInterval, true, 0.f);
}

void ASDKObjectDeadZone::SetMeteorLocations()
{
	if(HasAuthority() == false)
	{
		return;
	}

	if(SkillDirector == nullptr)
	{
		return;
	}

	if(MeteorCount >= MaxMeteorCount)
	{
		MeteorCount = 0;
		GetWorldTimerManager().ClearTimer(TimerHadle_LocationInterval);
	}

	// 캐릭터 위치 소환 가능 여부
	bool bCharacterPoint = false;

	// 메테오 위치 랜덤 지정
	vMeteorLocations = FVector::ZeroVector;
	if(MeteorCount == LocationIndex)
	{
		if(GetWorld()->GetFirstPlayerController() == nullptr)
		{
			return;
		}

		ASDKPlayerCharacter* SDKCharacter = Cast<ASDKPlayerCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
		if(SDKCharacter != nullptr)
		{
			vMeteorLocations = SDKCharacter->GetActorLocation();
		}

		// 구역 범위 안에 있는지 확인
		bCharacterPoint = UKismetMathLibrary::IsPointInBox(vMeteorLocations, GetActorLocation(), FVector(1400.f, 1400.f, 4000.f));
	}
	
	if(bCharacterPoint == false)
	{
		UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if(NavSystem != nullptr)
		{
			FNavLocation Result;
			bool bSuccess = NavSystem->GetRandomPointInNavigableRadius(GetActorLocation(), 1400.f, Result);
			if(bSuccess == true)
			{
				vMeteorLocations = Result.Location;
			}
		}
	}
	
	// 터레인 높이 값으로 위치 보정
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = { OBJECT_CUBE };
	TArray<AActor*> IgnoreActors = { this };
	
	FVector vStart = vMeteorLocations + FVector(0.f, 0.f, 1000.f);
	FVector vEnd = vStart + FVector(0.f, 0.f, KILL_Z);
	
	// 터레인 충돌 검사
	FHitResult HitResult;
	UKismetSystemLibrary::LineTraceSingleForObjects(GetWorld(), vStart, vEnd, ObjectTypes, false, IgnoreActors, EDrawDebugTrace::None, HitResult, true);
	
	// 최종 위치 설정
	if(HitResult.bBlockingHit == true)
	{
		vMeteorLocations.Z = HitResult.Location.Z + 5.f;
	}
	else
	{
		vMeteorLocations.Z += 5.f;
	}
	
	for(FConstPlayerControllerIterator Controller = GetWorld()->GetPlayerControllerIterator(); Controller; Controller++)
	{
		auto SDKPlayerController = Cast<ASDKInGameController>(Controller->Get());
		if(SDKPlayerController != nullptr)
		{
			//SDKPlayerController->MultiSetMeteorLocationEffect(vMeteorLocations);
		}
	}

	float DropInterval = MeteorInterval * 0.7f;
	if(DropInterval <= 0.f)
	{
		DropInterval = 0.1f;
	}

	GetWorldTimerManager().SetTimer(TimerHandle_DropInterval, this, &ASDKObjectDeadZone::SpawnMeteor, DropInterval, false);
}

void ASDKObjectDeadZone::SpawnMeteor()
{
	if(HasAuthority() == false)
	{
		return;
	}

	if (MeteorCount == MaxMeteorCount)
	{
		return;
	}

	auto ProjectileTable = USDKTableManager::Get()->FindTableRow<FS_Projectile>(ETableType::tb_Projectile, ProjectileID);
	if(ProjectileTable == nullptr)
	{
		return;
	}

	if (ProjectileTable->ProjectileClass.IsNull())
	{
		return;
	}

	TSubclassOf<ASDKProjectile> ProjectileClass(*USDKAssetManager::Get().LoadSynchronous(ProjectileTable->ProjectileClass));
	check(ProjectileClass);

	FVector vLocation = vMeteorLocations;
	vLocation.Z += 2000.f;

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.Owner = this;

	ASDKProjectile* SDKProjectile = GetWorld()->SpawnActor<ASDKProjectile>(ProjectileClass, vLocation, FRotator::ZeroRotator, SpawnInfo);
	if(SDKProjectile != nullptr)
	{
		float fDistance = vLocation.Z - (vMeteorLocations.Z - 5.f);

		// 생존 시간 계산
		float fLifeTime = 0.f;
		if(SDKProjectile->GetMovementComponent()->MaxSpeed > 0)
		{
			fLifeTime = fDistance / SDKProjectile->GetMovementComponent()->MaxSpeed;
		}

		// 프로젝타일 데이터 설정
		FProjectileData ProjectileData(0, 0, 1000, 1, false, FVector(0.f, 0.f, -1.f), fDistance, fLifeTime, 200.f, 0.f, 0.f, 0.f, 0.f, nullptr, 0.f);
		SDKProjectile->InitProjectile(ProjectileData);

		MeteorCount++;
	}
}
