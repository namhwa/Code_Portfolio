// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SDKUserWidget.h"
#include "SDKHousingButtonWidget.generated.h"

class USDKWidgetParam;
UENUM(BlueprintType)
enum class EHousingMenu : uint8
{
	HiddenList,
	Camera,
	Save,
	Max
};

class USizeBox;
class UImage;
class USDKCheckBoxParam;

UCLASS()
class ARENA_API USDKHousingButtonWidget : public USDKUserWidget
{
	GENERATED_BODY()

public:
	USDKHousingButtonWidget();

	void SetHousingMenuIndex(int32 index);

	void SetButtonRotation(bool bRotate);

	void SetButtonSize(FVector2D vSize);
	
	void CreateWidgetParam(int32 value);

	UFUNCTION()
	void OnClickedMenu(bool bIsChecked, USDKWidgetParam* param);

private:
	UPROPERTY(meta = (BindWidget))
	USDKCheckBoxParam* SDKCheckBoxMenu;

	UPROPERTY(meta = (BindWidget))
	USizeBox* SizeBoxButton;

	UPROPERTY(meta = (BindWidget))
	UImage* ImageIcon;

private:
	bool bActive;
};
