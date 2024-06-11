// Fill out your copyright notice in the Description page of Project Settings.

#include "Housing/SDKPreviewFurniture.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

#include "Character/SDKLobbyController.h"
#include "Manager/SDKTableManager.h"
#include "Share/SDKHelper.h"
#include "UI/SDKHousingModifyWidget.h"


ASDKPreviewFurniture::ASDKPreviewFurniture()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	IsPreviewActor = true;
	HudVisibility = true;

	// SpringArm
	SpringArmComponent = CreateDefaultSubobject<USDKSpringArmComponent>(TEXT("SpringArmComponenet"));
	SpringArmComponent->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	SpringArmComponent->SetRelativeRotation(FRotator(-75.f, 0.f, 0.f));

	SpringArmComponent->bEnableCameraLag = true;
	SpringArmComponent->bEnableCameraRotationLag = false;
	SpringArmComponent->bUseCameraLagSubstepping = true;

	SpringArmComponent->CameraLagSpeed = 3.f;
	SpringArmComponent->WallTypeLagSpeed = 1.5f;
	SpringArmComponent->GenerateTypeLagSpeed = 2.5f;
	SpringArmComponent->CameraRotationLagSpeed = 2.f;

	SpringArmComponent->CameraLagMinDistance = 100.f;
	SpringArmComponent->CameraLagMaxDistance = 700.f;

	SpringArmComponent->SetupAttachment(RootComponent);

	// Camera
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetHiddenInGame(true);
	CameraComponent->SetFieldOfView(60.f);
	CameraComponent->bUsePawnControlRotation = false;
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);

	// Tile
	TileComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileComponenet"));
	TileComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TileComponent->SetMobility(EComponentMobility::Movable);
	TileComponent->SetCastShadow(false);
	TileComponent->SetupAttachment(RootComponent);

	// HUD
	HUDComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HUDComponent"));
	HUDComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HUDComponent->SetEnableGravity(false);
	HUDComponent->SetVisibility(false);
	HUDComponent->bApplyImpulseOnDamage = false;

	HUDComponent->SetupAttachment(RootComponent);

	// 프리뷰 머터리얼 
	ConstructorHelpers::FObjectFinder<UMaterialInterface> EnablePreviewMaterial(TEXT("/Game/Effects/Materials/Fuzzy_arrow_blue.Fuzzy_arrow_blue"));
	if(EnablePreviewMaterial.Succeeded() == true)
	{
		EnableMaterial = EnablePreviewMaterial.Object;
	}

	ConstructorHelpers::FObjectFinder<UMaterialInterface> DisablePreviewMaterial(TEXT("/Game/Effects/Materials/Fuzzy_arrow_red.Fuzzy_arrow_red"));
	if(DisablePreviewMaterial.Succeeded() == true)
	{
		DisableMaterial = DisablePreviewMaterial.Object;
	}

	if(TileComponent == nullptr)
		return;

	ConstructorHelpers::FObjectFinder<UStaticMesh> Plan(TEXT("/Game/Environments/Myroom/Meshes/Plane.Plane"));
	if(Plan.Succeeded() == true)
	{
		TileComponent->SetStaticMesh(Plan.Object);
	}
}

#if WITH_EDITOR
void ASDKPreviewFurniture::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void ASDKPreviewFurniture::BeginPlay()
{
	Super::BeginPlay();

	IsPreviewActor = true;
}

void ASDKPreviewFurniture::InitFurnitureData()
{
	Super::InitFurnitureData();

	if(FurnitureType > EFurnitureType::Floor)
	{
		AttachHousingModifyWidget();
	}

	SetPreviewFurnitureSetting();
}

void ASDKPreviewFurniture::SetStaticMeshLocation(FVector vLocation)
{
	if(GetMeshComponent() == nullptr)
		return;

	Super::SetStaticMeshLocation(vLocation);

	SetPreviewTileLocationByMesh(vLocation);
}

