// Fill out your copyright notice in the Description page of Project Settings.


#include "Housing/SDKHousingModifyWidget.h"

#include "Character/SDKHUD.h"
#include "Share/SDKHelper.h"
#include "Object/SDKFurniture.h"
#include "UI/SDKButtonParam.h"
#include "UI/SDKWidgetParam.h"
#include "UI/SDKHousingWidget.h"
#include "Character/SDKLobbyController.h"
#include "Object/SDKPreviewFurniture.h"

#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/ScaleBox.h"


USDKHousingModifyWidget::USDKHousingModifyWidget()
	: WidgetScale(1.f)
{

}

void USDKHousingModifyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RotationResult = true;

	InitWidgetData();
	InitTextSetting();
}

void USDKHousingModifyWidget::InitWidgetData()
{
	// Save
	SetModifyButtonParam(SDKButtonSave, EModifyType::Save);

	// Cancel
	SetModifyButtonParam(SDKButtonCancel, EModifyType::Cancel);

	// Ok
	SetModifyButtonParam(SDKButtonOk, EModifyType::Ok);

	// Rotaion
	SetModifyButtonParam(SDKButtonRotation, EModifyType::Rotation);
}

void USDKHousingModifyWidget::InitTextSetting()
{
	SetTextBlockTableString(TextButtonSave, TEXT("231"));
	SetTextBlockTableString(TextButtonCancel, TEXT("133"));
	SetTextBlockTableString(TextButtonOk, TEXT("3"));
	SetTextBlockTableString(TextButtonRotation, TEXT("382"));
}

void USDKHousingModifyWidget::SetModifyButtonParam(USDKButtonParam* pButtonParam, EModifyType eType)
{
	USDKWidgetParam* pWidgetParam = pButtonParam->GetClickedParam();
	if(pWidgetParam == nullptr)
	{
		USDKWidgetParamInt32* pNewParamInt = NewObject<USDKWidgetParamInt32>(this, USDKWidgetParamInt32::StaticClass());
		if(pNewParamInt == nullptr)
			return;

		pNewParamInt->SetValue((int32)eType);
		pButtonParam->SetClickedParam(pNewParamInt);
		pButtonParam->OnClickedParam.AddDynamic(this, &USDKHousingModifyWidget::OnClickedParamButton);
	}
	else
	{
		USDKWidgetParamInt32* pParamInt = Cast<USDKWidgetParamInt32>(pWidgetParam);
		if(pParamInt == nullptr)
			return;

		pParamInt->SetValue((int32)eType);
	}
}

void USDKHousingModifyWidget::SetButtonIsEnable(bool bEnable, EModifyType eType)
{
	switch(eType)
	{
	case EModifyType::Save:
		if(SDKButtonSave == nullptr)
			return;

		SDKButtonSave->SetIsEnabled(bEnable);
		return;

	case EModifyType::Cancel:
		if(SDKButtonCancel == nullptr)
			return;

		SDKButtonCancel->SetIsEnabled(bEnable);
		return;

	case EModifyType::Ok:
		if(SDKButtonOk == nullptr)
			return;

		SDKButtonOk->SetIsEnabled(bEnable);
		return;

	case EModifyType::Rotation:
		if(SDKButtonRotation == nullptr)
			return;

		SDKButtonRotation->SetIsEnabled(bEnable);
		return;

	default:
		return;
	}
}

void USDKHousingModifyWidget::SetWidgetScale(int32 iSize)
{
	if(ScaleBoxWidget == nullptr)
		return;

	ScaleBoxWidget->SetUserSpecifiedScale(WidgetScale + (0.4f * iSize));
}

void USDKHousingModifyWidget::SetActiveBorderBG(bool bActive)
{
	if(BorderBG == nullptr)
		return;

	if(bActive)
	{
		BorderBG->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		BorderBG->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void USDKHousingModifyWidget::SetVisibilityWidget(bool bVisible)
{
	if(bVisible)
	{
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USDKHousingModifyWidget::SetVisibilityOverlayButtons(bool bVisible)
{
	if(OverlayButtons == nullptr)
		return;

	if(bVisible)
	{
		OverlayButtons->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		OverlayButtons->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool USDKHousingModifyWidget::CheckMovablePickingFurniture()
{
	return false;
}

void USDKHousingModifyWidget::CompleteMoveModifyFurniture(bool bComplete)
{
	auto pHousingWidget = GetSDKHousingWidget();
	if(pHousingWidget == nullptr)
		return;

	pHousingWidget->CompleteMoveModifyFurniture(bComplete);
}

void USDKHousingModifyWidget::OnClickedParamButton(USDKWidgetParam* param)
{
	if(param == nullptr)
		return;

	USDKWidgetParamInt32* pParamInt = Cast<USDKWidgetParamInt32>(param);
	if(pParamInt == nullptr)
		return;

	auto pHousingWidget = GetSDKHousingWidget();
	if(pHousingWidget == nullptr)
		return;

	switch((EModifyType)pParamInt->GetValue())
	{
	case EModifyType::Save:
		pHousingWidget->SavedModifyFurniture();
		break;

	case EModifyType::Cancel:
		pHousingWidget->CompleteModifyFurniture(false);
		break;

	case EModifyType::Ok:
		pHousingWidget->CompleteModifyFurniture(RotationResult);
		break;

	case EModifyType::Rotation:
		RotationResult = pHousingWidget->RotatorModifyFurniture();
		break;

	default:
		return;
	}
}

USDKHousingWidget* USDKHousingModifyWidget::GetSDKHousingWidget() const
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(SDKHUD == nullptr)
		return nullptr;

	if(SDKHUD->GetUI(EUI::Lobby_UIHousing) == nullptr)
		return nullptr;

	return Cast<USDKHousingWidget>(SDKHUD->GetUI(EUI::Lobby_UIHousing));
}
