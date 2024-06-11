// Fill out your copyright notice in the Description page of Project Settings.


#include "Housing/SDKFurnitureBoxWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "PaperSprite.h"

#include "Character/SDKHUD.h"
#include "Character/SDKLobbyController.h"
#include "Manager/SDKTableManager.h"
#include "Share/SDKHelper.h"

#include "UI/SDKButtonParam.h"
#include "UI/SDKWidgetParam.h"
#include "UI/SDKHousingWidget.h"

DEFINE_LOG_CATEGORY(LogFurnitureBox)


void USDKFurnitureBoxWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void USDKFurnitureBoxWidget::InitFurnitureInfo()
{
	if(TableID.IsEmpty() == true)
	{
		return;
	}

	auto FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(TableID);
	auto ItemTable = USDKTableManager::Get()->FindTableItem(TableID);
	if(FurnitureTable == nullptr || ItemTable == nullptr)
	{
		UE_LOG(LogFurnitureBox, Error, TEXT("Not Found Myroom Parts Table : %s"), *TableID);
		return;
	}

	if (ItemTable->IconPath.GetUniqueID().IsAsset() == true)
	{
		SetImageTexturePath(ImageFurnitureIcon, ItemTable->IconPath.ToString());
	}

	MaxCount = FurnitureTable->MaxCount;
	FurnitureType = FurnitureTable->Type;
	CategoryType = FurnitureTable->Category;

	SetTextBlockString(TextFurnitureType, FSDKHelpers::GetTableString(GetWorld(), FurnitureTable->TypeName));
	SetTextBlockString(TextFurnitureName, FSDKHelpers::GetTableString(GetWorld(), ItemTable->Name));
	SetButtonFurnitureParam();
}

void USDKFurnitureBoxWidget::SetTableID(FString NewTableID)
{
	if(TableID.IsEmpty() == true || TableID != NewTableID)
	{
		TableID = NewTableID;
		
		InitFurnitureInfo();
	}
}

void USDKFurnitureBoxWidget::SetOwnedCount(int32 NewCount)
{
	OwnedCount = NewCount;

	if(OwnedCount >= 0)
	{
		if(OwnedCount >= MaxCount)
		{
			OwnedCount = MaxCount;
		}

		SetTextBlockString(TextOwnedCount, FString::Printf(TEXT("%d / %d"), OwnedCount, MaxCount));
	}

	SetIsEnabled(OwnedCount > 0);
}

void USDKFurnitureBoxWidget::SetWidgetVisibility(bool bVisible)
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

void USDKFurnitureBoxWidget::SetButtonFurnitureParam()
{
	if(SDKButtonFurniture == nullptr)
		return;

	USDKWidgetParam* pParam = SDKButtonFurniture->GetClickedParam();
	if(pParam == nullptr)
	{
		USDKWidgetParamString* pNewParam = NewObject<USDKWidgetParamString>(this, USDKWidgetParamString::StaticClass());
		if(pNewParam == nullptr)
			return;

		pNewParam->SetValue(TableID);

		SDKButtonFurniture->SetClickedParam(pNewParam);
		SDKButtonFurniture->OnClickedParam.AddDynamic(this, &USDKFurnitureBoxWidget::OnClickedFurniture);
	}
	else
	{
		USDKWidgetParamString* pFurnitureParam = Cast<USDKWidgetParamString>(pParam);
		if(pFurnitureParam == nullptr)
			return;

		pFurnitureParam->SetValue(TableID);
	}
}

void USDKFurnitureBoxWidget::OnClickedFurniture(USDKWidgetParam* param)
{
	if(param == nullptr)
		return;

	USDKWidgetParamString* pFurnitureParam = Cast<USDKWidgetParamString>(param);
	if(pFurnitureParam == nullptr)
		return;

	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetOwningPlayer()->GetHUD());
	if(SDKHUD == nullptr)
		return;

	if(SDKHUD->GetUI(EUI::Lobby_UIHousing) == nullptr)
		return;

	auto pHousingWidget = Cast<USDKHousingWidget>(SDKHUD->GetUI(EUI::Lobby_UIHousing));
	if(pHousingWidget == nullptr)
		return;

	pHousingWidget->TogglePutMode(true, pFurnitureParam->GetValue());
}
