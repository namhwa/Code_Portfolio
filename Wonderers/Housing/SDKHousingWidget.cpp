// Fill out your copyright notice in the Description page of Project Settings.

#include "Housing/SDKHousingWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Kismet/GameplayStatics.h"

#include "Character/SDKHUD.h"
#include "Character/SDKMyInfo.h"
#include "Character/SDKLobbyController.h"
#include "Character/SDKPlayerCharacter.h"

#include "Engine/SDKBlueprintFunctionLibrary.h"
#include "Manager/SDKTableManager.h"
#include "Object/SDKFurniture.h"
#include "Object/SDKPreviewFurniture.h"
#include "Share/SDKData.h"
#include "Share/SDKHelper.h"

#include "UI/SDKLobbyWidget.h"
#include "UI/SDKMessageBoxWidget.h"
#include "UI/SDKFurnitureBoxWidget.h"
#include "UI/SDKHousingButtonWidget.h"
#include "UI/SDKHousingCategoryWidget.h"
#include "UI/SDKCheckBoxParam.h"
#include "UI/SDKWidgetParam.h"

DEFINE_LOG_CATEGORY(LogHousing)

USDKHousingWidget::USDKHousingWidget()
{
	ClearHousingParameters();
}

void USDKHousingWidget::CreateUIProcess()
{
	Super::CreateUIProcess();

	InitHousingWidget();
}

FReply USDKHousingWidget::NativeOnTouchGesture(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if(InGestureEvent.GetGestureType() == EGestureEvent::Swipe)
	{
		ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
		if(IsValid(LobbyController) && IsValid(BorderBackground))
		{
			if(BorderBackground->GetVisibility() == ESlateVisibility::Visible)
			{
				LobbyController->MoveHousingViewCamera(InGestureEvent.GetGestureDelta());
			}
		}
	}

	return Super::NativeOnTouchGesture(InGeometry, InGestureEvent);
}

void USDKHousingWidget::OpenUIProcess()
{
	ClearHousingParameters();

	SetToggleProcess(true);

	Super::OpenUIProcess();
}

void USDKHousingWidget::CloseUIProcess()
{
	SetToggleProcess(false);

	Super::CloseUIProcess();
}


void USDKHousingWidget::InitHousingWidget()
{
	InitButtonSetting();

	SetFurnitureList();

	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(IsValid(SDKHUD))
	{
		USDKUserWidget* MainWidget = SDKHUD->GetUI(EUI::Lobby_Main);
		if(IsValid(MainWidget))
		{
			USDKLobbyWidget* LobbyWidget = Cast<USDKLobbyWidget>(MainWidget);
			if(IsValid(LobbyWidget))
			{
				LobbyWidget->SetSDKHousingWidget(this);	
			}
		}
	}
}

