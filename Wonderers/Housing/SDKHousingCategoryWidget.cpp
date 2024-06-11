// Fill out your copyright notice in the Description page of Project Settings.


#include "Housing/SDKHousingCategoryWidget.h"

#include "Character/SDKHUD.h"
#include "Manager/SDKTableManager.h"
#include "Share/SDKEnum.h"
#include "Share/SDKHelper.h"

#include "UI/SDKWidgetParam.h"
#include "UI/SDKLobbyWidget.h"
#include "UI/SDKCheckBoxParam.h"
#include "UI/SDKHousingWidget.h"

#include "PaperSprite.h"


void USDKHousingCategoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitCategoryButtons();
}

void USDKHousingCategoryWidget::InitCategoryButtons()
{
	auto WidgetTable = USDKTableManager::Get()->FindTableWidgetBluePrint(EWidgetBluePrint::Housing_btn);
	if (WidgetTable == nullptr)
		return;

	if (WidgetTable->WidgetBluePrintPath.GetUniqueID().IsAsset() == false)
		return;

	auto pWidget = LoadClass<USDKUserWidget>(nullptr, *WidgetTable->WidgetBluePrintPath.ToString());
	if (pWidget == nullptr)
		return;

	for (int32 iType = 0; iType < (int32)EFurnitureCategory::Max; ++iType)
	{
		FIntPoint vIndex = { iType / 2, iType % 2 };
		GridCategoryButton.AddGridMenu(GetWorld(), GridPanelCategory, pWidget, FString::FromInt(iType), GetCategoryButtonImagePath(iType), TEXT(""), vIndex);
	}

	TMap<FString, USDKCheckBoxParam*> mapMenuCheckBox = GridCategoryButton.GetMapCheckBox();
	for (auto& iter : mapMenuCheckBox)
	{
		if (iter.Value == nullptr)
		{
			continue;
		}

		USDKWidgetParamInt32* WidgetParamID = NewObject<USDKWidgetParamInt32>(this, USDKWidgetParamInt32::StaticClass());
		if (WidgetParamID == nullptr)
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

FString USDKHousingCategoryWidget::GetCategoryButtonImagePath(int32 iIndex)
{
	auto GlobalTable = USDKTableManager::Get()->FindTableGlobalDefine(EGlobalDefine::HousingCategoryImage);
	if(GlobalTable == nullptr || GlobalTable->Value.Num() <= 0)
		return TEXT("");

	if(GlobalTable->Value.IsValidIndex(iIndex) == false)
		return TEXT("");

	auto TextureTable = USDKTableManager::Get()->FindTableTextureSprite(GlobalTable->Value[iIndex]);
	if(TextureTable == nullptr || TextureTable->TexturePath.GetUniqueID().IsAsset() == false)
		return TEXT("");

	return TextureTable->TexturePath.ToString();
}

void USDKHousingCategoryWidget::OnClickEventCategoryButton(bool bIsChecked, USDKWidgetParam* param)
{
	if(param == nullptr)
		return;

	USDKWidgetParamInt32* WidgetParamID = Cast<USDKWidgetParamInt32>(param);
	if(WidgetParamID == nullptr)
		return;

	FString strValue = FString::FromInt(WidgetParamID->GetValue());
	GridCategoryButton.SetCheckBoxCheckedState(strValue, bIsChecked);

	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(SDKHUD == nullptr || SDKHUD->GetUI(EUI::Lobby_UIHousing) == nullptr)
		return;

	USDKHousingWidget* HousingWidget = Cast<USDKHousingWidget>(SDKHUD->GetUI(EUI::Lobby_UIHousing));
	if(HousingWidget == nullptr)
		return;

	HousingWidget->SortFurnitureBoxByCategory((EFurnitureCategory)WidgetParamID->GetValue());
}