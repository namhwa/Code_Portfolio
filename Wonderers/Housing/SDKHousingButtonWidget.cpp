// Fill out your copyright notice in the Description page of Project Settings.


#include "Housing/SDKHousingButtonWidget.h"

#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "PaperSprite.h"

#include "Character/SDKHUD.h"
#include "Share/SDKEnum.h"
#include "Manager/SDKTableManager.h"
#include "UI/SDKCheckBoxParam.h"
#include "UI/SDKParam.h"
#include "UI/SDKHousingWidget.h"


USDKHousingButtonWidget::USDKHousingButtonWidget()
: bActive(true)
{
	
}

void USDKHousingButtonWidget::SetHousingMenuIndex(int32 index)
{
	FS_GlobalDefine* GlobalTable = USDKTableManager::Get()->FindTableGlobalDefine(EGlobalDefine::HousingMenuImage);
	if(GlobalTable == nullptr || GlobalTable->Value.Num() <= 0)
	{
		return;
	}

	FS_TextureSprite* TextureTable = USDKTableManager::Get()->FindTableTextureSprite(GlobalTable->Value[index]);
	if(TextureTable != nullptr)
	{
		SetImageTexturePath(ImageIcon, TextureTable->TexturePath.ToString());
	}

	USDKWidgetParam* Param = SDKCheckBoxMenu->GetClickedParam();
	if(!IsValid(Param))
	{
		CreateWidgetParam(index);
	}
	else
	{
		USDKWidgetParamInt32* IntParam = Cast<USDKWidgetParamInt32>(pParam);
		if(IsValid(IntParam))
		{
			IntParam->SetValue(index);
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
	if(IsValid(SizeBoxButton))
	{
		SizeBoxButton->SetWidthOverride(vSize.X);
		SizeBoxButton->SetHeightOverride(vSize.Y);
	}
}

void USDKHousingButtonWidget::CreateWidgetParam(int32 value)
{
	USDKWidgetParamInt32* NewParam = NewObject<USDKWidgetParamInt32>(this, USDKWidgetParamInt32::StaticClass());
	if(IsValid(NewParam))
	{
		NewParam->SetValue(value);
		SDKCheckBoxMenu->SetClickedParam(NewParam);
		SDKCheckBoxMenu->OnCheckStateChangedParam.AddDynamic(this, &USDKHousingButtonWidget::OnClickedMenu);
	}
}

void USDKHousingButtonWidget::OnClickedMenu(bool bIsChecked, USDKWidgetParam* param)
{
	if(!IsValid(param))
	{
		return;
	}

	USDKWidgetParamInt32* IntParam = Cast<USDKWidgetParamInt32>(param);
	if(IsValid(IntParam))
	{
		ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
		if(IsValid(SDKHUD))
		{
			USDKUserWidget* MainWidget = SDKHUD->GetUI(EUI::Lobby_UIHousing);
			if(IsValid(MainWidget))
			{
				USDKHousingWidget* HousingWidget = Cast<USDKHousingWidget>(MainWidget);
				if(IsValid(HousingWidget))
				{
					bActive = !bActive;
				
					EHousingMenu menu = static_cast<EHousingMenu>(IntParam->GetValue());
					switch(menu)
					{
					case EHousingMenu::HiddenList:		HousingWidget->SetHiddenListState(bActive);		break;
					case EHousingMenu::Camera:		HousingWidget->ToggleRotatorHousingCamera(!bActive);	break;
					case EHousingMenu::Save:		HousingWidget->SaveHousingArrangeData();		break;
					default:
						break;
					}
				}
			}
		}
	}
}