void USDKHousingWidget::InitButtonSetting()
{
	if(IsValid(ButtonReset))
	{
		if(!ButtonReset->OnClicked.Contains(this, TEXT("ToggleResetMessage")))
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

	ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
	if(IsValid(LobbyController))
	{
		LobbyController->SetEnableClickEvent(bOpen);
		LobbyController->InitHousingCamera(bOpen);
		LobbyController->SetUseInputTouch(bOpen);
		LobbyController->OnSetVisibilityVirtualJoystick(!bOpen);
	
		ASDKPlayerCharacter* PlayerPawn = Cast<ASDKPlayerCharacter>(LobbyController->GetPawn());
		if(IsValid(PlayerPawn))
		{
			PlayerPawn->SetActorHiddenInGame(bOpen);
			PlayerPawn->SetActorEnableCollision(!bOpen);
		
			if(bOpen)
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
	}
}

void USDKHousingWidget::SetCloseUIHandleBinding(bool bBind)
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(IsValid(SDKHUD))
	{
		if(bBind)
		{
			if(!CloseUIHandle.IsValid())
			{
				CloseUIHandle = SDKHUD->CloseSubUIEvent.AddUObject(this, &USDKHousingWidget::ToggleCloseMessage);
			}		
		}
		else
		{
			SDKHUD->CloseMessageBox();
	
			if(CloseUIHandle.IsValid())
			{
				SDKHUD->CloseSubUIEvent.Remove(CloseUIHandle);
				CloseUIHandle.Reset();
			}
		}
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
	if(mapFurnitures.Contains(FurnitureID))
	{
		USDKFurnitureBoxWidget* FurnitureBoxWidget = mapFurnitures[FurnitureID];
		if(IsValid(FurnitureBoxWidget))
		{
			int32 OwnedCount = FurnitureBoxWidget->GetOwnedCount() + NewCount;
			FurnitureBoxWidget->SetOwnedCount(OwnedCount);
		}
	}
}

void USDKHousingWidget::SetLobbyControllerInputMode(bool bGameOnly)
{
	ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
	if(IsValid(LobbyController))
	{
		LobbyController->SetInputGameMode(bGameOnly);
	}
}

void USDKHousingWidget::SetActiveHousingCamera(bool bActive)
{
	ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
	if(IsValid(LobbyController))
	{
		if(bActive)
		{
			LobbyController->OnStartFurnitureCamera();
	
			if(IsValid(PreviewGroup.BaseFurniture))
			{
				PreviewGroup.BaseFurniture->SetCameraActive();
			}
			
		}
		else
		{
			LobbyController->OnEndFurnitureCamera();
		}
	}
}

void USDKHousingWidget::SetHousingState(EHousingState state)
{
	HousingState = state;
}

void USDKHousingWidget::SetActiveBackgroundBorder(bool bActive)
{
	if(IsValid(BorderBackground))
	{
		if(bActive)
		{
			BorderBackground->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			BorderBackground->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
}

void USDKHousingWidget::SetHiddenListState(bool bHidden)
{
	bHiddenListState = bHidden;

	ToggleFurnitureList(bHiddenListState);
}

void USDKHousingWidget::SetPreviewFurnitureCollision(bool bEnable)
{
	if(IsValid(PreviewGroup.BaseFurniture) && HousingState != EHousingState::PutModeState)
	{
		PreviewGroup.BaseFurniture->SetCollisionEnabled(bEnable);
	}
}

void USDKHousingWidget::SetPreviewGroupMaterialType(bool bEnable)
{
	if(IsValid(PreviewGroup.BaseFurniture))
	{
		PreviewGroup.BaseFurniture->SetFurnitureMaterialType(bEnable);
		for(auto& iter : PreviewGroup.Decorations)
		{
			if(IsValid(iter))
			{
				iter->SetFurnitureMaterialType(bEnable);
			}
		}
	}
}

void USDKHousingWidget::SetModifyFurniture(ASDKFurniture* NewFurniture)
{
	ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
	if(IsValid(LobbyController))
	{	
		if(IsValid(NewFurniture))
		{
			ModifyGroup.BaseFurniture = NewFurniture;
			if(NewFurniture->GetFurnitureType() != EFurnitureType::Decoration)
			{
				// 가구 위에 장식 있는지 확인 : 있는 경우 같이 처리
				LobbyController->CheckDecorationOnFurniture(NewFurniture->GetArrangedRoomIndex(), NewFurniture->GetUniqueID(), NewFurniture->GetSavedIndex(), NewFurniture->GetFurnitureSize(), ModifyGroup.Decorations);
			}
		}

		if(IsValid(ModifyGroup.BaseFurniture))
		{
			SetVisibilityModifyFurniture(HousingState == EHousingState::ModifyModeState);
		}
	}
}

void USDKHousingWidget::SetModifyMeshOffsetRotator(FVector vSize)
{
	if(vSize.X == vSize.Y)
	{
		return;
	}

	FVector vInit = FVector(vSize.Y, vSize.X, vSize.Z) - vSize;
	FVector2D vInterval = FVector2D(FMath::Abs(vInit.X), FMath::Abs(vInit.Y));

	RotatorMeshOffset.Empty(4);
	for(int32 idx = 0; idx < 4; ++idx)
	{
		FVector2D vOffset = vInterval;
		if(idx < 2)
		{
			vOffset.X *= -1.f;
		}
		
		if(idx == 1 || idx == 2)
		{
			vOffset.Y *= -1.f;	
		}

		RotatorMeshOffset.Add(vOffset);
	}
}

void USDKHousingWidget::SetVisibilityModifyFurniture(bool bVisible)
{
	if(IsValid(ModifyGroup.BaseFurniture))
	{
		ModifyGroup.BaseFurniture->SetActorHiddenInGame(bVisible);
		ModifyGroup.BaseFurniture->SetActorEnableCollision(!bVisible);

		if(ModifyGroup.Decorations.Num() > 0)
		{
			for(auto& iter : ModifyGroup.Decorations)
			{
				if(IsValid(iter))
				{
					iter->SetActorHiddenInGame(bVisible);
					iter->SetActorEnableCollision(!bVisible);
				}
			}
		}
	}
}


// 정렬
void USDKHousingWidget::SortFurnitureBoxByCategory(EFurnitureCategory eCategory)
{
	for(auto& iter : mapFurnitures)
	{
		if(IsValid(iter.Value))
		{
			if(eCategory == EFurnitureCategory::None)
			{
				iter.Value->SetWidgetVisibility(true);
				continue;
			}
	
			iter.Value->SetWidgetVisibility(eCategory == iter.Value->GetFurnitureCategoryType());
		}
	}
}


// 검사 관련 함수
bool USDKHousingWidget::CheckMovablePickFurniture(ETouchIndex::Type type)
{
	if(HousingState != EHousingState::ModifyModeState && HousingState != EHousingState::PutModeState)
	{
		return false;
	}

	ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
	if(IsValid(LobbyController))
	{

		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = { OBJECT_OBJECT };
		FHitResult HitResult;

#if PLATFORM_WINDOWS
		if(!LobbyController->GetHitResultUnderCursorForObjects(ObjectTypes, true, HitResult))
#else 
		if(!LobbyController->GetHitResultUnderFingerForObjects(type, ObjectTypes, true, HitResult))
#endif
		{
			UE_LOG(LogHousing, Log, TEXT("No Have Hit Result under the cursor"));
			return false;
		}
	
		ASDKFurniture* HitTarget = Cast<ASDKFurniture>(HitResult.Actor);
		if(IsValid(HitTarget))
		{
			// 드래그 OR 수정 모드
			if(HousingState == EHousingState::ModifyModeState)
			{
				if(IsValid(ModifyGroup.BaseFurniture) && IsValid(PreviewGroup.BaseFurniture) && PreviewGroup.BaseFurniture == HitTarget)
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
	}
	
	return false;
}

void USDKHousingWidget::CheckClickEventMyroomFurnitur(ASDKFurniture* ClickedObject)
{
	if(!IsValid(ClickedObject) || HousingState == EHousingState::PutModeState)
	{
		return;
	}

	if(!ClickedObject->GetIsPreviewActor() && !IsValid(ModifyGroup.BaseFurniture) && HousingState == EHousingState::None)
	{
		ToggleModifyMode(true, ClickedObject);
	}
}

bool USDKHousingWidget::CheckDecorationPutableType(ECollisionChannel eCollisionType, AActor* HitActor)
{
	if(IsValid(HitActor))
	{
		ASDKFurniture* OvelapFurniture = Cast<ASDKFurniture>(HitActor);
		if(IsValid(OvelapFurniture) && IsValid(PreviewGroup.BaseFurniture))
		{
			EFurnitureType ePrevType = PreviewGroup.BaseFurniture->GetFurnitureType();
			EFurnitureType eOverlapType = OvelapFurniture->GetFurnitureType();
	
			if(eCollisionType != COLLISION_OBJECT)
			{
				return false;
			}
	
			return (PreviewGroup.BaseFurniture == OvelapFurniture) || (ePrevType == EFurnitureType::Decoration && eOverlapType == EFurnitureType::Furniture);
		}
	}
	return false;
}

bool USDKHousingWidget::CheckGeneralPreviewPutable(FHitResult tHitResult)
{
	ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
	if(IsValid(LobbyController))
	{
		ASDKFurniture* OverlapObject = Cast<ASDKFurniture>(tHitResult.GetActor());
		if(IsValid(OverlapObject) && IsValid(PreviewGroup.BaseFurniture))
		{
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
			{
				return false;
			}
		
			bResultType = LobbyController->CalcPossibleArrangeType(iRoomIndex, vIndex, vSize, PreviewGroup.BaseFurniture->GetFurnitureID());
			
			return bResultPos && bResultType;
		}
	}

	return false;
}

bool USDKHousingWidget::CheckDecorationPreviewPutable(FHitResult tHitResult)
{
	ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
	if(IsValid(LobbyController))
	{
		ASDKFurniture* OverlapObject = Cast<ASDKFurniture>(tHitResult.GetActor());
		if(IsValid(OverlapObject))
		{
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
	}

	return false;
}

bool USDKHousingWidget::CheckCreatePreviewPosition()
{
	ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
	if(IsValid(LobbyController))
	{
		if(IsValid(LobbyController->GetHousingViewCamera()))
		{
			if(IsValid(PreviewGroup.BaseFurniture))
			{
				EFurnitureType PreviewType = PreviewGroup.BaseFurniture->GetFurnitureType();
				if(PreviewType == EFurnitureType::Wall || PreviewType == EFurnitureType::Floor)
				{
					return true;
				}
			
				FVector CameraLoc = LobbyController->GetHousingViewCamera()->GetActorLocation();
				FVector CameraDir = LobbyController->GetHousingViewCamera()->GetCameraComponent()->GetForwardVector();
			
				FHitResult HitResult;
				TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = { OBJECT_OBJECT, OBJECT_WALL, OBJECT_FLOOR };
				if(!GetWorld()->LineTraceSingleByObjectType(HitResult, CameraLoc, CameraLoc + CameraDir * 100000.f, FCollisionObjectQueryParams(ObjectTypes)))
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
		}
	}

	return false;
}

void USDKHousingWidget::CalcWallFurnitureRotator(AActor* HitActor)
{
	if(!IsValid(HitActor) || !IsValid(PreviewGroup.BaseFurniture))
	{
		return;
	}

	ASDKFurniture* HitFurniture = Cast<ASDKFurniture>(HitActor);
	if(!IsValid(HitFurniture) || HitFurniture->GetFurnitureType() != EFurnitureType::Wall)
	{
		return;
	}

	FS_MyroomBasic* MyroomBasicTable = USDKTableManager::Get()->FindTableMyroomBasic(HitFurniture->GetBaseTableID());
	if(MyroomBasicTable == nullptr)
	{
		return;
	}

	FS_MyroomParts* FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(PreviewGroup.BaseFurniture->GetFurnitureID());
	if(FurnitureTable == nullptr)
	{
		return;
	}

	FVector vResult = FVector::ZeroVector;
	FVector vSize = FurnitureTable->Size;
	float fRotatorValue = 135.f;
	
	if(!MyroomBasicTable->Direction)
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
	FS_WidgetBluPrintTable* WidgetTable = USDKTableManager::Get()->FindTableWidgetBluePrint(EWidgetBluePrint::Housing_btn);
	if (WidgetTable != nullptr)
	{
		UClass* WidgetClass = LoadClass<USDKUserWidget>(nullptr, *WidgetTable->WidgetBluePrintPath.ToString());
		if (IsValid(WidgetClass))
		{
			USDKHousingButtonWidget* ButtonWidget = CreateWidget<USDKHousingButtonWidget>(GetOwningPlayer(), WidgetClass);
			if(IsValid(ButtonWidget))
			{
				ButtonWidget->SetHousingMenuIndex(index);
				ButtonWidget->SetButtonSize(FVector2D(60.f, 60.f));
			
				if(IsValid(HorizontalBoxMenu))
				{
					HorizontalBoxMenu->AddChildToHorizontalBox(ButtonWidget);
				}
			
				MenuButtons.Emplace(ButtonWidget);
			}
		}
	}
}

void USDKHousingWidget::CreateFurnitrueBoxWidget(FString TableID)
{
	if(TableID.IsEmpty())
	{
		return;
	}
	
	FS_WidgetBluPrintTable* WidgetTable = USDKTableManager::Get()->FindTableWidgetBluePrint(EWidgetBluePrint::Housing_Box);
	if(WidgetTable != nullptr)
	{
		UClass* WidgetClass = LoadClass<USDKUserWidget>(nullptr, *WidgetTable->WidgetBluePrintPath.ToString());
		if (IsValid(WidgetClass))
		{
			USDKFurnitureBoxWidget* FurnitureWidget = CreateWidget<USDKFurnitureBoxWidget>(GetOwningPlayer(), WidgetClass);
			if (IsValid(FurnitureWidget))
			{		
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
	if(TableID.IsEmpty())
	{
		return;
	}

	if(!IsValid(PreviewGroup.BaseFurniture))
	{
		FS_MyroomParts* FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(TableID);
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

			ASDKPreviewFurniture* PreviewActor = GetWorld()->SpawnActor<ASDKPreviewFurniture>(ASDKPreviewFurniture::StaticClass(), vLoaction, vRotator, SpawnInfo);
			if(IsValid(PreviewActor))
			{
				PreviewActor->SetFurnitureID(TableID);
				PreviewActor->SetVisibilityModifyWidget(false);

				PreviewGroup.BaseFurniture = PreviewActor;

				SetPreviewGroupMaterialType(true);
				SetPreviewFurnitureCollision(true);
			}
		}
	}
}

void USDKHousingWidget::CreateModifyPreviewFurniture(ASDKFurniture* pBaseFurniture, ASDKPreviewFurniture*& pPreview)
{
	if(IsValid(ModifyGroup.BaseFurniture))
	{
		FRotator vRotator = FRotator::ZeroRotator;
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
		pPreview = GetWorld()->SpawnActor<ASDKPreviewFurniture>(ASDKPreviewFurniture::StaticClass(), FVector::ZeroVector, vRotator, SpawnInfo);
		if(IsValid(pPreview))
		{
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
	}
}

void USDKHousingWidget::RemovePreviewFurniture()
{
	if(IsValid(PreviewGroup.BaseFurniture))
	{
		PreviewGroup.Remove();
	}
}

bool USDKHousingWidget::StartMovablePreviewFurniture(ETouchIndex::Type type)
{
	if(!CheckMovablePickFurniture(type))
	{
		return false;
	}

	SetActiveBackgroundBorder(false);
	SetPreviewGroupMaterialType(true);

	SetLobbyControllerInputMode(true);
	SetActiveHousingCamera(true);

	if(!IsValid(PreviewGroup.BaseFurniture))
	{
		return false;
	}
	
	PreviewGroup.BaseFurniture->SetVisibilityModifyWidget(false);
	return true;
}

void USDKHousingWidget::UpdatePreviewFurniture(ETouchIndex::Type type)
{
	if(!IsValid(PreviewGroup.BaseFurniture))
	{
		return;
	}

	if(HousingState == EHousingState::ModifyModeState || HousingState == EHousingState::PutModeState)
	{
		return;
	}

	ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
	if(!IsValid(LobbyControlle))
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
	if(!LobbyController->GetHitResultUnderCursorForObjects(ObjectTypes, true, HitResult))
#else 
	if(!LobbyController->GetHitResultUnderFingerForObjects(type, ObjectTypes, true, HitResult))
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
	if(IsValid(PreviewGroup.BaseFurniture))
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

	return false;
}

bool USDKHousingWidget::UpdatePreviewWallAndFloor()
{
	if(IsValid(PreviewGroup.BaseFurniture))
	{
		bool bResult = false;
	
		ASDKFurniture* pHitActor = Cast<ASDKFurniture>(PreviewHitResult.Actor);
		if(IsValid(pHitActor))
		{
			bResult = (PreviewGroup.BaseFurniture->GetFurnitureType() == EFurnitureType::Wall && pHitActor->GetFurnitureType() == EFurnitureType::Wall)
				|| (PreviewGroup.BaseFurniture->GetFurnitureType() == EFurnitureType::Floor && pHitActor->GetFurnitureType() == EFurnitureType::Floor);
		}
	
		if(bResult)
		{
			FRotator tRotator = pHitActor->GetActorRotation();
			FVector vLocation = pHitActor->GetActorLocation();
	
			FS_MyroomBasic* MyroomBasicTable = USDKTableManager::Get()->FindTableMyroomBasic(pHitActor->GetBaseTableID());
			if(MyroomBasicTable != nullptr)
			{
				if(MyroomBasicTable->Type)
				{
					vLocation.X -= 20.f;
					vLocation.Y += 20.f;
					vLocation.Z += 200.f;
	
					tRotator = FRotator::ZeroRotator;
				}
				else
				{
					tRotator.Yaw = 0.f;
					if(!MyroomBasicTable->Direction)
					{
						vLocation.Y += 10.f;
					}
					else
					{
						vLocation.X -= 20.f;
						vLocation.Y += 100.f;
					}
				}
				
				UStaticMesh* FurnitureMesh = LoadObject<UStaticMesh>(nullptr, *MyroomBasicTable->MeshPath.ToString());
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
			else
			{
				PreviewGroup.BaseFurniture->SetStaticMesh(nullptr);
			}
	
			PreviewGroup.BaseFurniture->SetActorRelativeLocation(vLocation);
			PreviewGroup.BaseFurniture->SetActorRelativeRotation(tRotator);
		}
	
		return bResult;
	}

	return false;
}

void USDKHousingWidget::UpdatePreviewFurnitureData(int32 iRoomIndex, FVector vLocation, FVector vIndex)
{
	if(IsValid(PreviewGroup.BaseFurniture))
	{
		PreviewGroup.BaseFurniture->SetActorRelativeLocation(vLocation);
		PreviewGroup.BaseFurniture->SetSavedIndex(vIndex);
		PreviewGroup.BaseFurniture->SetArrangedRoomIndex(iRoomIndex);
	
		if(PreviewGroup.Decorations.Num() > 0)
		{
			for(auto& iter : PreviewGroup.Decorations)
			{
				if(IsValid(iter))
				{
					iter->SetArrangedRoomIndex(iRoomIndex);
				}
			}
		}
	}
}

// 배치 모드
void USDKHousingWidget::TogglePutMode(bool bOn, FString TableID /*= TEXT("")*/)
{
	if(HousingState == EHousingState::ModifyModeState || HousingState == EHousingState::ModifyMovableState)
	{
		return;
	}

	SetHousingState(bOn ? EHousingState::PutModeState : EHousingState::None);

	ToggleFurnitureList(bHiddenListState && !bOn);

	SetLobbyControllerInputMode(bOn);
	SetActiveHousingCamera(bOn);

	SetActiveBackgroundBorder(!bOn);

	if(bOn)
	{
		CreatePreviewFurniture(TableID);
		if(IsValid(PreviewGroup.BaseFurniture))
		{
			PreviewGroup.BaseFurniture->SetCollisionEnabled(true);
			SetPreviewGroupMaterialType(true);
	
			if(!CheckCreatePreviewPosition())
			{
				RemovePreviewFurniture();
				TogglePutMode(false);
				return;
			}
		}
	}
}

void USDKHousingWidget::CompletePutFurniture(bool bComplete)
{
	if(IsValid(PreviewGroup.BaseFurniture))
	{
		ASDKFurniture* PutFurniture = nullptr;
	
		if(bComplete)
		{
			ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
			if(IsValid(LobbyController))
			{
				PutFurniture = LobbyController->PutFurniture(PreviewGroup.BaseFurniture);
				if(IsValid(PutFurniture))
				{
					SetFurnitureCount(PutFurniture->GetFurnitureID(), -1);
				}
			}
		}
	
		RemovePreviewFurniture();
	
		TogglePutMode(false);
	}
}


// 수정 모드
bool USDKHousingWidget::ToggleModifyMode(bool bOn, ASDKFurniture* ModifyObject /*= nullptr*/)
{
	if(HousingState == EHousingState::PutModeState)
	{
		return false;
	}
	
	if(!IsValid(ModifyGroup.BaseFurniture) && IsValid(ModifyObject))
	{
		return false;
	}

	SetHousingState(bOn ? EHousingState::ModifyModeState : EHousingState::None);

	SetModifyFurniture(ModifyObject);
	ToggleFurnitureList(bHiddenListState && !bOn);
	SetActiveBackgroundBorder(!bOn);

	if(bOn)
	{
		// 기본 미리보기 가구 생성
		if(!IsValid(PreviewGroup.BaseFurniture))
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
				if(IsValid(PreviewGroup.Decorations[idx]))
				{
					PreviewGroup.Decorations[idx]->SetVisibilityPreviewGroupModifyWidget(false);
					PreviewGroup.Decorations[idx]->SetPreviewGroupTransform(ModifyGroup.Decorations[idx]->GetActorTransform());
	
					PreviewGroup.Decorations[idx]->AttachToComponent(PreviewGroup.BaseFurniture->GetMeshComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
					PreviewGroup.Decorations[idx]->ApplyPreviewGroupingTransform();
				}
			}
		}

		ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
		if(IsValid(LobbyController))
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
	if(!IsValid(PreviewGroup.BaseFurniture) || HousingState != EHousingState::ModifyModeState)
	{
		return;
	}

	ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
	if(IsValid(LobbyController))
	{
		if(PreviewGroup.Decorations.Num() <= 0)
		{
			// 가구 저장
			LobbyController->SaveFurniture(ModifyGroup.BaseFurniture);

			if(IsValid(ModifyGroup.BaseFurniture))
			{
				// 가구 수 증가 및 리스트에 추가
				SetFurnitureCount(ModifyGroup.BaseFurniture->GetFurnitureID(), 1);
			}
		}
	
		// 수정 모드 종료
		CompleteModifyFurniture(false);
	}
}

bool USDKHousingWidget::RotatorModifyFurniture()
{
	if(!IsValid(PreviewGroup.BaseFurniture) || HousingState != EHousingState::ModifyModeState)
	{
		return false;
	}

	ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
	if(!IsValid(LobbyController) || !IsValid(ModifyGroup.BaseFurniture))
	{
		return false;
	}

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
		switch(static_cast<ERotatorType>(iType))
		{
		case ERotatorType::Rotator_0:
		{
			vSavedIndex = FVector(vSavedIndex.X, vSavedIndex.Y + vSize.Y, vSavedIndex.Z);
		}
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

	if(IsValid(PreviewGroup.BaseFurniture))
	{
		if(IsValid(ModifyGroup.BaseFurniture) && !bComplete)
		{
			PreviewGroup.BaseFurniture->SetActorRelativeTransform(ModifyGroup.BaseFurniture->GetActorTransform());
		}
	
		SetLobbyControllerInputMode(false);
		SetActiveHousingCamera(false);
	
		SetPreviewFurnitureCollision(true);
	
		PreviewGroup.BaseFurniture->SetVisibilityModifyWidget(true);
		SetPreviewGroupMaterialType(true);
	
		ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
		if(IsValid(LobbyController))
		{
			LobbyController->SetLocationHousingCamera(PreviewGroup.BaseFurniture->GetActorLocation());
		}
	}
}

void USDKHousingWidget::CompleteModifyFurniture(bool bComplete)
{
	if(!IsValid(ModifyGroup.BaseFurniture) || HousingState == EHousingState::ModifyMovableState)
	{
		return;
	}

	if(bComplete)
	{
		ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
		if(IsValid(LobbyController))
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

	ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
	if(IsValid(LobbyController))
	{
		LobbyController->SaveMyroomHousingData();
	}
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
	{
		return;
	}

	bListVisible = bVisible;
	OnPlayToggleFurnitureList(bVisible);

	if(MenuButtons.IsValidIndex(0) && IsValid(MenuButtons[0]))
	{
		MenuButtons[0]->SetButtonRotation(bVisible);
	}
}

void USDKHousingWidget::ToggleRotatorHousingCamera(bool bRotate)
{
	ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
	if(IsValid(LobbyController))
	{
		LobbyController->RotateHousingViewCamera(bRotate);
	}
}

void USDKHousingWidget::ToggleResetMessage()
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(IsValid(SDKHUD))
	{
		USDKMessageBoxWidget* MessageBoxWidget = SDKHUD->MessageBoxOKandCANCEL(TEXT("60010134"), TEXT("60010135"), TEXT(""), TEXT("3"), TEXT("133"));
		if(IsValid(MessageBoxWidget))
		{
			if(IsValid(MessageBoxWidget->GetButtonOk()))
			{
				if(!MessageBoxWidget->GetButtonOk()->OnClicked.Contains(this, TEXT("OnClickedResetButton")))
				{
					MessageBoxWidget->GetButtonOk()->OnClicked.AddDynamic(this, &USDKHousingWidget::OnClickedResetButton);
				}
			}
		
			if(IsValid(MessageBoxWidget->GetButtonCancel())
			{
				if(!MessageBoxWidget->GetButtonCancel()->OnClicked.Contains(this, TEXT("OnClickedCancelClosed")))
				{
					MessageBoxWidget->GetButtonCancel()->OnClicked.AddDynamic(this, &USDKHousingWidget::OnClickedCancelClosed);
				}
			}
		}
	}
}

void USDKHousingWidget::ToggleCloseMessage()
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(IsValid(SDKHUD))
	{
		if(g_pMyInfo->GetHousingData().GetIsSavedData())
		{
			SDKHUD->CloseUI(EUI::Lobby_UIHousing);
		}
		else
		{
			USDKMessageBoxWidget* MessageBoxWidget = SDKHUD->MessageBoxOKandCANCEL(TEXT("60010067"), TEXT("60010068"), TEXT(""), TEXT("72"), TEXT("133"));
			if(IsValid(MessageBoxWidget))
			{
				if(IsValid(MessageBoxWidget->GetButtonOk()))
				{
					if(!MessageBoxWidget->GetButtonOk()->OnClicked.Contains(this, TEXT("OnClickedClosedWidget")))
					{
						MessageBoxWidget->GetButtonOk()->OnClicked.AddDynamic(this, &USDKHousingWidget::OnClickedClosedWidget);
					}
				}
		
				if(IsValid(MessageBoxWidget->GetButtonCancel())
				{
					if(!MessageBoxWidget->GetButtonCancel()->OnClicked.Contains(this, TEXT("OnClickedCancelClosed")))
					{
						MessageBoxWidget->GetButtonCancel()->OnClicked.AddDynamic(this, &USDKHousingWidget::OnClickedCancelClosed);
					}
				}
			}
		}
	}
}

void USDKHousingWidget::OnClickedResetButton()
{
	ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
	if(IsValid(LobbyController))
	{
		ASDKHUD* SDKHUD = Cast<ASDKHUD>(LobbyController->GetHUD());
		if(IsValid(SDKHUD))
		{
			SDKHUD->CloseMessageBox();
		}
	
		LobbyController->ClearMyroomAllFurnitureData();
	}
}

void USDKHousingWidget::OnClickedClosedWidget()
{
	ASDKLobbyController* LobbyController = Cast<ASDKLobbyController>(GetOwningPlayer());
	if(IsValid(LobbyController))
	{
		LobbyController->RevertMyroomHousignData();
	}

	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(IsValid(SDKHUD))
	{
		SDKHUD->CloseUI(EUI::Lobby_UIHousing);
	}
}

void USDKHousingWidget::OnClickedCancelClosed()
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(IsValid(SDKHUD))
	{
		SDKHUD->CloseMessageBox();
	}
}
