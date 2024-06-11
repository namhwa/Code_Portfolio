// Fill out your copyright notice in the Description page of Project Settings.

#include "Housing/SDKHousingWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Kismet/GameplayStatics.h"

#include "Character/SDKHUD.h"
#include "Share/SDKData.h"
#include "Character/SDKMyInfo.h"
#include "Share/SDKHelper.h"
#include "Manager/SDKTableManager.h"
#include "Character/SDKLobbyController.h"
#include "Character/SDKPlayerCharacter.h"
#include "Engine/SDKBlueprintFunctionLibrary.h"
#include "UI/SDKWidgetParam.h"
#include "UI/SDKLobbyWidget.h"
#include "UI/SDKMessageBoxWidget.h"
#include "UI/SDKFurnitureBoxWidget.h"
#include "UI/SDKHousingButtonWidget.h"
#include "UI/SDKHousingCategoryWidget.h"
#include "Object/SDKFurniture.h"
#include "Object/SDKPreviewFurniture.h"
#include "UI/SDKCheckBoxParam.h"

DEFINE_LOG_CATEGORY(LogHousing)

USDKHousingWidget::USDKHousingWidget()
{
	ClearHousingParameters();
}

void USDKHousingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitHousingWidget();
}

FReply USDKHousingWidget::NativeOnTouchGesture(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	// hajang : 모바일에서 카메라 드래그 확인 필요

	if(InGestureEvent.GetGestureType() == EGestureEvent::Swipe)
	{
		auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		if(LobbyController != nullptr && BorderBackground != nullptr)
		{
			if(BorderBackground->GetVisibility() == ESlateVisibility::Visible)
			{
				LobbyController->MoveHousingViewCamera(InGestureEvent.GetGestureDelta());
			}
		}
	}

	return Super::NativeOnTouchGesture(InGeometry, InGestureEvent);
}

void USDKHousingWidget::OpenProcess()
{
	ClearHousingParameters();

	SetToggleProcess(true);

	Super::OpenProcess();
}

void USDKHousingWidget::CloseProcess()
{
	SetToggleProcess(false);

	Super::CloseProcess();
}


void USDKHousingWidget::InitHousingWidget()
{
	InitButtonSetting();

	SetFurnitureList();

	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(SDKHUD == nullptr || SDKHUD->GetUI(EUI::Lobby_Main) == nullptr)
		return;

	USDKLobbyWidget* pLobbyWidget = Cast<USDKLobbyWidget>(SDKHUD->GetUI(EUI::Lobby_Main));
	if(pLobbyWidget == nullptr)
		return;

	pLobbyWidget->SetSDKHousingWidget(this);
}

void USDKHousingWidget::InitButtonSetting()
{
	if(ButtonReset != nullptr)
	{
		if(ButtonReset->OnClicked.Contains(this, TEXT("ToggleResetMessage")) == false)
		{
			ButtonReset->OnClicked.AddDynamic(this, &USDKHousingWidget::ToggleResetMessage);
		}
	}

	for(int32 idx = 0; idx < 3; ++idx)
	{
		CreateMenuButtonWidget(idx);
	}
}

void USDKHousingWidget::SetToggleProcess(bool bOpen)
{
	SetVisibilityModifyFurniture(false);
	SetHousingState(EHousingState::None);
	SetCloseUIHandleBinding(bOpen);

	ToggleFurnitureList(true);

	// 변수 초기화
	ModifyGroup.Clear();
	RemovePreviewFurniture();

	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
		return;

	LobbyController->SetEnableClickEvent(bOpen);
	LobbyController->InitHousingCamera(bOpen);
	LobbyController->SetUseInputTouch(bOpen);
	LobbyController->OnSetVisibilityVirtualJoystick(!bOpen);

	auto PlayerPawn = Cast<ASDKPlayerCharacter>(LobbyController->GetPawn());
	if(PlayerPawn == nullptr)
		return;

	PlayerPawn->SetActorHiddenInGame(bOpen);
	PlayerPawn->SetActorEnableCollision(!bOpen);

	if(bOpen == true)
	{
		PlayerPawn->ClientInputModeChange(false);
		SetFurnitureList();
	}
	else
	{
		PlayerPawn->ClientInputModeChange(true);
		SetLobbyControllerInputMode(bOpen);
	}
}

void USDKHousingWidget::SetCloseUIHandleBinding(bool bBind)
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(SDKHUD == nullptr)
		return;

	if(bBind)
	{
		if(CloseUIHandle.IsValid() == true)
			return;

		CloseUIHandle = SDKHUD->CloseSubUIEvent.AddUObject(this, &USDKHousingWidget::ToggleCloseMessage);
	}
	else
	{
		SDKHUD->CloseMessageBox();

		if(CloseUIHandle.IsValid() == false)
			return;

		SDKHUD->CloseSubUIEvent.Remove(CloseUIHandle);
		CloseUIHandle.Reset();
	}
}

void USDKHousingWidget::SetFurnitureList()
{
	TMap<int32, CItemData> OwnedHousingItems;
	g_pMyInfo->GetInvenDataList().GetOutTypeItemDataMap(EItemType::Housing, OwnedHousingItems);

	CHousingData HousingData = g_pMyInfo->GetHousingData();

	// 소팅
	FSDKHelpers::SortDescendingItemID(OwnedHousingItems);

	mapFurnitures.Reserve(OwnedHousingItems.Num());
	for(auto& iter : OwnedHousingItems)
	{
		if(mapFurnitures.Find(iter.Value.m_ItemID) == nullptr)
		{
			CreateFurnitrueBoxWidget(iter.Value.m_ItemID);
		}

		int32 inewCount = iter.Value.m_ItemCount - HousingData.GetArrangedItemCount(iter.Value.m_ItemID);
		mapFurnitures[iter.Value.m_ItemID]->SetOwnedCount(inewCount);
	}
}

