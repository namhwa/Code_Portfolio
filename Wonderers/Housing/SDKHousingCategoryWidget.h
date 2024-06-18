// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SDKUserWidget.h"
#include "Share/SDKStructUI.h"
#include "SDKHousingCategoryWidget.generated.h"

class UGridPanel;
class USDKWidgetParam;

UCLASS()
class ARENA_API USDKHousingCategoryWidget : public USDKUserWidget
{
	GENERATED_BODY()
	
public:
	//Begine UserWidget Interface
	virtual void CreateUIProcess() override;
	//End UserWidget Interface
	
	void InitCategoryButtons();

	// 버튼 이미지 경로 반환
	FString GetCategoryButtonImagePath(int32 iIndex);

	UFUNCTION()
	void OnClickEventCategoryButton(bool bIsChecked, USDKWidgetParam* param);

private:
	UPROPERTY(meta = (BindWidget))
	UGridPanel* GridPanelCategory;

	// 버튼 연결 델리게이트
	FCreateUIMenuCheckBoxParam GridCategoryButton;
};
