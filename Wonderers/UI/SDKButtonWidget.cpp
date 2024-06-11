// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SDKButtonWidget.h"

#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"

#include "Manager/SDKTableManager.h"
#include "Share/SDKDataTable.h"

#include "UI/SDKButtonParam.h"
#include "UI/SDKUserHotKeyWidget.h"


USDKButtonParam* USDKButtonWidget::GetButtonParam()
{
	return SDKButton;
}

UButton* USDKButtonWidget::GetButton()
{
	return SDKButton;
}

void USDKButtonWidget::SetButtonOKSign(bool bOKSign)
{
	if (bOKSign)
	{
		SetButtonStyle(SDKButton, TEXT("MainBtn_Type1"), TEXT("MainBtn_Type3"), TEXT("MainBtn_Type2"));
	}
	else
	{
		SetButtonStyle(SDKButton, TEXT("MainBtn_Type4"), TEXT("MainBtn_Type6"), TEXT("MainBtn_Type5"));
	}
}

void USDKButtonWidget::SetButtonVisibleAsset(bool bVisible)
{
	if (IsValid(WidgetSwitcherType))
	{
		if (bVisible)
		{
			WidgetSwitcherType->SetActiveWidgetIndex(1);
		}
		else
		{
			WidgetSwitcherType->SetActiveWidgetIndex(0);
		}
	}
}

void USDKButtonWidget::SetButtonVisibleCount(bool bVisible)
{
	if (IsValid(WidgetSwitcherType))
	{
		if (bVisible)
		{
			WidgetSwitcherType->SetActiveWidgetIndex(2);
		}
		else
		{
			WidgetSwitcherType->SetActiveWidgetIndex(0);
		}
	}
}

void USDKButtonWidget::SetButtonText(FText InText)
{
	SetTextBlockText(TextButton, InText);
	SetTextBlockText(TextButtonAsset, InText);
}

void USDKButtonWidget::SetButtonTableText(FString InTableID)
{
	SetTextBlockTableText(TextButton, InTableID);
	SetTextBlockTableText(TextButtonAsset, InTableID);
}

void USDKButtonWidget::SetButtonTextColor(EDesignFormat DesignFormat)
{
	SetTextBlockFont(TextButton, DesignFormat);
	SetTextBlockFont(TextButtonAsset, DesignFormat);
}

void USDKButtonWidget::SetButtonTextShadowColor(FLinearColor Color)
{
	SetTextBlockShadowColor(TextButton, Color);
	SetTextBlockShadowColor(TextButtonAsset, Color);
}

void USDKButtonWidget::SetButtonAssetType(EAssetType InAsset, int32 InCount)
{
	if (InAsset != EAssetType::None)
	{
		FS_Asset* AssetTable = USDKTableManager::Get()->FindTableAsset(InAsset);
		if (AssetTable != nullptr)
		{
			SetImageTexturePath(ImageAsset, AssetTable->AssetIcon.ToString());
		}

		SetTextBlockText(TextAsset, NumberToText(InCount));
		SetButtonVisibleAsset(true);
	}
	else
	{
		SetButtonVisibleAsset(false);
	}
}

void USDKButtonWidget::SetButtonItemType(FString InItemID, int32 InCount)
{
	FS_Item* ItemTable = USDKTableManager::Get()->FindTableRow<FS_Item>(ETableType::tb_Item, InItemID);
	if (ItemTable != nullptr)
	{
		SetImageTexturePath(ImageAsset, ItemTable->IconPath.ToString());
	}
	
	SetTextBlockText(TextAsset, NumberToText(InCount));
	SetButtonVisibleAsset(true);
}

void USDKButtonWidget::SetButtonIAPPriceTag(const FString& IAPPriceTag)
{
	SetWidgetVisibility(ImageAsset, ESlateVisibility::Collapsed);	
	SetTextBlockText(TextAsset, FText::FromString(IAPPriceTag));
	SetButtonVisibleAsset(true);
}

void USDKButtonWidget::SetButtonCount(int32 InCount, int32 InMaxCount)
{
	SetTextBlockText(TextCount, CountToText(InCount, InMaxCount));
	SetButtonVisibleCount(true);
}

void USDKButtonWidget::SetButtonPriceColor(const bool bBuyAble)
{
	SetTextBlockFont(TextAsset, bBuyAble ? EDesignFormat::Contents_Active : EDesignFormat::Color_False);
}

void USDKButtonWidget::SetButtonEnable(bool bEnable)
{
	if (IsValid(SDKButton))
	{
		SDKButton->SetIsEnabled(bEnable);
	}
}

void USDKButtonWidget::SetActionKey(FName KeyName)
{
	HotKey->SetVisibility(ESlateVisibility::HitTestInvisible);
	HotKey->InitMappingAction(KeyName);
}

void USDKButtonWidget::SetAssetActionKey(FName KeyName)
{
	AssetHotKey->SetVisibility(ESlateVisibility::HitTestInvisible);
	AssetHotKey->InitMappingAction(KeyName);
}

void USDKButtonWidget::SetVisibleRedDot(bool bVisible)
{
	if (IsValid(ImageRedDot))
	{
		if (bVisible)
		{
			ImageRedDot->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			ImageRedDot->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

FReply USDKButtonWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
		return FReply::Handled();
	}
	
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void USDKButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (!IsValid(ButtonStyle.Normal.GetResourceObject()) && IsValid(SDKButton))
	{
		ButtonStyle = SDKButton->WidgetStyle;
	}

	if (IsValid(TextButton) && !TextButton->GetText().IsEmpty() && ButtonString.IsEmpty())
	{
		ButtonString = TextButton->GetText();
	}

	if (IsValid(TextButtonAsset) && !TextButtonAsset->GetText().IsEmpty() && ButtonAssetString.IsEmpty())
	{
		ButtonAssetString = TextButtonAsset->GetText();
	}
}

TSharedRef<SWidget> USDKButtonWidget::RebuildWidget()
{
	auto Widget = Super::RebuildWidget();

	if (IsValid(SDKButton) && IsValid(ButtonStyle.Normal.GetResourceObject()))
	{
		SDKButton->SetStyle(ButtonStyle);
	}

	SetWidgetSwitcherActiveWidgetIndex(WidgetSwitcherType, ActiveWidgetIndex);

	if (!ButtonString.IsEmpty())
	{
		SetTextBlockText(TextButton, ButtonString);
	}

	if (!ButtonAssetString.IsEmpty())
	{
		SetTextBlockText(TextButtonAsset, ButtonAssetString);
	}

	return Widget;
}

#if WITH_EDITOR
void USDKButtonWidget::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	if (MemberPropertyName == FName("ButtonStyle"))
	{
		if (IsValid(WidgetSwitcherType))
		{
			SDKButton->SetStyle(ButtonStyle);
		}
	}

	if (MemberPropertyName == FName("ActiveWidgetIndex"))
	{
		SetWidgetSwitcherActiveWidgetIndex(WidgetSwitcherType, ActiveWidgetIndex);
	}

	if (MemberPropertyName == FName("ButtonString"))
	{
		SetTextBlockText(TextButton, ButtonString);
	}

	if (MemberPropertyName == FName("ButtonAssetString"))
	{
		SetTextBlockText(TextButtonAsset, ButtonAssetString);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