void USDKHousingWidget::SetFurnitureCount(FString FurnitureID, int32 NewCount)
{
	if(mapFurnitures.Contains(FurnitureID) == false)
		return;

	auto FurnitureBoxWidget = mapFurnitures[FurnitureID];

	int32 OwnedCount = FurnitureBoxWidget->GetOwnedCount() + NewCount;
	FurnitureBoxWidget->SetOwnedCount(OwnedCount);
}

void USDKHousingWidget::SetLobbyControllerInputMode(bool bGameOnly)
{
	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
		return;

	LobbyController->SetInputGameMode(bGameOnly);
}

void USDKHousingWidget::SetActiveHousingCamera(bool bActive)
{
	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
		return;

	if(bActive)
	{
		LobbyController->OnStartFurnitureCamera();

		if(PreviewGroup.BaseFurniture == nullptr)
			return;

		PreviewGroup.BaseFurniture->SetCameraActive();
	}
	else
	{
		LobbyController->OnEndFurnitureCamera();
	}
}

void USDKHousingWidget::SetHousingState(EHousingState state)
{
	HousingState = state;
}

void USDKHousingWidget::SetActiveBackgroundBorder(bool bActive)
{
	if(BorderBackground == nullptr)
		return;

	if(bActive == true)
	{
		BorderBackground->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		BorderBackground->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void USDKHousingWidget::SetHiddenListState(bool bHidden)
{
	bHiddenListState = bHidden;

	ToggleFurnitureList(bHiddenListState);
}

void USDKHousingWidget::SetPreviewFurnitureCollision(bool bEnable)
{
	if(PreviewGroup.BaseFurniture == nullptr || HousingState == EHousingState::PutModeState)
		return;

	PreviewGroup.BaseFurniture->SetCollisionEnabled(bEnable);
}

void USDKHousingWidget::SetPreviewGroupMaterialType(bool bEnable)
{
	if(PreviewGroup.BaseFurniture == nullptr)
		return;

	PreviewGroup.BaseFurniture->SetFurnitureMaterialType(bEnable);
	for(auto& iter : PreviewGroup.Decorations)
	{
		if(iter == nullptr)
			continue;

		iter->SetFurnitureMaterialType(bEnable);
	}
}

void USDKHousingWidget::SetModifyFurniture(ASDKFurniture* NewFurniture)
{
	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
		return;

	if(NewFurniture != nullptr)
	{
		ModifyGroup.BaseFurniture = NewFurniture;

		if(NewFurniture->GetFurnitureType() != EFurnitureType::Decoration)
		{
			// 가구 위에 장식 있는지 확인 : 있는 경우 같이 처리
			LobbyController->CheckDecorationOnFurniture(NewFurniture->GetArrangedRoomIndex(), NewFurniture->GetUniqueID(), NewFurniture->GetSavedIndex(), NewFurniture->GetFurnitureSize(), ModifyGroup.Decorations);
		}
	}

	if(ModifyGroup.BaseFurniture != nullptr)
	{
		SetVisibilityModifyFurniture(HousingState == EHousingState::ModifyModeState);
	}
}

void USDKHousingWidget::SetModifyMeshOffsetRotator(FVector vSize)
{
	if(vSize.X == vSize.Y)
		return;

	FVector vInit = FVector(vSize.Y, vSize.X, vSize.Z) - vSize;
	FVector2D vInterval = FVector2D(FMath::Abs(vInit.X), FMath::Abs(vInit.Y));

	RotatorMeshOffset.Empty(4);
	for(int32 idx = 0; idx < 4; ++idx)
	{
		FVector2D vOffset = vInterval;
		if(idx < 2)
			vOffset.X *= -1.f;

		if(idx == 1 || idx == 2)
			vOffset.Y *= -1.f;

		RotatorMeshOffset.Add(vOffset);
	}
}

void USDKHousingWidget::SetVisibilityModifyFurniture(bool bVisible)
{
	if(ModifyGroup.BaseFurniture == nullptr)
		return;

	ModifyGroup.BaseFurniture->SetActorHiddenInGame(bVisible);
	ModifyGroup.BaseFurniture->SetActorEnableCollision(!bVisible);

	for(auto& iter : ModifyGroup.Decorations)
	{
		iter->SetActorHiddenInGame(bVisible);
		iter->SetActorEnableCollision(!bVisible);
	}
}


// 정렬
void USDKHousingWidget::SortFurnitureBoxByCategory(EFurnitureCategory eCategory)
{
	for(auto& iter : mapFurnitures)
	{
		if(eCategory == EFurnitureCategory::None)
		{
			iter.Value->SetWidgetVisibility(true);
			continue;
		}

		iter.Value->SetWidgetVisibility(eCategory == iter.Value->GetFurnitureCategoryType());
	}
}


// 검사 관련 함수
bool USDKHousingWidget::CheckMovablePickFurniture(ETouchIndex::Type type)
{
	if(HousingState != EHousingState::ModifyModeState && HousingState != EHousingState::PutModeState)
		return false;

	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
		return false;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = { OBJECT_OBJECT };
	FHitResult HitResult;

#if PLATFORM_WINDOWS
	if(LobbyController->GetHitResultUnderCursorForObjects(ObjectTypes, true, HitResult) == false)
#else 
	if(LobbyController->GetHitResultUnderFingerForObjects(type, ObjectTypes, true, HitResult) == false)
#endif
	{
		UE_LOG(LogHousing, Log, TEXT("No Have Hit Result under the cursor"));
		return false;
	}

	ASDKFurniture* HitTarget = Cast<ASDKFurniture>(HitResult.Actor);
	if(HitTarget != nullptr)
	{
		// 드래그 OR 수정 모드
		if(HousingState == EHousingState::ModifyModeState)
		{
			if(ModifyGroup.BaseFurniture != nullptr && PreviewGroup.BaseFurniture != nullptr && PreviewGroup.BaseFurniture == HitTarget)
			{
				SetHousingState(EHousingState::ModifyMovableState);
				SetPreviewFurnitureCollision(false);

				return true;
			}
		}
		else if(HousingState == EHousingState::PutModeState)
		{
			if(HitTarget == PreviewGroup.BaseFurniture)
			{
				SetHousingState(EHousingState::PutMovableState);
				SetPreviewFurnitureCollision(false);

				return true;
			}
		}
	}

	return false;
}

void USDKHousingWidget::CheckClickEventMyroomFurnitur(ASDKFurniture* ClickedObject)
{
	if(ClickedObject == nullptr || HousingState == EHousingState::PutModeState)
		return;

	if(ClickedObject->GetIsPreviewActor() == false && ModifyGroup.BaseFurniture == nullptr && HousingState == EHousingState::None)
	{
		ToggleModifyMode(true, ClickedObject);
	}
}

bool USDKHousingWidget::CheckDecorationPutableType(ECollisionChannel eCollisionType, AActor* HitActor)
{
	bool bResult = false;
	if(HitActor == nullptr)
		return bResult;

	ASDKFurniture* OvelapFurniture = Cast<ASDKFurniture>(HitActor);
	if(OvelapFurniture != nullptr)
	{
		EFurnitureType ePrevType = PreviewGroup.BaseFurniture->GetFurnitureType();
		EFurnitureType eOverlapType = OvelapFurniture->GetFurnitureType();

		if(eCollisionType != COLLISION_OBJECT)
			return bResult;

		bResult = (PreviewGroup.BaseFurniture == OvelapFurniture) || (ePrevType == EFurnitureType::Decoration && eOverlapType == EFurnitureType::Furniture) ? true : false;
	}

	return bResult;
}

bool USDKHousingWidget::CheckGeneralPreviewPutable(FHitResult tHitResult)
{
	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
		return false;

	ASDKFurniture* OverlapObject = Cast<ASDKFurniture>(tHitResult.GetActor());
	if(OverlapObject == nullptr)
		return false;

	// 현재 가구 타입
	EFurnitureType ePreviewType = PreviewGroup.BaseFurniture->GetFurnitureType();

	// 벽장식 회전값 계산
	if(ePreviewType == EFurnitureType::WallHangings)
	{
		CalcWallFurnitureRotator(OverlapObject);
	}

	// 처리 여부 변수
	bool bResultPos, bResultType;

	// 현재 방
	int32 iRoomIndex = OverlapObject->GetArrangedRoomIndex();

	// 크기
	FVector vSize = PreviewGroup.BaseFurniture->GetFurnitureSize();

	// 타입에 따른 인덱스 및 Z값 설정
	FVector vIndex = LobbyController->CalcPickingTileIndex(iRoomIndex, tHitResult.Location);
	if(ePreviewType > EFurnitureType::Floor && ePreviewType != EFurnitureType::WallHangings)
	{
		vIndex.Z = 0.f;
	}

	// 위치
	FVector vLocation = LobbyController->CalcFurnitureCenterLocation(iRoomIndex, vIndex, vSize);

	UpdatePreviewFurnitureData(iRoomIndex, vLocation, vIndex);

	// 배치 가능 위치 확인
	if(HousingState == EHousingState::ModifyModeState || HousingState == EHousingState::ModifyMovableState)
	{
		bResultPos = LobbyController->CalcPossibleArrangeTile(iRoomIndex, vIndex, vSize, ModifyGroup.BaseFurniture->GetUniqueID());
	}
	else
	{
		bResultPos = LobbyController->CalcPossibleArrangeTile(iRoomIndex, vIndex, vSize);
	}

#if WITH_EDITOR
	GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Green, FString::Printf(TEXT("TileData : %s"), bResultPos ? TEXT("True") : TEXT("False")));
#endif

	// 가구 타입과 히트 타입 확인
	if(tHitResult.GetComponent()->GetCollisionObjectType() != COLLISION_WALL && tHitResult.GetComponent()->GetCollisionObjectType() != COLLISION_FLOOR)
		return false;

	bResultType = LobbyController->CalcPossibleArrangeType(iRoomIndex, vIndex, vSize, PreviewGroup.BaseFurniture->GetFurnitureID());

	return bResultPos && bResultType;
}

bool USDKHousingWidget::CheckDecorationPreviewPutable(FHitResult tHitResult)
{
	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
		return false;

	ASDKFurniture* OverlapObject = Cast<ASDKFurniture>(tHitResult.GetActor());
	if(OverlapObject == nullptr)
		return false;

	// 처리 여부 변수
	bool bResultPos, bResultType;

	// 콜리전 오브젝트 타입
	ECollisionChannel CollisionType = tHitResult.GetComponent()->GetCollisionObjectType();

	// 현재 방
	int32 iRoomIndex = OverlapObject->GetArrangedRoomIndex();

	// 인덱스, 위치
	FVector vSize = PreviewGroup.BaseFurniture->GetFurnitureSize();

	FVector vIndex = FVector(-1.f);
	FVector vLocation = FVector::ZeroVector;
	if(CollisionType == COLLISION_OBJECT)
	{
		vIndex = LobbyController->CalcDecorationPickingTileIndex(iRoomIndex, tHitResult.Location);
		vLocation = LobbyController->CalcDecorationCenterLocation(iRoomIndex, vIndex, vSize, tHitResult.Location.Z);
	}
	else
	{
		vIndex = LobbyController->CalcPickingTileIndex(iRoomIndex, tHitResult.Location);
		vLocation = LobbyController->CalcFurnitureCenterLocation(iRoomIndex, vIndex, vSize);
	}

	UpdatePreviewFurnitureData(iRoomIndex, vLocation, vIndex);

	// 배치 가능 위치 확인
	if(HousingState == EHousingState::ModifyModeState || HousingState == EHousingState::ModifyMovableState)
	{
		bResultPos = LobbyController->CalcPossibleArrangeTile(iRoomIndex, vIndex, vSize, ModifyGroup.BaseFurniture->GetUniqueID());
	}
	else
	{
		bResultPos = LobbyController->CalcPossibleArrangeTile(iRoomIndex, vIndex, vSize);
	}

	// 가구 타입과 히트 타입 확인
	if(CollisionType == COLLISION_WALL || CollisionType == COLLISION_FLOOR)
	{
		bResultType = LobbyController->CalcPossibleArrangeType(iRoomIndex, vIndex, vSize, PreviewGroup.BaseFurniture->GetFurnitureID());
	}
	else
	{
		bResultType = CheckDecorationPutableType(CollisionType, OverlapObject);
	}

	return bResultPos && bResultType;
}

bool USDKHousingWidget::CheckCreatePreviewPosition()
{
	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
		return false;

	if(LobbyController->GetHousingViewCamera() == nullptr)
		return false;

	EFurnitureType PreviewType = PreviewGroup.BaseFurniture->GetFurnitureType();
	if(PreviewType == EFurnitureType::Wall || PreviewType == EFurnitureType::Floor)
		return true;

	FVector CameraLoc = LobbyController->GetHousingViewCamera()->GetActorLocation();
	FVector CameraDir = LobbyController->GetHousingViewCamera()->GetCameraComponent()->GetForwardVector();

	FHitResult HitResult;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = { OBJECT_OBJECT, OBJECT_WALL, OBJECT_FLOOR };
	if(GetWorld()->LineTraceSingleByObjectType(HitResult, CameraLoc, CameraLoc + CameraDir * 100000.f, FCollisionObjectQueryParams(ObjectTypes)) == false)
	{
		PreviewGroup.BaseFurniture->SetActorLocation(FVector(-1400.f, 300.f, 210.f));
	}
	else
	{
		CheckGeneralPreviewPutable(HitResult);
	}

	LobbyController->SetLocationHousingCamera(PreviewGroup.BaseFurniture->GetActorLocation());

	return true;
}

void USDKHousingWidget::CalcWallFurnitureRotator(AActor* HitActor)
{
	FVector vResult = FVector::ZeroVector;

	if(HitActor == nullptr)
	{
		return;
	}

	ASDKFurniture* HitFurniture = Cast<ASDKFurniture>(HitActor);
	if(HitFurniture == nullptr || HitFurniture->GetFurnitureType() != EFurnitureType::Wall)
	{
		return;
	}

	auto MyroomBasicTable = USDKTableManager::Get()->FindTableMyroomBasic(HitFurniture->GetBaseTableID());
	if(MyroomBasicTable == nullptr)
	{
		return;
	}

	auto FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(PreviewGroup.BaseFurniture->GetFurnitureID());
	if(FurnitureTable == nullptr)
		return;

	FVector vSize = FurnitureTable->Size;
	float fRotatorValue = 135.f;
	if(MyroomBasicTable->Direction == false)
	{
		fRotatorValue = 45.f;
		PreviewGroup.BaseFurniture->SetFurnitureSize(vSize);
	}
	else
	{
		PreviewGroup.BaseFurniture->SetFurnitureSize(FVector(vSize.Y, vSize.X, vSize.Z));
	}

	PreviewGroup.BaseFurniture->SetActorRotation(FRotator(0.f, fRotatorValue, 0.f));
	PreviewGroup.BaseFurniture->SetWallHangingForwardRotator(!MyroomBasicTable->Direction);
}

void USDKHousingWidget::CalcRotatorSavedIndex(float fRotatorYaw, FVector& vSavedIndex, FVector& vSize)
{
	vSize = FVector(vSize.Y, vSize.X, vSize.Z);

	int32 iRotatorValue = fRotatorYaw / 90.f;
	if(iRotatorValue == 1)
	{
		vSavedIndex.Y -= int32(vSize.Y - 1.f);
	}
	else if(iRotatorValue == 2)
	{
		vSavedIndex.X -= int32(vSize.X - 1.f);
	}
}

void USDKHousingWidget::CreateMenuButtonWidget(int32 index)
{
	auto WidgetTable = USDKTableManager::Get()->FindTableWidgetBluePrint(EWidgetBluePrint::Housing_btn);
	if (WidgetTable == nullptr)
		return;

	if (WidgetTable->WidgetBluePrintPath.GetUniqueID().IsAsset() == false)
		return;

	auto pWidget = LoadClass<USDKUserWidget>(nullptr, *WidgetTable->WidgetBluePrintPath.ToString());
	if (pWidget == nullptr)
		return;

	USDKHousingButtonWidget* pButtonWidget = CreateWidget<USDKHousingButtonWidget>(UGameplayStatics::GetPlayerController(GetWorld(), 0), pWidget);
	if(pButtonWidget == nullptr)
		return;

	pButtonWidget->SetHousingMenuIndex(index);
	pButtonWidget->SetButtonSize(FVector2D(60.f, 60.f));

	if(HorizontalBoxMenu != nullptr)
	{
		HorizontalBoxMenu->AddChildToHorizontalBox(pButtonWidget);
	}

	MenuButtons.Emplace(pButtonWidget);
}

void USDKHousingWidget::CreateFurnitrueBoxWidget(FString TableID)
{
	if(TableID.IsEmpty() == true)
		return;

	auto WidgetTable = USDKTableManager::Get()->FindTableWidgetBluePrint(EWidgetBluePrint::Housing_Box);
	if(WidgetTable != nullptr)
	{
		if (WidgetTable->WidgetBluePrintPath.GetUniqueID().IsAsset() == true)
		{
			auto pWidget = LoadClass<USDKUserWidget>(nullptr, *WidgetTable->WidgetBluePrintPath.ToString());
			if (pWidget != nullptr)
			{
				USDKFurnitureBoxWidget* FurnitureWidget = CreateWidget<USDKFurnitureBoxWidget>(GetOwningPlayer(), pWidget);
				if (FurnitureWidget == nullptr)
					return;

				mapFurnitures.Add(TableID, FurnitureWidget);
				HorizontalBoxList->AddChildToHorizontalBox(FurnitureWidget);

				FurnitureWidget->SetTableID(TableID);
			}
		}
	}
}

// 예시 가구 관련
void USDKHousingWidget::CreatePreviewFurniture(FString TableID)
{
	if(TableID.IsEmpty() == true)
		return;

	if(PreviewGroup.BaseFurniture == nullptr)
	{
		auto FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(TableID);
		if(FurnitureTable != nullptr)
		{
			FVector vLoaction = FVector::ZeroVector;
			FRotator vRotator = FRotator::ZeroRotator;
			if(FurnitureTable->Type == EFurnitureType::Wall || FurnitureTable->Type == EFurnitureType::Floor)
			{
				vLoaction = FurnitureTable->Type == EFurnitureType::Floor ? FVector(100.f, -100.f, 150.f) : FVector(-500.f, 320.f, 70.f);
				vRotator = FRotator(0.f, -45.f, 0.f);
			}

			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			auto pPreview = GetWorld()->SpawnActor<ASDKPreviewFurniture>(ASDKPreviewFurniture::StaticClass(), vLoaction, vRotator, SpawnInfo);
			if(pPreview != nullptr)
			{

				pPreview->SetFurnitureID(TableID);
				pPreview->SetVisibilityModifyWidget(false);

				PreviewGroup.BaseFurniture = pPreview;

				SetPreviewGroupMaterialType(true);
				SetPreviewFurnitureCollision(true);
			}
		}
	}
}

void USDKHousingWidget::CreateModifyPreviewFurniture(ASDKFurniture* pBaseFurniture, ASDKPreviewFurniture*& pPreview)
{
	if(ModifyGroup.BaseFurniture == nullptr)
		return;

	FRotator vRotator = FRotator::ZeroRotator;
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	pPreview = GetWorld()->SpawnActor<ASDKPreviewFurniture>(ASDKPreviewFurniture::StaticClass(), FVector::ZeroVector, vRotator, SpawnInfo);
	if(pPreview == nullptr)
		return;

	pPreview->SetFurnitureID(pBaseFurniture->GetFurnitureID());

	pPreview->SetFurnitureMaterialType(true);
	pPreview->SetVisibilityModifyWidget(false);

	pPreview->SetFurnitureSize(pBaseFurniture->GetFurnitureSize());
	pPreview->SetStaticMeshScale(pBaseFurniture->GetStaticMeshScale());
	pPreview->SetActorRelativeTransform(pBaseFurniture->GetActorTransform());
	pPreview->SetSavedIndex(pBaseFurniture->GetSavedIndex());

	pPreview->SetUniqueID(pBaseFurniture->GetUniqueID());
	pPreview->SetArrangedRoomIndex(pBaseFurniture->GetArrangedRoomIndex());
}

void USDKHousingWidget::RemovePreviewFurniture()
{
	if(PreviewGroup.BaseFurniture == nullptr)
		return;

	PreviewGroup.Remove();
}

bool USDKHousingWidget::StartMovablePreviewFurniture(ETouchIndex::Type type)
{
	if(CheckMovablePickFurniture(type) == false)
		return false;

	SetActiveBackgroundBorder(false);
	SetPreviewGroupMaterialType(true);

	SetLobbyControllerInputMode(true);
	SetActiveHousingCamera(true);

	if(PreviewGroup.BaseFurniture == nullptr)
		return false;

	PreviewGroup.BaseFurniture->SetVisibilityModifyWidget(false);

	return true;
}

void USDKHousingWidget::UpdatePreviewFurniture(ETouchIndex::Type type)
{
	if(PreviewGroup.BaseFurniture == nullptr)
		return;

	if(HousingState == EHousingState::ModifyModeState || HousingState == EHousingState::PutModeState)
		return;

	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
	{
		RemovePreviewFurniture();
		return;
	}

	// 커서 밑 오브젝트 충돌 검사
	bool bResult = false;

	FHitResult HitResult;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = { OBJECT_OBJECT, OBJECT_WALL, OBJECT_FLOOR, OBJECT_CUBE };

	EFurnitureType FurnitureType = PreviewGroup.BaseFurniture->GetFurnitureType();

	if(FurnitureType == EFurnitureType::Wall || FurnitureType == EFurnitureType::Floor)
	{
		ObjectTypes.Remove(OBJECT_OBJECT);
		ObjectTypes.Remove(OBJECT_CUBE);
	}

#if PLATFORM_WINDOWS
	if(LobbyController->GetHitResultUnderCursorForObjects(ObjectTypes, true, HitResult) == false)
#else 
	if(LobbyController->GetHitResultUnderFingerForObjects(type, ObjectTypes, true, HitResult) == false)
#endif
	{
		UE_LOG(LogHousing, Log, TEXT("No Have Hit Result under the cursor"));
	}
	else
	{
		PreviewHitResult = HitResult;

		if(FurnitureType == EFurnitureType::Wall || FurnitureType == EFurnitureType::Floor)
		{
			bResult = UpdatePreviewWallAndFloor();
			SetPreviewGroupMaterialType(true);
		}
		else
		{
			bResult = UpdatePreviewGeneral();
			SetPreviewGroupMaterialType(bResult);
		}
	}

	LobbyController->SetIsPutableFurnitrue(bResult);
}

bool USDKHousingWidget::UpdatePreviewGeneral()
{
	bool bResult = false;
	if(PreviewGroup.BaseFurniture->GetFurnitureType() == EFurnitureType::Decoration)
	{
		bResult = CheckDecorationPreviewPutable(PreviewHitResult);
	}
	else
	{
		bResult = CheckGeneralPreviewPutable(PreviewHitResult);
	}

	// 매쉬 위치 변경된 경우, 다시 조정
	if(PreviewGroup.BaseFurniture->GetFurnitureType() != EFurnitureType::WallHangings)
	{
		PreviewGroup.BaseFurniture->SetStaticMeshLocation(FVector::ZeroVector);
	}

#if WITH_EDITOR
	GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::White, PreviewGroup.BaseFurniture->GetSavedIndex().ToString());
#endif

	return bResult;
}

