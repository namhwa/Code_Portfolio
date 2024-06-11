// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SSDKButton.h"
#include "Components/Button.h"
#include "UI/SDKWidgetParam.h"
#include "SDKButtonParam.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnButtonParamEvent, USDKWidgetParam*, param);

UCLASS()
class ARENA_API USDKButtonParam : public UButton
{
	GENERATED_UCLASS_BODY()
	
public:
	UPROPERTY(BlueprintAssignable, Category = "Button|Event")
	FOnButtonParamEvent OnClickedParam;
	
	UPROPERTY(BlueprintAssignable, Category = "Button|Event")
	FOnButtonParamEvent OnPressedParam;

	UPROPERTY(BlueprintAssignable, Category = "Button|Event")
	FOnButtonParamEvent OnReleasedParam;

	UPROPERTY(BlueprintAssignable, Category = "Button|Event")
	FOnButtonParamEvent OnHoveredParam;
	
	UPROPERTY(BlueprintAssignable, Category = "Button|Event")
	FOnButtonParamEvent OnUnhoveredParam;
		
	UPROPERTY(BlueprintAssignable, Category = "Button|Event")
	FOnButtonParamEvent OnPressedTimeCheckedParam;
	
	UPROPERTY(BlueprintAssignable, Category = "Button|Event")
	FOnButtonParamEvent OnClickedMouseRightButtonDown;

	UFUNCTION(BlueprintCallable)
	void SetClickedParam(USDKWidgetParam* WidgetParam) { ClickedWidgetParam = WidgetParam; }
	UFUNCTION(BlueprintCallable)
	USDKWidgetParam* GetClickedParam() { return ClickedWidgetParam; }
	
	void SetPressedTimeChecker(uint8 GlobalDefineKey);
	UFUNCTION(BlueprintCallable)
	void SetPressedTimeChecker(float fPressTime);
	void SetUseSameTimeClickPressEvent(bool bUse);

	bool IsPressed();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableRightMouseButtonClick = false;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	FReply SlateHandleClickedParam();
	void SlateHandlePressedParam();
	void SlateHandleReleasedParam();
	void SlateHandleHoveredParam();
	void SlateHandleUnhoveredParam();
	void SlateHandlePressedTimeCheckedParam();

protected:
	UPROPERTY()
	USDKWidgetParam* ClickedWidgetParam;

	bool bUseSameTime_ClickPressEvent = false;

	float PressedCheckStartTime = 0.f;
	float PressedCheckSecondTime = 0.f; 

	FTimerHandle TimerHendlerPressedCheck;
};
