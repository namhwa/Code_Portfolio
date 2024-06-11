// Fill out your copyright notice in the Description page of Project Settings.


#include "Housing/SDKHousingButtonWidget.h"

#include "Character/SDKHUD.h"
#include "Share/SDKEnum.h"
#include "Share/SDKHelper.h"
#include "Manager/SDKTableManager.h"
#include "UI/SDKCheckBoxParam.h"
#include "UI/SDKHousingWidget.h"

#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "PaperSprite.h"

USDKHousingButtonWidget::USDKHousingButtonWidget()
{
	bActive = true;
}

void USDKHousingButtonWidget::SetHousingMenuIndex(int32 index)
{
	auto GlobalTable = USDKTableManager::Get()->FindTableGlobalDefine(EGlobalDefine::HousingMenuImage);
	if(GlobalTable == nullptr || GlobalTable->Value.Num() <= 0)
		return;

	auto TextureTable = USDKTableManager::Get()->FindTableTextureSprite(GlobalTable->Value[index]);
	if(TextureTable == nullptr || TextureTable->TexturePath.GetUniqueID().IsAsset() == false)
		return;

	SetImageTexturePath(ImageIcon, TextureTable->TexturePath.ToString());

	USDKWidgetParam* pParam = SDKCheckBoxMenu->GetClickedParam();
	if(pParam == nullptr)
	{
		CreateWidgetParam(index);
	}
	else
	{
		USDKWidgetParamInt32* pIntParam = Cast<USDKWidgetParamInt32>(pParam);
		if(pIntParam != nullptr)
		{
			pIntParam->SetValue(index);
		}
		else
		{
			CreateWidgetParam(index);
		}
	}
}

void USDKHousingButtonWidget::SetButtonRotation(bool bRotate)
{
	SetRenderTransformAngle(bRotate ? 0.f : 180.f);
}

void USDKHousingButtonWidget::SetButtonSize(FVector2D vSize)
{
	if(SizeBoxButton == nullptr)
		return;

	SizeBoxButton->SetWidthOverride(vSize.X);
	SizeBoxButton->SetHeightOverride(vSize.Y);
}

void USDKHousingButtonWidget::CreateWidgetParam(int32 value)
{
	USDKWidgetParamInt32* pNewParam = NewObject<USDKWidgetParamInt32>(this, USDKWidgetParamInt32::StaticClass());
	if(pNewParam == nullptr)
		return;

	pNewParam->SetValue(value);
	SDKCheckBoxMenu->SetClickedParam(pNewParam);
	SDKCheckBoxMenu->OnCheckStateChangedParam.AddDynamic(this, &USDKHousingButtonWidget::OnClickedMenu);
}

void USDKHousingButtonWidget::OnClickedMenu(bool bIsChecked, USDKWidgetParam* param)
{
	if(param == nullptr)
		return;

	USDKWidgetParamInt32* pIntParam = Cast<USDKWidgetParamInt32>(param);
	if(pIntParam == nullptr)
		return;

	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(SDKHUD == nullptr || SDKHUD->GetUI(EUI::Lobby_UIHousing) == nullptr)
		return;

	USDKHousingWidget* pHousingWidget = Cast<USDKHousingWidget>(SDKHUD->GetUI(EUI::Lobby_UIHousing));
	if(pHousingWidget == nullptr)
		return;

	bActive = !bActive;

	EHousingMenu menu = (EHousingMenu)pIntParam->GetValue();
	switch(menu)
	{
	case EHousingMenu::HiddenList:	pHousingWidget->SetHiddenListState(bActive);			break;
	case EHousingMenu::Camera:		pHousingWidget->ToggleRotatorHousingCamera(!bActive);	break;
	case EHousingMenu::Save:		pHousingWidget->SaveHousingArrangeData();				break;
	default:
		break;
	}
}