bool USDKHousingWidget::UpdatePreviewWallAndFloor()
{
	bool bResult = false;

	ASDKFurniture* pHitActor = Cast<ASDKFurniture>(PreviewHitResult.Actor);
	if(pHitActor != nullptr)
	{
		bResult = (PreviewGroup.BaseFurniture->GetFurnitureType() == EFurnitureType::Wall && pHitActor->GetFurnitureType() == EFurnitureType::Wall)
			|| (PreviewGroup.BaseFurniture->GetFurnitureType() == EFurnitureType::Floor && pHitActor->GetFurnitureType() == EFurnitureType::Floor);
	}

	if(bResult)
	{
		FRotator tRotator = pHitActor->GetActorRotation();
		FVector vLocation = pHitActor->GetActorLocation();

		auto MyroomBasicTable = USDKTableManager::Get()->FindTableMyroomBasic(pHitActor->GetBaseTableID());
		if(MyroomBasicTable != nullptr)
		{
			if(MyroomBasicTable->Type == true)
			{
				vLocation.X -= 20.f;
				vLocation.Y += 20.f;
				vLocation.Z += 200.f;

				tRotator = FRotator::ZeroRotator;
			}
			else
			{
				tRotator.Yaw = 0.f;
				if(MyroomBasicTable->Direction == false)
				{
					vLocation.Y += 10.f;
				}
				else
				{
					vLocation.X -= 20.f;
					vLocation.Y += 100.f;
				}
			}

			if (MyroomBasicTable->MeshPath.GetUniqueID().IsAsset() == true)
			{
				auto FurnitureMesh = LoadObject<UStaticMesh>(nullptr, *MyroomBasicTable->MeshPath.ToString());
				if (FurnitureMesh)
				{
					PreviewGroup.BaseFurniture->SetStaticMesh(FurnitureMesh);
				}
				else
				{
					FString Str = FString::Printf(TEXT("Invalid MyroomBasicTable MeshPath : (%s)"), *MyroomBasicTable->MeshPath.ToString());
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Str, true, FVector2D(3.f, 3.f));
				}
			}
		}
		else
		{
			PreviewGroup.BaseFurniture->SetStaticMesh(nullptr);
		}

		PreviewGroup.BaseFurniture->SetActorRelativeLocation(vLocation);
		PreviewGroup.BaseFurniture->SetActorRelativeRotation(tRotator);
	}

	return bResult;
}

