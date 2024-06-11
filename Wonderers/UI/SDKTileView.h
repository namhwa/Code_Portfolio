// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TileView.h"
#include "Components/ListViewBase.h"
#include "UI/SSDKTileView.h"
#include "SDKTileView.generated.h"

class UUserWidget;
class STableViewBase;

/**
 * 
 */
UCLASS()
class ARENA_API USDKTileView : public UTileView
{
	GENERATED_BODY()
	
public:
	USDKTileView(const FObjectInitializer& Initializer);

	bool GetEnableScroll() const { return bEnableScoll; }
	void SetEnableScroll(bool bEnable);

protected:
	virtual TSharedRef<STableViewBase> RebuildListWidget() override;
	virtual UUserWidget& OnGenerateEntryWidgetInternal(UObject* Item, TSubclassOf<UUserWidget> DesiredEntryClass, const TSharedRef<STableViewBase>& OwnerTable) override;

protected:
	UPROPERTY(EditAnywhere, Category = Scroll)
	bool bEnableScoll;

	/** The bar style */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style", meta = (DisplayName = "Bar Style"))
	FScrollBarStyle WidgetBarStyle;

	/** The thickness of the scrollbar thumb */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
	FVector2D ScrollbarThickness;

	/** The margin around the scrollbar */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
	FMargin ScrollbarPadding;
};
