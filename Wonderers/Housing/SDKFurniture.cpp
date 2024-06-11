// Fill out your copyright notice in the Description page of Project Settings.

#include "Housing/SDKFurniture.h"

#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "UObject/ConstructorHelpers.h"

#include "Character/SDKHUD.h"
#include "GameMode/SDKGameState.h"
#include "Manager/SDKTableManager.h"
#include "Share/SDKEnum.h"
#include "Share/SDKHelper.h"

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
	if (ShadowMesh.Succeeded() == true)
	{
		ShadowMeshComponent->SetStaticMesh(ShadowMesh.Object);
	}

	ConstructorHelpers::FObjectFinder<UMaterialInterface> ShadowMaterial(TEXT("/Game/Characters/Public/Materials/M_Shadow.M_Shadow"));
	if (ShadowMaterial.Succeeded() == true)
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

	auto PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController == nullptr)
		return;

	auto MyHUD = Cast<ASDKHUD>(PlayerController->GetHUD());
	if (MyHUD == nullptr || MyHUD->GetUI(EUI::Lobby_Main) == nullptr)
		return;

	USDKLobbyWidget* LobbyWidget = Cast<USDKLobbyWidget>(MyHUD->GetUI(EUI::Lobby_Main));
	if(LobbyWidget == nullptr)
		return;

	LobbyWidget->NotifyClickEventFurniture(this);
}

void ASDKFurniture::NotifyActorOnInputTouchEnd(const ETouchIndex::Type FingerIndex)
{
	Super::NotifyActorOnInputTouchEnd(FingerIndex);

	if(FurnitureType <= EFurnitureType::Floor || IsPreviewActor)
		return;

	auto PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController == nullptr)
		return;

	auto MyHUD = Cast<ASDKHUD>(PlayerController->GetHUD());
	if (MyHUD == nullptr || MyHUD->GetUI(EUI::Lobby_Main) == nullptr)
		return;

	USDKLobbyWidget* LobbyWidget = Cast<USDKLobbyWidget>(MyHUD->GetUI(EUI::Lobby_Main));
	if(LobbyWidget == nullptr)
		return;

	LobbyWidget->NotifyClickEventFurniture(this);
}

void ASDKFurniture::InitFurnitureData()
{
	if(FurnitureID.IsEmpty() == true)
	{
		return;
	}

	auto FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(FurnitureID);
	auto ItemTable = USDKTableManager::Get()->FindTableItem(FurnitureID);
	if(FurnitureTable == nullptr || ItemTable == nullptr)
	{
		UE_LOG(LogGame, Error, TEXT("Not Found MyroomParts Table : %s"), *FurnitureID);
		return;
	}

	SetFurnitureType(FurnitureTable->Type);
	SetFurnitureSize(FurnitureTable->Size);
	SetStaticMeshLocationByType();
	
	// BP가 아닌 경우에만 처리
	if (FurnitureTable->PartsClass.GetUniqueID().IsAsset() == false)
	{
		if(ItemTable->MeshPath.GetUniqueID().IsAsset() == true)
		{
			auto FurnitureMesh = LoadObject<UStaticMesh>(nullptr, *ItemTable->MeshPath.ToString());
			if (FurnitureMesh)
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
			if(ItemTable->MeshPath.GetUniqueID().IsAsset() == true)
			{
				auto FurnitureMesh = LoadObject<UStaticMesh>(nullptr, *ItemTable->MeshPath.ToString());
				if (FurnitureMesh)
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
			MeshComponent->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
		}
	}

	// 그림자 처리
	SetFurnitureShadow();
}

void ASDKFurniture::SetUniqueID(FString newUniqueID)
{
	if(newUniqueID.IsEmpty() == true)
	{
		return;
	}

	UniqueID = newUniqueID;
}

void ASDKFurniture::SetFurnitureID(FString NewTableID)
{
	if(NewTableID == FurnitureID || NewTableID.IsEmpty() == true)
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
	if(MeshComponent != nullptr)
	{
		return MeshComponent->GetRelativeTransform().GetLocation();
	}

	return FVector::ZeroVector;
}

void ASDKFurniture::SetStaticMeshLocationByType()
{
	if(MeshComponent != nullptr)
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
	if(MeshComponent != nullptr)
	{
		vLocation *= TILE_HALF;
		vLocation.Z = GetStaticMeshLocation().Z;

		MeshComponent->SetRelativeLocation(vLocation);
	}
}

FVector ASDKFurniture::GetStaticMeshScale() const
{
	if(MeshComponent != nullptr)
	{
		return MeshComponent->GetRelativeTransform().GetScale3D();
	}
	
	return FVector::ZeroVector;
}

void ASDKFurniture::SetStaticMeshScale(FVector vScale)
{
	if(MeshComponent != nullptr)
	{
		MeshComponent->SetRelativeScale3D(vScale);
	}
}

void ASDKFurniture::SetStaticMesh(UStaticMesh * NewMesh)
{
	if(MeshComponent != nullptr)
	{
		MeshComponent->SetStaticMesh(NewMesh);
	}
}

void ASDKFurniture::SetStaticMeshMaterial(UMaterialInterface* NewMaterial)
{
	if(MeshComponent != nullptr && NewMaterial != nullptr)
	{
		MeshComponent->SetMaterial(0, NewMaterial);
	}
}

void ASDKFurniture::SetCollisionSize(FVector vNewSize)
{
	if(CollisionComponent != nullptr)
	{
		CollisionComponent->SetBoxExtent(CollsionSize * vNewSize);
	}
}

void ASDKFurniture::SetCollisionEnabled(bool bEnable)
{
	if(MeshComponent == nullptr)
	{
		return;
	}

	if(bEnable == true)
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ASDKFurniture::SetFurnitureShadow()
{
	if (ShadowMeshComponent == nullptr)
	{
		return;
	}

	if (FurnitureType == EFurnitureType::Furniture || FurnitureType == EFurnitureType::Decoration)
	{
		ShadowMeshComponent->SetVisibility(true);
		ShadowMeshComponent->SetRelativeScale3D(FVector(Size.X, Size.Y, 1.f) * 3.f);
	}
}

void ASDKFurniture::SetFurnitureArrangeEffect()
{
	UParticleSystem* SpawnParticleObj = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/Effects/Skill/Public/My_Room_Dust.My_Room_Dust"), nullptr, LOAD_None, nullptr);
	if(SpawnParticleObj != nullptr)
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
		if(SpawnEffectComponent != nullptr)
		{
			UE_LOG(LogGame, Log, TEXT("Success Attached Spawn Effect"));
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Invalid SpawnParticleObj"), true, FVector2D(3.f, 3.f));
	}
}