void USDKHousingWidget::UpdatePreviewFurnitureData(int32 iRoomIndex, FVector vLocation, FVector vIndex)
{
	if(PreviewGroup.BaseFurniture == nullptr)
		return;

	PreviewGroup.BaseFurniture->SetActorRelativeLocation(vLocation);
	PreviewGroup.BaseFurniture->SetSavedIndex(vIndex);

	PreviewGroup.BaseFurniture->SetArrangedRoomIndex(iRoomIndex);
	for(auto& iter : PreviewGroup.Decorations)
	{
		iter->SetArrangedRoomIndex(iRoomIndex);
	}
}


// 배치 모드
void USDKHousingWidget::TogglePutMode(bool bOn, FString TableID /*= TEXT("")*/)
{
	if(HousingState == EHousingState::ModifyModeState || HousingState == EHousingState::ModifyMovableState)
		return;

	SetHousingState(bOn ? EHousingState::PutModeState : EHousingState::None);

	ToggleFurnitureList(bHiddenListState && !bOn);

	SetLobbyControllerInputMode(bOn);
	SetActiveHousingCamera(bOn);

	SetActiveBackgroundBorder(!bOn);

	if(bOn == true)
	{
		CreatePreviewFurniture(TableID);
		if(PreviewGroup.BaseFurniture == nullptr)
			return;

		PreviewGroup.BaseFurniture->SetCollisionEnabled(true);
		SetPreviewGroupMaterialType(true);

		if(CheckCreatePreviewPosition() == false)
		{
			RemovePreviewFurniture();
			TogglePutMode(false);
			return;
		}
	}
}