void ASDKPreviewFurniture::SetStaticMeshLocationByType()
{
	if(GetMeshComponent() == nullptr)
		return;

	Super::SetStaticMeshLocationByType();

	SetPreviewTileLocationByMesh(GetMeshComponent()->GetRelativeTransform().GetLocation());
}

void ASDKPreviewFurniture::SetPreviewFurnitureSetting()
{
	SetCollisionEnabled(false);

	if(FurnitureType > EFurnitureType::Floor)
	{
		SetPreviewTileSize();
		SetPreviewTileLocation();
		SetPreviewTileRotation();
		SetVisibilityPreviewTile(true);

		SetModifyWidgetSetting();
	}
	else
	{
		SetStaticMeshScale(FVector(1.01f, 1.01f, 1.f));
		SetVisibilityPreviewTile(false);
	}
}

void ASDKPreviewFurniture::SetHudVisibility(bool bHud)
{
	HudVisibility = bHud;
}

void ASDKPreviewFurniture::SetFurnitureMaterialType(bool bPossibleArrange)
{
	if(GetMeshComponent() == nullptr || TileComponent == nullptr)
	{
		return;
	}

	if(bPossibleArrange == true)
	{
		GetMeshComponent()->SetMaterial(0, EnableMaterial);
		TileComponent->SetMaterial(0, EnableMaterial);
	}
	else
	{
		GetMeshComponent()->SetMaterial(0, DisableMaterial);
		TileComponent->SetMaterial(0, DisableMaterial);
	}
}

void ASDKPreviewFurniture::SetPreviewRotationSize(FVector vNewSize)
{
	Size = vNewSize;
}

void ASDKPreviewFurniture::SetWallHangingForwardRotator(bool bForward)
{
	if(FurnitureType != EFurnitureType::WallHangings)
		return;

	if(SpringArmComponent == nullptr)
		return;

	if(SpringArmComponent->bEnableCameraRotationLag == false)
	{
		SpringArmComponent->bEnableCameraRotationLag = true;
	}

	SpringArmComponent->SetWallTypesForwardRotator(bForward);
}

void ASDKPreviewFurniture::SetPreviewGroupTransform(FTransform tTransform)
{
	GroupingTransform = tTransform;
}

void ASDKPreviewFurniture::SetPreviewTileSize()
{
	if(TileComponent == nullptr)
		return;

	FVector vScale = FVector::OneVector;

	switch(FurnitureType)
	{
	case EFurnitureType::WallHangings:
		vScale.X = Size.X * 1.4f;
		vScale.Y = Size.Z * 1.4f;
		break;
	case EFurnitureType::Furniture:
	case EFurnitureType::Decoration:
	case EFurnitureType::Carpet:
		vScale.X = Size.X * 1.4f;
		vScale.Y = Size.Y * 1.4f;
		break;
	default:
		break;
	}

	TileComponent->SetRelativeScale3D(vScale);
}

void ASDKPreviewFurniture::SetPreviewTileRotation()
{
	if(TileComponent == nullptr)
		return;

	if(FurnitureType != EFurnitureType::WallHangings)
		return;

	FRotator tNewRotator = GetActorRotation();
	tNewRotator.Roll += 90.f;
	tNewRotator.Yaw -= 45.f;

	TileComponent->SetRelativeRotation(tNewRotator);
}

void ASDKPreviewFurniture::SetPreviewTileLocation()
{
	if(TileComponent == nullptr)
		return;

	FVector vLocation = FVector::ZeroVector;

	switch(FurnitureType)
	{
	case EFurnitureType::WallHangings:
		if(GetMeshComponent() == nullptr)
			break;

		vLocation = GetMeshComponent()->GetRelativeTransform().GetLocation();
		break;
	case EFurnitureType::Furniture:
	case EFurnitureType::Decoration:
	case EFurnitureType::Carpet:
		vLocation.Z = -Size.Z * (FMath::Sqrt(2.f) * 50.f) + 3.f;
		break;
	default:
		break;
	}

	TileComponent->SetRelativeLocation(vLocation);
}

