// Fill out your copyright notice in the Description page of Project Settings.

#include "Housing/SDKFurniture.h"

#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "UObject/ConstructorHelpers.h"

#include "Character/SDKHUD.h"
#include "Manager/SDKTableManager.h"
#include "Share/SDKEnum.h"

#include "UI/SDKLobbyWidget.h"


ASDKFurniture::ASDKFurniture()
	: RoomIndex(INDEX_NONE), IsPreviewActor(false)
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetMobility(EComponentMobility::Movable);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetRelativeRotation(FRotator(0.f, 45.f, 0.f));
	CollisionComponent->SetGenerateOverlapEvents(false);
	CollisionComponent->SetBoxExtent(CollsionSize);
	RootComponent = CollisionComponent;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponenet"));
	MeshComponent->SetRelativeLocation(FVector(0.f, 0.f, -TILE_HALF));
	MeshComponent->SetMobility(EComponentMobility::Movable);
	MeshComponent->SetCastShadow(false);
	MeshComponent->SetupAttachment(RootComponent);

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionObjectType(COLLISION_OBJECT);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(COLLISION_CHARACTER, ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(COLLISION_OBJECT, ECR_Block);

	// 오브젝트 그림자 추가
	ShadowMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShadowMeshComponenet"));
	ShadowMeshComponent->SetRelativeLocation(FVector(0.f, 0.f, 1.f));
	ShadowMeshComponent->SetMobility(EComponentMobility::Movable);
	ShadowMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShadowMeshComponent->SetCastShadow(false);
	ShadowMeshComponent->SetupAttachment(MeshComponent);
	ShadowMeshComponent->SetVisibility(false);
	
	ConstructorHelpers::FObjectFinder<UStaticMesh> ShadowMesh(TEXT("/Game/Environments/Public/Bbox/Plane.Plane"));
	if (ShadowMesh.Succeeded())
	{
		ShadowMeshComponent->SetStaticMesh(ShadowMesh.Object);
	}

	ConstructorHelpers::FObjectFinder<UMaterialInterface> ShadowMaterial(TEXT("/Game/Characters/Public/Materials/M_Shadow.M_Shadow"));
	if (ShadowMaterial.Succeeded())
	{
		ShadowMeshComponent->SetMaterial(0, ShadowMaterial.Object);
	}

	CollsionSize = FVector(TILE_HALF);
}

#if WITH_EDITOR
void ASDKFurniture::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void ASDKFurniture::BeginPlay()
{
	Super::BeginPlay();
}

void ASDKFurniture::NotifyActorOnReleased(FKey ButtonReleased /*= EKeys::LeftMouseButton*/)
{
	Super::NotifyActorOnReleased(EKeys::LeftMouseButton);

	if(FurnitureType <= EFurnitureType::Floor || IsPreviewActor)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (IsValid(PlayerController))
	{
		ASDKHUD* MyHUD = Cast<ASDKHUD>(PlayerController->GetHUD());
		if (IsValid(MyHUD)))
		{
			USDKUserWidget* ModeMainWidget = (MyHUD->GetUI(EUI::Lobby_Main));
			if(IsValid(ModeMainWidget))
			{
				USDKLobbyWidget* LobbyWidget = Cast<USDKLobbyWidget>(ModeMainWidget);
				if(IsValid(LobbyWidget))
				{
					LobbyWidget->NotifyClickEventFurniture(this);
				}
			}
		}
	}
}

void ASDKFurniture::NotifyActorOnInputTouchEnd(const ETouchIndex::Type FingerIndex)
{
	Super::NotifyActorOnInputTouchEnd(FingerIndex);

	if(FurnitureType <= EFurnitureType::Floor || IsPreviewActor)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (IsValid(PlayerController))
	{
		ASDKHUD* MyHUD = Cast<ASDKHUD>(PlayerController->GetHUD());
		if (IsValid(MyHUD)))
		{
			USDKUserWidget* ModeMainWidget = (MyHUD->GetUI(EUI::Lobby_Main));
			if(IsValid(ModeMainWidget))
			{
				USDKLobbyWidget* LobbyWidget = Cast<USDKLobbyWidget>(ModeMainWidget);
				if(IsValid(LobbyWidget))
				{
					LobbyWidget->NotifyClickEventFurniture(this);
				}
			}
		}
	}
}