void USDKHousingWidget::CompletePutFurniture(bool bComplete)
{
	if(PreviewGroup.BaseFurniture == nullptr)
		return;

	ASDKFurniture* PutFurniture = nullptr;

	if(bComplete == true)
	{
		auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		if(LobbyController != nullptr)
		{
			PutFurniture = LobbyController->PutFurniture(PreviewGroup.BaseFurniture);
			if(PutFurniture != nullptr)
			{
				SetFurnitureCount(PutFurniture->GetFurnitureID(), -1);
			}
		}
	}

	RemovePreviewFurniture();

	TogglePutMode(false);
}


// 수정 모드
bool USDKHousingWidget::ToggleModifyMode(bool bOn, ASDKFurniture* ModifyObject /*= nullptr*/)
{
	if(HousingState == EHousingState::PutModeState || (ModifyGroup.BaseFurniture != nullptr && ModifyObject != nullptr))
		return false;

	SetHousingState(bOn ? EHousingState::ModifyModeState : EHousingState::None);

	SetModifyFurniture(ModifyObject);
	ToggleFurnitureList(bHiddenListState && !bOn);
	SetActiveBackgroundBorder(!bOn);

	if(bOn == true)
	{
		// 기본 미리보기 가구 생성
		if(PreviewGroup.BaseFurniture == nullptr)
		{
			CreateModifyPreviewFurniture(ModifyGroup.BaseFurniture, PreviewGroup.BaseFurniture);
			SetPreviewFurnitureCollision(true);
			SetModifyMeshOffsetRotator(ModifyGroup.BaseFurniture->GetFurnitureSize());
			PreviewGroup.BaseFurniture->SetVisibilityModifyWidget(true);

			// 그 위 데코 가구 생성
			PreviewGroup.Decorations.AddZeroed(ModifyGroup.Decorations.Num());
			for(int32 idx = 0; idx < ModifyGroup.Decorations.Num(); ++idx)
			{
				CreateModifyPreviewFurniture(ModifyGroup.Decorations[idx], PreviewGroup.Decorations[idx]);
				PreviewGroup.Decorations[idx]->SetVisibilityPreviewGroupModifyWidget(false);
				PreviewGroup.Decorations[idx]->SetPreviewGroupTransform(ModifyGroup.Decorations[idx]->GetActorTransform());

				PreviewGroup.Decorations[idx]->AttachToComponent(PreviewGroup.BaseFurniture->GetMeshComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				PreviewGroup.Decorations[idx]->ApplyPreviewGroupingTransform();
			}
		}

		auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		if(LobbyController != nullptr)
		{
			LobbyController->OnSetModifyFurnitureType(ModifyGroup.BaseFurniture->GetFurnitureType());
			LobbyController->SetLocationHousingCamera(ModifyGroup.BaseFurniture->GetActorLocation());
			LobbyController->SetInputGameMode(true);
		}
	}
	else
	{
		ModifyGroup.BaseFurniture->SetFurnitureArrangeEffect();
		ModifyGroup.Clear();
	}

	return true;
}

void USDKHousingWidget::SavedModifyFurniture()
{
	if(PreviewGroup.BaseFurniture == nullptr || HousingState != EHousingState::ModifyModeState)
	{
		return;
	}

	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
	{
		return;
	}

	if(PreviewGroup.Decorations.Num() <= 0)
	{
		// 가구 저장F
		LobbyController->SaveFurniture(ModifyGroup.BaseFurniture);

		// 가구 수 증가 및 리스트에 추가
		SetFurnitureCount(ModifyGroup.BaseFurniture->GetFurnitureID(), 1);
	}

	// 수정 모드 종료
	CompleteModifyFurniture(false);
}

bool USDKHousingWidget::RotatorModifyFurniture()
{
	if(PreviewGroup.BaseFurniture == nullptr || HousingState != EHousingState::ModifyModeState)
		return false;

	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr || ModifyGroup.BaseFurniture == nullptr)
		return false;

	// 베이스 가구 회전
	FRotator tRotator = PreviewGroup.BaseFurniture->GetActorRotation();
	tRotator.Yaw -= 90.f;

	// 회전값 적용
	PreviewGroup.BaseFurniture->SetActorRelativeRotation(tRotator);

	// 가구 크기에 따른 시작인덱스나 사이즈 값 변경
	FVector vSavedIndex = PreviewGroup.BaseFurniture->GetSavedIndex();
	FVector vSize = PreviewGroup.BaseFurniture->GetFurnitureSize();
	if(vSize.X != vSize.Y)
	{
		vSize = FVector(vSize.Y, vSize.X, vSize.Z);

		FVector vMeshOffset = PreviewGroup.BaseFurniture->GetStaticMeshLocation();
		vMeshOffset /= TILE_HALF;
		OffsetIndex = (vMeshOffset.X == 0.f && vMeshOffset.Y == 0.f) ? 0 : OffsetIndex;

		int32 iType = (tRotator.Yaw - 90.f) / -90.f;
		switch((ERotatorType)iType)
		{
		case ERotatorType::Rotator_0:
		{
			vSavedIndex = FVector(vSavedIndex.X, vSavedIndex.Y + vSize.Y, vSavedIndex.Z);
		}
		break;
		case ERotatorType::Rotator_90:
			break;
		case ERotatorType::Rotator_180:
		{
			vSavedIndex.X -= int32(vSize.X - 1.f);
		}
		break;
		case ERotatorType::Rotator_270:
		{
			float fInterval = (vSize.X >= vSize.Y ? vSize.X : vSize.Y) - 1.f;
			vSavedIndex = FVector(vSavedIndex.X + fInterval, vSavedIndex.Y - fInterval, vSavedIndex.Z);
		}
		break;
		default:
			break;
		}

		vMeshOffset.X += RotatorMeshOffset[OffsetIndex].X;
		vMeshOffset.Y += RotatorMeshOffset[OffsetIndex].Y;
		OffsetIndex++;

		PreviewGroup.BaseFurniture->SetFurnitureSize(vSize);
		PreviewGroup.BaseFurniture->SetSavedIndex(vSavedIndex);
		PreviewGroup.BaseFurniture->SetStaticMeshLocation(vMeshOffset);
	}

	bool bResult = LobbyController->CalcPossibleArrangeTile(PreviewGroup.BaseFurniture->GetArrangedRoomIndex(), vSavedIndex, vSize, ModifyGroup.BaseFurniture->GetUniqueID());
	bool bResultType = LobbyController->CalcPossibleArrangeType(PreviewGroup.BaseFurniture->GetArrangedRoomIndex(), vSavedIndex, vSize, PreviewGroup.BaseFurniture->GetFurnitureID());

	SetPreviewGroupMaterialType(bResult && bResultType);

	return bResult && bResultType;
}

