// Fill out your copyright notice in the Description page of Project Settings.


#include "Housing/SDKFurnitureBoxWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "PaperSprite.h"

#include "Character/SDKHUD.h"
#include "Manager/SDKTableManager.h"
#include "Share/SDKHelper.h"

#include "UI/SDKButtonParam.h"
#include "UI/SDKWidgetParam.h"
#include "UI/SDKHousingWidget.h"

DEFINE_LOG_CATEGORY(LogFurnitureBox)


void USDKFurnitureBoxWidget::InitFurnitureInfo()
{
	if(TableID.IsEmpty())
	{
		return;
	}

	FS_MyroomParts* FurnitureTable = USDKTableManager::Get()->FindTableMyroomParts(TableID);
	FS_Item* ItemTable = USDKTableManager::Get()->FindTableItem(TableID);
	if(FurnitureTable == nullptr || ItemTable == nullptr)
	{
		UE_LOG(LogFurnitureBox, Error, TEXT("Not Found Myroom Parts Table : %s"), *TableID);
		return;
	}

	MaxCount = FurnitureTable->MaxCount;
	FurnitureType = FurnitureTable->Type;
	CategoryType = FurnitureTable->Category;

	SetImageTexturePath(ImageFurnitureIcon, ItemTable->IconPath.ToString());
	SetTextBlockString(TextFurnitureType, FSDKHelpers::GetTableString(GetWorld(), FurnitureTable->TypeName));
	SetTextBlockString(TextFurnitureName, FSDKHelpers::GetTableString(GetWorld(), ItemTable->Name));
	SetButtonFurnitureParam();
}

void USDKFurnitureBoxWidget::SetTableID(FString NewTableID)
{
	if(TableID.IsEmpty() || TableID != NewTableID)
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

void USDKFurnitureBoxWidget::SetButtonFurnitureParam()
{
	if(!IsValid(SDKButtonFurniture))
	{
		return;
	}

	USDKWidgetParam* Param = SDKButtonFurniture->GetClickedParam();
	if(!IsValid(Param))
	{
		USDKWidgetParamString* NewParam = NewObject<USDKWidgetParamString>(this, USDKWidgetParamString::StaticClass());
		if(IsValid(NewParam))
		{
			NewParam->SetValue(TableID);
	
			SDKButtonFurniture->SetClickedParam(NewParam);
			SDKButtonFurniture->OnClickedParam.AddDynamic(this, &USDKFurnitureBoxWidget::OnClickedFurniture);
		}
	}
	else
	{
		USDKWidgetParamString* FurnitureParam = Cast<USDKWidgetParamString>(pParam);
		if(IsValid(FurnitureParam))
		{
			FurnitureParam->SetValue(TableID);
		}
	}
}

void USDKFurnitureBoxWidget::OnClickedFurniture(USDKWidgetParam* param)
{
	if(!IsValid(param))
	{
		return;
	}

	USDKWidgetParamString* FurnitureParam = Cast<USDKWidgetParamString>(param);
	if(IsValid(FurnitureParam))
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
					HousingWidget->TogglePutMode(true, FurnitureParam->GetValue());
				}
			}
		}
	}
}