void ASDKFurniture::InitFurnitureData()
{
	if(FurnitureID.IsEmpty())
	{
		return;
	}

	FS_MyroomParts* FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(FurnitureID);
	FS_Item* ItemTable = USDKTableManager::Get()->FindTableItem(FurnitureID);
	if(FurnitureTable == nullptr || ItemTable == nullptr)
	{
		UE_LOG(LogGame, Error, TEXT("Not Found MyroomParts Table : %s"), *FurnitureID);
		return;
	}

	SetFurnitureType(FurnitureTable->Type);
	SetFurnitureSize(FurnitureTable->Size);
	SetStaticMeshLocationByType();
	
	// BP가 아닌 경우에만 처리
	if (!FurnitureTable->PartsClass.GetUniqueID().IsAsset())
	{
		if(ItemTable->MeshPath.GetUniqueID().IsAsset())
		{
			UStaticMesh* FurnitureMesh = LoadObject<UStaticMesh>(nullptr, *ItemTable->MeshPath.ToString());
			if (IsValid(FurnitureMesh))
			{
				SetStaticMesh(FurnitureMesh);
			}
			else
			{
				FString Str = FString::Printf(TEXT("Invalid ItemTable MeshPath: (%s)"), *ItemTable->MeshPath.ToString());
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Str, true, FVector2D(3.f, 3.f));
			}
		}
	}
	else
	{
		if(IsPreviewActor)
		{
			if(ItemTable->MeshPath.GetUniqueID().IsAsset())
			{
				UStaticMesh* FurnitureMesh = LoadObject<UStaticMesh>(nullptr, *ItemTable->MeshPath.ToString());
				if (IsValid(FurnitureMesh))
				{
					SetStaticMesh(FurnitureMesh);
				}
				else
				{
					FString Str = FString::Printf(TEXT("Invalid ItemTable MeshPath: (%s)"), *ItemTable->MeshPath.ToString());
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Str, true, FVector2D(3.f, 3.f));
				}
			}

			FVector vScale = FurnitureType == EFurnitureType::WallHangings ? FVector(2.5f) : FVector::OneVector;
			SetStaticMeshScale(vScale);

			if(IsValid(MeshComponent))
			{
				MeshComponent->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
			}
		}
	}

	// 그림자 처리
	SetFurnitureShadow();
}

void ASDKFurniture::SetUniqueID(FString newUniqueID)
{
	if(newUniqueID.IsEmpty())
	{
		return;
	}

	UniqueID = newUniqueID;
}

void ASDKFurniture::SetFurnitureID(FString NewTableID)
{
	if(NewTableID == FurnitureID || NewTableID.IsEmpty())
	{
		return;
	}

	FurnitureID = NewTableID;

	InitFurnitureData();
}

void ASDKFurniture::SetFurnitureType(EFurnitureType NewType)
{
	if(NewType == FurnitureType || NewType == EFurnitureType::None)
	{
		return;
	}

	FurnitureType = NewType;
}

void ASDKFurniture::SetArrangedRoomIndex(int32 newIndex)
{
	RoomIndex = newIndex;
}

void ASDKFurniture::SetFurnitureSize(FVector vNewSize)
{
	Size = vNewSize;
}

void ASDKFurniture::SetSavedIndex(FVector vNewIndex)
{
	SavedIndex = vNewIndex;

	SetStaticMeshLocationByType();
}

FVector ASDKFurniture::GetStaticMeshLocation() const
{
	if(IsValid(MeshComponent))
	{
		return MeshComponent->GetRelativeTransform().GetLocation();
	}

	return FVector::ZeroVector;
}