void USDKHousingWidget::CompleteMoveModifyFurniture(bool bComplete)
{
	SetHousingState(EHousingState::ModifyModeState);

	if(ModifyGroup.BaseFurniture != nullptr && bComplete == false)
	{
		PreviewGroup.BaseFurniture->SetActorRelativeTransform(ModifyGroup.BaseFurniture->GetActorTransform());
	}

	SetLobbyControllerInputMode(false);
	SetActiveHousingCamera(false);

	SetPreviewFurnitureCollision(true);

	PreviewGroup.BaseFurniture->SetVisibilityModifyWidget(true);
	SetPreviewGroupMaterialType(true);

	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
		return;

	LobbyController->SetLocationHousingCamera(PreviewGroup.BaseFurniture->GetActorLocation());
}

void USDKHousingWidget::CompleteModifyFurniture(bool bComplete)
{
	if(ModifyGroup.BaseFurniture == nullptr || HousingState == EHousingState::ModifyMovableState)
		return;

	if(bComplete == true)
	{
		auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		if(LobbyController != nullptr)
		{
			LobbyController->CompleteModifyFurniture(ModifyGroup.BaseFurniture, PreviewGroup.BaseFurniture, false);

			for(int32 idx = 0; idx < ModifyGroup.Decorations.Num(); ++idx)
			{
				LobbyController->CompleteModifyFurniture(ModifyGroup.Decorations[idx], PreviewGroup.Decorations[idx], true);
			}
		}
	}

	RemovePreviewFurniture();

	ToggleModifyMode(false);
}

