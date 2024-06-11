// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SDKUserWidget.h"
#include "SDKHousingModifyWidget.generated.h"

class UBorder;
class UOverlay;
class UScaleBox;

class ASDKFurniture;
class USDKButtonParam;
class USDKWidgetParam;
class USDKHousingWidget;

UENUM(BlueprintType)
enum class EModifyType : uint8 
{
	None,
	Save,
	Cancel,
	Ok,
	Rotation,
	Max
};

UCLASS()
class ARENA_API USDKHousingModifyWidget : public USDKUserWidget
{
	GENERATED_BODY()
	
public:
	USDKHousingModifyWidget();

	virtual void NativeConstruct() override;

	void InitWidgetData();
	void InitTextSetting();

	/** 버튼 설정 */
	void SetModifyButtonParam(USDKButtonParam* pButtonParam, const EModifyType eType);

	void SetButtonIsEnable(bool bEnable, EModifyType eType);

	void SetWidgetScale(int32 iSize);

	void SetActiveBorderBG(bool bActive);

	void SetVisibilityWidget(bool bVisible);

	UFUNCTION(BlueprintCallable)
	void SetVisibilityOverlayButtons(bool bVisible);

	UFUNCTION(BlueprintCallable)
	bool CheckMovablePickingFurniture();

	UFUNCTION(BlueprintCallable)
	void CompleteMoveModifyFurniture(bool bComplete);

	UFUNCTION()
	void OnClickedParamButton(USDKWidgetParam* param);

	UFUNCTION(BlueprintImplementableEvent)
	void OnPlayToggleAnim(bool bOpen);

	USDKHousingWidget* GetSDKHousingWidget() const;

private:
	UPROPERTY(meta = (BindWidget))
	UBorder* BorderBG;

	UPROPERTY(meta = (BindWidget))
	UOverlay* OverlayButtons;

	UPROPERTY(meta = (BindWidget))
	UScaleBox* ScaleBoxWidget;

	UPROPERTY(meta = (BindWidget))
	USDKButtonParam* SDKButtonSave;

	UPROPERTY(meta = (BindWidget))
	USDKButtonParam* SDKButtonCancel;

	UPROPERTY(meta = (BindWidget))
	USDKButtonParam* SDKButtonOk;

	UPROPERTY(meta = (BindWidget))
	USDKButtonParam* SDKButtonRotation;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextButtonSave;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextButtonCancel;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextButtonOk;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextButtonRotation;

private:
	UPROPERTY()
	bool RotationResult;

	float WidgetScale;
};
