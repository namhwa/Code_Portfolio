// Fill out your copyright notice in the Description page of Project Settings.


#include "Housing/SDKHousingCategoryWidget.h"

#include "Components/GridPanel.h"
#include "PaperSprite.h"

#include "Character/SDKHUD.h"
#include "Manager/SDKTableManager.h"
#include "Share/SDKEnum.h"

#include "UI/SDKWidgetParam.h"
#include "UI/SDKLobbyWidget.h"
#include "UI/SDKCheckBoxParam.h"
#include "UI/SDKHousingWidget.h"


void USDKHousingCategoryWidget::CreateUIProcess()
{
	InitCategoryButtons();
}

void USDKHousingCategoryWidget::InitCategoryButtons()
{
	FS_WidgetBluePrint* WidgetTable = USDKTableManager::Get()->FindTableWidgetBluePrint(EWidgetBluePrint::Housing_btn);
	if (WidgetTable)
	{
		UClass* WidgetClass = LoadClass<USDKUserWidget>(nullptr, *WidgetTable->WidgetBluePrintPath.ToString());
		if (IsValid(WidgetClass))
		{
			for (int32 iType = 0; iType < (int32)EFurnitureCategory::Max; ++iType)
			{
				FIntPoint vIndex = { iType / 2, iType % 2 };
				GridCategoryButton.AddGridMenu(GetWorld(), GridPanelCategory, WidgetClass, FString::FromInt(iType), GetCategoryButtonImagePath(iType), TEXT(""), vIndex);
			}

			TMap<FString, USDKCheckBoxParam*> mapMenuCheckBox = GridCategoryButton.GetMapCheckBox();
			for (auto& iter : mapMenuCheckBox)
			{
				if (!IsValid(iter.Value))
				{
					continue;
				}
		
				USDKWidgetParamInt32* WidgetParamID = NewObject<USDKWidgetParamInt32>(this, USDKWidgetParamInt32::StaticClass());
				if (!IsValid(WidgetParamID))
				{
					continue;
				}
		
				WidgetParamID->SetValue(FCString::Atoi(*iter.Key));
		
				iter.Value->SetClickedParam(WidgetParamID);
				iter.Value->OnCheckStateChangedParam.AddDynamic(this, &USDKHousingCategoryWidget::OnClickEventCategoryButton);
			}

			if (mapMenuCheckBox.Num() > 0)
			{
				auto CategoryAll = mapMenuCheckBox.CreateIterator();
				GridCategoryButton.SetCheckBoxCheckedState(CategoryAll->Key, true);
			}
		}
	}
}

FString USDKHousingCategoryWidget::GetCategoryButtonImagePath(int32 iIndex)
{
	FS_GlobalDefine* GlobalTable = USDKTableManager::Get()->FindTableGlobalDefine(EGlobalDefine::HousingCategoryImage);
	if(GlobalTable && GlobalTable->Value.Num() > 0 && GlobalTable->Value.IsValidIndex(iIndex))
	{
		FS_TextureSprite* TextureTable = USDKTableManager::Get()->FindTableTextureSprite(GlobalTable->Value[iIndex]);
		if(TextureTable)
		{
			return TextureTable->TexturePath.ToString();
		}
	}
	
	return TEXT("");
}

void USDKHousingCategoryWidget::OnClickEventCategoryButton(bool bIsChecked, USDKWidgetParam* param)
{
	if(!IsValid(param))
	{
		return;
	}

	USDKWidgetParamInt32* WidgetParamID = Cast<USDKWidgetParamInt32>(param);
	if(IsValid(WidgetParamID))
	{
		FString strValue = FString::FromInt(WidgetParamID->GetValue());
		GridCategoryButton.SetCheckBoxCheckedState(strValue, bIsChecked);
	
		ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
		if(IsValid(SDKHUD))
		{
			USDKUserWidget* MainWidget = SDKHUD->GetUI(EUI::Lobby_UIHousing);
			if(IsValid(MainWidget))
			{
				USDKHousingWidget* HousingWidget = Cast<USDKHousingWidget>(SDKHUD->GetUI(EUI::Lobby_UIHousing));
				if(IsValid(HousingWidget))
				{	
					HousingWidget->SortFurnitureBoxByCategory(static_cast<EFurnitureCategory>(WidgetParamID->GetValue()));
				}
			}
		}
	}
}