void USDKHousingWidget::SaveHousingArrangeData()
{
	CompletePutFurniture(false);

	CompleteModifyFurniture(false);

	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
		return;

	LobbyController->SaveMyroomHousingData();
}

void USDKHousingWidget::ClearHousingParameters()
{
	PreviewGroup = FPreviewGroup();
	ModifyGroup = FFurnitureGroup();

	bListVisible = true;
	bHiddenListState = true;
}

void USDKHousingWidget::ToggleFurnitureList(bool bVisible)
{
	if(bVisible == bListVisible)
		return;

	bListVisible = bVisible;
	OnPlayToggleFurnitureList(bVisible);

	if(MenuButtons.IsValidIndex(0) == false || MenuButtons[0] == nullptr)
		return;

	MenuButtons[0]->SetButtonRotation(bVisible);
}

void USDKHousingWidget::ToggleRotatorHousingCamera(bool bRotate)
{
	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
		return;

	LobbyController->RotateHousingViewCamera(bRotate);
}

void USDKHousingWidget::ToggleResetMessage()
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(SDKHUD == nullptr)
		return;

	USDKMessageBoxWidget* MessageBoxWidget = SDKHUD->MessageBoxOKandCANCEL(TEXT("60010134"), TEXT("60010135"), TEXT(""), TEXT("3"), TEXT("133"));
	if(MessageBoxWidget == nullptr)
		return;

	if(MessageBoxWidget->GetButtonOk() != nullptr)
	{
		if(MessageBoxWidget->GetButtonOk()->OnClicked.Contains(this, TEXT("OnClickedResetButton")) == false)
		{
			MessageBoxWidget->GetButtonOk()->OnClicked.AddDynamic(this, &USDKHousingWidget::OnClickedResetButton);
		}
	}

	if(MessageBoxWidget->GetButtonCancel() != nullptr)
	{
		if(MessageBoxWidget->GetButtonCancel()->OnClicked.Contains(this, TEXT("OnClickedCancelClosed")) == false)
		{
			MessageBoxWidget->GetButtonCancel()->OnClicked.AddDynamic(this, &USDKHousingWidget::OnClickedCancelClosed);
		}
	}
}