void ASDKPreviewFurniture::SetPreviewTileLocationByMesh(FVector vLocation)
{
	if(TileComponent == nullptr)
		return;

	vLocation *= TILE_HALF;

	if(FurnitureType != EFurnitureType::WallHangings)
	{
		vLocation.Z = TileComponent->GetRelativeTransform().GetLocation().Z;
	}

	TileComponent->SetRelativeLocation(vLocation);
}

void ASDKPreviewFurniture::SetCameraActive()
{
	if(SpringArmComponent == nullptr || CameraComponent == nullptr)
		return;

	SpringArmComponent->SetCameraType(FurnitureType);

	if(FurnitureType == EFurnitureType::Floor)
	{
		SpringArmComponent->SetRelativeLocation(FVector(-1000.f, 300.f, 0.f));
		SpringArmComponent->TargetArmLength = 3500.f;
	}

	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
		return;

	LobbyController->SetViewTarget(this);
}

void ASDKPreviewFurniture::SetModifyWidgetSetting()
{
	if(HUDComponent == nullptr || HUDComponent->GetUserWidgetObject() == nullptr)
		return;

	USDKHousingModifyWidget* pModifyWidget = Cast<USDKHousingModifyWidget>(HUDComponent->GetUserWidgetObject());
	if(pModifyWidget == nullptr)
		return;

	if(FurnitureType == EFurnitureType::WallHangings)
	{
		pModifyWidget->SetButtonIsEnable(false, EModifyType::Rotation);
	}

	pModifyWidget->SetWidgetScale((int32)Size.X - 1);
}

void ASDKPreviewFurniture::SetVisibilityPreviewTile(bool bVisible)
{
	if(TileComponent == nullptr)
		return;

	TileComponent->SetVisibility(bVisible);
}

void ASDKPreviewFurniture::SetVisibilityModifyWidget(bool bVisible)
{
	if(bVisible == HudVisibility || HUDComponent == nullptr || HUDComponent->GetUserWidgetObject() == nullptr)
		return;

	USDKHousingModifyWidget* pModifyWidget = Cast<USDKHousingModifyWidget>(HUDComponent->GetUserWidgetObject());
	if(pModifyWidget == nullptr)
		return;

	SetHudVisibility(bVisible);

	pModifyWidget->OnPlayToggleAnim(HudVisibility);
}

void ASDKPreviewFurniture::SetVisibilityPreviewGroupModifyWidget(bool bVisible)
{
	if(HUDComponent == nullptr || HUDComponent->GetUserWidgetObject() == nullptr)
		return;

	USDKHousingModifyWidget* pModifyWidget = Cast<USDKHousingModifyWidget>(HUDComponent->GetUserWidgetObject());
	if(pModifyWidget == nullptr)
		return;

	pModifyWidget->SetVisibilityWidget(bVisible);
}

void ASDKPreviewFurniture::AttachHousingModifyWidget()
{
	if(HUDComponent == nullptr)
		return;

	if(HUDComponent->GetWidgetClass() != nullptr || HUDComponent->GetUserWidgetObject() != nullptr)
		return;

	auto WidgetTable = USDKTableManager::Get()->FindTableWidgetBluePrint(EWidgetBluePrint::Housing_ModifyHud);
	if(WidgetTable == nullptr)
		return;

	auto pWidget = LoadClass<USDKUserWidget>(nullptr, *WidgetTable->WidgetBluePrintPath.ToString());
	if (pWidget == nullptr)
		return;

	HUDComponent->SetWidgetClass(pWidget);
	HUDComponent->SetVisibility(true);

	SetVisibilityModifyWidget(false);
}

void ASDKPreviewFurniture::ApplyPreviewGroupingTransform()
{
	SetActorTransform(GroupingTransform);
}
