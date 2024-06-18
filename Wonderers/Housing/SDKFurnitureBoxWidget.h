// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SDKUserWidget.h"
#include "Share/SDKEnum.h"
#include "SDKFurnitureBoxWidget.generated.h"

class UImage;
class UTextBlock;

class USDKButtonParam;
class USDKWidgetParam;

DECLARE_LOG_CATEGORY_EXTERN(LogFurnitureBox, Log, All);


UCLASS()
class ARENA_API USDKFurnitureBoxWidget : public USDKUserWidget
{
	GENERATED_BODY()
	
public:
	/** 가구 정보 초기화 */
	void InitFurnitureInfo();

	/** 가구 타입 반환 */
	UFUNCTION(BlueprintCallable)
	EFurnitureType GetFurnitureType() const { return FurnitureType; }

	/** 가구 카테고리 타입 반환 */
	EFurnitureCategory GetFurnitureCategoryType() const { return CategoryType; }

	/** 테이블 아이디 */
	UFUNCTION(BlueprintCallable)
	FString GetTableID() const { return TableID; }
	void SetTableID(FString NewTableID);

	/** 보유한 가구의 갯수 */
	int32 GetOwnedCount() const { return OwnedCount; }
	void SetOwnedCount(int32 NewCount);

	/** 설정 : 가구 버튼 */
	void SetButtonFurnitureParam();

	UFUNCTION()
	void OnClickedFurniture(USDKWidgetParam* param);

private:
	UPROPERTY(meta = (BindWidget))
	USDKButtonParam* SDKButtonFurniture;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextFurnitureName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextFurnitureType;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextOwnedCount;
	
	UPROPERTY(meta = (BindWidget))
	UImage* ImageFurnitureIcon;

private:
	// 가구 테이블 ID
	UPROPERTY()
	FString TableID;

	// 가구 타입
	UPROPERTY()
	EFurnitureType FurnitureType;

	// 가구 카테고리 타입
	UPROPERTY()
	EFurnitureCategory CategoryType;

	// 최대 갯수
	UPROPERTY()
	int32 MaxCount;

	// 보유 갯수
	UPROPERTY()
	int32 OwnedCount;
};