void ASDKFurniture::SetStaticMeshLocationByType()
{
	if(IsValid(MeshComponent))
	{
		FVector vLocation = FVector(0.f, 0.f, -TILE_HALF);
		if(FurnitureType == EFurnitureType::WallHangings)
		{
			auto FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(FurnitureID);
			if(FurnitureTable != nullptr)
			{
				vLocation.Y = TILE_HALF * FurnitureTable->Size.Y * 1.1f;
				vLocation.X = -vLocation.Y / 10.f;

				vLocation.Z = -(FurnitureTable->Size.Z - 1) * TILE_HALF * 2.f;
			}
		}
		else
		{
			vLocation.Z -= (Size.Z - 1) * TILE_HALF;
		}

		MeshComponent->SetRelativeLocation(vLocation);
	}
}

void ASDKFurniture::SetStaticMeshLocation(FVector vLocation)
{
	if(IsValid(MeshComponent))
	{
		vLocation *= TILE_HALF;
		vLocation.Z = GetStaticMeshLocation().Z;

		MeshComponent->SetRelativeLocation(vLocation);
	}
}

FVector ASDKFurniture::GetStaticMeshScale() const
{
	if(IsValid(MeshComponent))
	{
		return MeshComponent->GetRelativeTransform().GetScale3D();
	}
	
	return FVector::ZeroVector;
}

void ASDKFurniture::SetStaticMeshScale(FVector vScale)
{
	if(IsValid(MeshComponent))
	{
		MeshComponent->SetRelativeScale3D(vScale);
	}
}

void ASDKFurniture::SetStaticMesh(UStaticMesh * NewMesh)
{
	if(IsValid(MeshComponent))
	{
		MeshComponent->SetStaticMesh(NewMesh);
	}
}

void ASDKFurniture::SetStaticMeshMaterial(UMaterialInterface* NewMaterial)
{
	if(IsValid(MeshComponent) && IsValid(NewMaterial))
	{
		MeshComponent->SetMaterial(0, NewMaterial);
	}
}

void ASDKFurniture::SetCollisionSize(FVector vNewSize)
{
	if(IsValid(CollisionComponent))
	{
		CollisionComponent->SetBoxExtent(CollsionSize * vNewSize);
	}
}

void ASDKFurniture::SetCollisionEnabled(bool bEnable)
{
	if(IsValid(MeshComponent))
	{
		if(bEnable)
		{
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		else
		{
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ASDKFurniture::SetFurnitureShadow()
{
	if (IsValid(ShadowMeshComponent))
	{
		if (FurnitureType == EFurnitureType::Furniture || FurnitureType == EFurnitureType::Decoration)
		{
			ShadowMeshComponent->SetVisibility(true);
			ShadowMeshComponent->SetRelativeScale3D(FVector(Size.X, Size.Y, 1.f) * 3.f);
		}
	}
}

void ASDKFurniture::SetFurnitureArrangeEffect()
{
	UParticleSystem* SpawnParticleObj = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/Effects/Skill/Public/My_Room_Dust.My_Room_Dust"), nullptr, LOAD_None, nullptr);
	if(IsValid(SpawnParticleObj))
	{
		FVector vEffectLocation = GetActorTransform().GetLocation();
		if(FurnitureType == EFurnitureType::WallHangings)
		{
			vEffectLocation.Y += GetStaticMeshLocation().Y;
		}
		else
		{
			vEffectLocation.Z -= Size.Z * TILE_HALF;
		}
		
		// 이펙트 추가 및 자동 소멸
		SpawnEffectComponent = UGameplayStatics::SpawnEmitterAttached(SpawnParticleObj, RootComponent, NAME_None, RootComponent->GetRelativeTransform().GetLocation(), RootComponent->GetRelativeTransform().Rotator(), Size + 1.f, EAttachLocation::Type::KeepWorldPosition, true);
		if(IsValid(SpawnEffectComponent))
		{
			UE_LOG(LogGame, Log, TEXT("Success Attached Spawn Effect"));
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Invalid SpawnParticleObj"), true, FVector2D(3.f, 3.f));
	}
}