void USDKHousingWidget::ToggleCloseMessage()
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(SDKHUD == nullptr)
		return;

	if(g_pMyInfo->GetHousingData().GetIsSavedData() == true)
	{
		SDKHUD->CloseUI(EUI::Lobby_UIHousing);
	}
	else
	{
		USDKMessageBoxWidget* MessageBoxWidget = SDKHUD->MessageBoxOKandCANCEL(TEXT("60010067"), TEXT("60010068"), TEXT(""), TEXT("72"), TEXT("133"));
		if(MessageBoxWidget == nullptr)
			return;

		if(MessageBoxWidget->GetButtonOk() != nullptr)
		{
			if(MessageBoxWidget->GetButtonOk()->OnClicked.Contains(this, TEXT("OnClickedClosedWidget")) == false)
			{
				MessageBoxWidget->GetButtonOk()->OnClicked.AddDynamic(this, &USDKHousingWidget::OnClickedClosedWidget);
			}
		}

		if(MessageBoxWidget->GetButtonCancel() != nullptr)
		{
			if(MessageBoxWidget->GetButtonCancel()->OnClicked.Contains(this, TEXT("OnClickedCancelClosed")) == false)
			{
				MessageBoxWidget->GetButtonCancel()->OnClicked.AddDynamic(this, &USDKHousingWidget::OnClickedCancelClosed);
			}
		}
	}
}

void USDKHousingWidget::OnClickedResetButton()
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(SDKHUD == nullptr)
		return;

	SDKHUD->CloseMessageBox();

	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
		return;

	LobbyController->ClearMyroomAllFurnitureData();
}

void USDKHousingWidget::OnClickedClosedWidget()
{
	auto LobbyController = Cast<ASDKLobbyController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(LobbyController == nullptr)
		return;

	LobbyController->RevertMyroomHousignData();

	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(SDKHUD == nullptr)
		return;

	SDKHUD->CloseUI(EUI::Lobby_UIHousing);
}

void USDKHousingWidget::OnClickedCancelClosed()
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(SDKHUD == nullptr)
		return;

	SDKHUD->CloseMessageBox();
}