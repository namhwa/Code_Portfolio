// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SDKTileView.h"

#include "Styling/SlateTypes.h"
#include "Widgets/Views/STableViewBase.h"

#include "UI/SDKUserWidget.h"
#include "UI/SSDKObjectTableRow.h"


static FScrollBarStyle* DefaultSDKTileViewScrollBarStyle = nullptr;

USDKTileView::USDKTileView(const FObjectInitializer& Initializer)
	: Super(Initializer)
	, ScrollbarThickness(9.0f, 9.0f)
	, ScrollbarPadding(2.0f)
{
	if (DefaultSDKTileViewScrollBarStyle == nullptr)
	{
		// HACK: THIS SHOULD NOT COME FROM CORESTYLE AND SHOULD INSTEAD BE DEFINED BY ENGINE TEXTURES/PROJECT SETTINGS
		DefaultSDKTileViewScrollBarStyle = new FScrollBarStyle(FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("ScrollBar"));

		// Unlink UMG default colors from the editor settings colors.
		DefaultSDKTileViewScrollBarStyle->UnlinkColors();
	}

	WidgetBarStyle = *DefaultSDKTileViewScrollBarStyle;
}

void USDKTileView::SetEnableScroll(bool bEnable)
{
	bEnableScoll = bEnable;
	SetScrollbarVisibility(bEnable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	auto SDKTileView = ConstructTileView<SSDKTileView>();
 	SDKTileView.Get().SetEnableScroll(bEnable);
	SDKTileView.Get().SetScrollBarStyle(&WidgetBarStyle);
	SDKTileView.Get().SetScrollbarThickness(ScrollbarThickness);
	SDKTileView.Get().SetScrollbarPadding(ScrollbarPadding);
}

TSharedRef<STableViewBase> USDKTileView::RebuildListWidget()
{
	auto SDKTileView = ConstructTileView<SSDKTileView>();
	SDKTileView.Get().SetEnableScroll(bEnableScoll);
	SDKTileView.Get().SetScrollBarStyle(&WidgetBarStyle);
	SDKTileView.Get().SetScrollbarThickness(ScrollbarThickness);
	SDKTileView.Get().SetScrollbarPadding(ScrollbarPadding);

	return SDKTileView;
}

UUserWidget& USDKTileView::OnGenerateEntryWidgetInternal(UObject* Item, TSubclassOf<UUserWidget> DesiredEntryClass, const TSharedRef<STableViewBase>& OwnerTable)
{
	if (DesiredEntryClass->IsChildOf<USDKUserWidget>())
	{
		return GenerateTypedEntry<UUserWidget, SSDKObjectTableRow<UObject*>>(DesiredEntryClass, OwnerTable);
	}

	return GenerateTypedEntry(DesiredEntryClass, OwnerTable);
}
