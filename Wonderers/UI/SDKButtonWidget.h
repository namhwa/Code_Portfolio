// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SDKShopPopupWidget.h"
#include "UI/SDKUserWidget.h"
#include "SDKButtonWidget.generated.h"

class USDKUserHotKeyWidget;
class UImage;
class UButton;
class UTextBlock;
class UWidgetSwitcher;

class USDKButtonParam;

UCLASS()
class ARENA_API USDKButtonWidget : public USDKUserWidget
{
	GENERATED_BODY()
	
public:
	USDKButtonParam* GetButtonParam();
	UButton* GetButton();

	/** Button Texture */
	void SetButtonOKSign(bool bOKSign);

	/** Button Type */
	void SetButtonVisibleAsset(bool bVisible);

	/** Button Type2 */
	void SetButtonVisibleCount(bool bVisible);

	/** TextBlock */
	void SetButtonText(FText InText);
	void SetButtonTableText(FString InTableID);
	void SetButtonTextColor(EDesignFormat DesignFormat);
	void SetButtonTextShadowColor(FLinearColor Color);

	/** Asset */
	void SetButtonAssetType(EAssetType InAsset, int32 InCount);
	/** Item */
	void SetButtonItemType(FString InItemID, int32 InCount);
	/** IAP */
	void SetButtonIAPPriceTag(const FString& IAPPriceTag);
	/** Count */
	void SetButtonCount(int32 InCount, int32 InMaxCount);
	/** PriceColor */
	void SetButtonPriceColor(const bool bBuyAble);

	/** Enable */
	void SetButtonEnable(bool bEnable);
	void SetActionKey(FName KeyName);
	void SetAssetActionKey(FName KeyName);

	void SetVisibleRedDot(bool bVisible);

	DECLARE_MULTICAST_DELEGATE(FOnSDKButtonWidgetMouseRightButtonDownDelegate)
	FOnSDKButtonWidgetMouseRightButtonDownDelegate OnClickMouseRightButton;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	virtual void NativePreConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SDKButton")
	FButtonStyle ButtonStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SDKButton")
	int32 ActiveWidgetIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SDKButton")
	FText ButtonString;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SDKButton")
	FText ButtonAssetString;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "SDKButton")
	USDKButtonParam* SDKButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "SDKButton")
	UWidgetSwitcher* WidgetSwitcherType;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* ImageAsset;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* TextAsset;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* TextCount;

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "SDKButton")
	UTextBlock* TextButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "SDKButton")
	UTextBlock* TextButtonAsset;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "SDKButton")
	USDKUserHotKeyWidget* HotKey;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "SDKButton")
	USDKUserHotKeyWidget* AssetHotKey;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "SDKButton")
	UImage* ImageRedDot;
};
