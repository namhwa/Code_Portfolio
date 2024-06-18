// Fill out your copyright notice in the Description page of Project Settings.


#include "Housing/SDKHousingModifyWidget.h"

#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"

#include "Character/SDKHUD.h"
#include "Object/SDKFurniture.h"
#include "Object/SDKPreviewFurniture.h"

#include "UI/SDKButtonParam.h"
#include "UI/SDKWidgetParam.h"
#include "UI/SDKHousingWidget.h"


USDKHousingModifyWidget::USDKHousingModifyWidget()
: WidgetScale(1.f)
{

}

void USDKHousingModifyWidget::CreateUIPorcess()
{
	Super::CreateUIProcess();

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
	if(!IsValid(pWidgetParam))
	{
		USDKWidgetParamInt32* pNewParamInt = NewObject<USDKWidgetParamInt32>(this, USDKWidgetParamInt32::StaticClass());
		if(IsValid(pNewParamInt))
		{
			pNewParamInt->SetValue((int32)eType);
			pButtonParam->SetClickedParam(pNewParamInt);
			pButtonParam->OnClickedParam.AddDynamic(this, &USDKHousingModifyWidget::OnClickedParamButton);
		}
	}
	else
	{
		USDKWidgetParamInt32* pParamInt = Cast<USDKWidgetParamInt32>(pWidgetParam);
		if(IsValid(pParamInt))
		{
			pParamInt->SetValue(static_cast<int32>(eType));
		}
	}
}

void USDKHousingModifyWidget::SetButtonIsEnable(bool bEnable, EModifyType eType)
{
	switch(eType)
	{
	case EModifyType::Save:
		if(IsValid(SDKButtonSave))
		{
			SDKButtonSave->SetIsEnabled(bEnable);
		}
		break;

	case EModifyType::Cancel:
		if(IsValid(SDKButtonCancel))
		{
			SDKButtonCancel->SetIsEnabled(bEnable);
		}
		break;

	case EModifyType::Ok:
		if(IsValid(SDKButtonOk))
		{
			SDKButtonOk->SetIsEnabled(bEnable);
		}
		break;

	case EModifyType::Rotation:
		if(IsValid(SDKButtonRotation))
		{
			SDKButtonRotation->SetIsEnabled(bEnable);
		}
		break;

	default:
		break;
	}
}

void USDKHousingModifyWidget::SetWidgetScale(int32 iSize)
{
	if(IsValid(ScaleBoxWidget))
	{
		ScaleBoxWidget->SetUserSpecifiedScale(WidgetScale + (0.4f * iSize));	
	}
}

void USDKHousingModifyWidget::SetActiveBorderBG(bool bActive)
{
	if(IsValid(BorderBG))
	{
		if(bActive)
		{
			SetWidgetVisibility(BorderBG, ESlateVisibility::Visible);
		}
		else
		{
			SetWidgetVisibility(BorderBG, ESlateVisibility::SelfHitTestInvisible);
		}
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
	if(IsValid(OverlayButtons))
	{
		if(bVisible)
		{
			SetWidgetVisibility(OverlayButtons, ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			SetWidgetVisibility(OverlayButtons, ESlateVisibility::Collapsed);
		}
	}
}

bool USDKHousingModifyWidget::CheckMovablePickingFurniture()
{
	return false;
}

void USDKHousingModifyWidget::CompleteMoveModifyFurniture(bool bComplete)
{
	USDKHousingWidget* HousingWidget = GetSDKHousingWidget();
	if(IsValid(HousingWidget))
	{
		HousingWidget->CompleteMoveModifyFurniture(bComplete);
	}
}

void USDKHousingModifyWidget::OnClickedParamButton(USDKWidgetParam* param)
{
	if(!IsValid(param))
	{
		return;
	}

	USDKWidgetParamInt32* ParamInt = Cast<USDKWidgetParamInt32>(param);
	if(IsValid(ParamInt))
	{
		USDKHousingWidget* HousingWidget = GetSDKHousingWidget();
		if(IsValid(HousingWidget))
		{
			switch(static_cast<EModifyType>(ParamInt->GetValue()))
			{
			case EModifyType::Save:
				HousingWidget->SavedModifyFurniture();
				break;
		
			case EModifyType::Cancel:
				HousingWidget->CompleteModifyFurniture(false);
				break;
		
			case EModifyType::Ok:
				HousingWidget->CompleteModifyFurniture(RotationResult);
				break;
		
			case EModifyType::Rotation:
				RotationResult = HousingWidget->RotatorModifyFurniture();
				break;
		
			default:
				break;
			}
		}
	}
}

USDKHousingWidget* USDKHousingModifyWidget::GetSDKHousingWidget() const
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(IsValid(SDKHUD))
	{
		USDKUserWidget* MainWidget = SDKHUD->GetUI(EUI::Lobby_UIHousing);
		if(IsValid(MainWidget))
		{
			return Cast<USDKHousingWidget>(MainWidget);
		}
	}

	return nullptr;
}
