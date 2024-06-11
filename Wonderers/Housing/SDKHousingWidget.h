// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SDKUserWidget.h"
#include "Share/SDKEnum.h"
#include "Share/SDKStruct.h"
#include "Share/SDKStructUI.h"
#include "SDKHousingWidget.generated.h"

class UBorder;
class USizeBox;
class UButton;
class UHorizontalBox;

class ASDKFurniture;
class ASDKLobbyController;
class ASDKPreviewFurniture;

class USDKWidgetParam;
class USDKFurnitureBoxWidget;
class USDKHousingButtonWidget;
class USDKHousingCategoryWidget;


DECLARE_LOG_CATEGORY_EXTERN(LogHousing, Log, All);

UCLASS()
class ARENA_API USDKHousingWidget : public USDKUserWidget
{
	GENERATED_BODY()

public:
	USDKHousingWidget();

	// Begin UserWidget Instance
	virtual void NativeConstruct() override;
	virtual FReply NativeOnTouchGesture(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	virtual void OpenProcess() override;
	virtual void CloseProcess() override;
	// End UserWidget Instance

	/** 초기화 */
	void InitHousingWidget();
	void InitButtonSetting();

	/** 반환 : 하우징 상태 */
	EHousingState GetHousingState() const { return HousingState; }

	/** 반환 : 충돌 결과 */
	FHitResult GetPreviewHitResult() const { return PreviewHitResult; }

	/** 반환 : 미리보기 가구 액터 */
	ASDKFurniture* GetPreviewFurniture() { return PreviewGroup.BaseFurniture; }

	/** 설정 : 토글 프로세스 */
	void SetToggleProcess(bool bOpen);
	void SetCloseUIHandleBinding(bool bBind);

	/** 설정 : 보유 가구 목록 */
	void SetFurnitureList();
	void SetFurnitureCount(FString FurnitureID, int32 NewCount);

	/** 설정 : 컨트롤러 & 카메라 */
	void SetLobbyControllerInputMode(bool bGameOnly);
	void SetActiveHousingCamera(bool bActive);

	/** 설정 : 위젯 */
	void SetHousingState(EHousingState state);
	void SetActiveBackgroundBorder(bool bActive);
	void SetHiddenListState(bool bHidden);

	/** 설정 : 미리보기 가구 */
	void SetPreviewFurnitureCollision(bool bEnable);
	void SetPreviewGroupMaterialType(bool bEnable);

	/** 설정 : 수정하려는 가구 */
	void SetModifyFurniture(ASDKFurniture* NewFurniture);
	void SetModifyMeshOffsetRotator(FVector vSize);
	void SetVisibilityModifyFurniture(bool bVisible);

	/** 정렬 : 위젯 카테고리 타입에 따른 정렬 */
	void SortFurnitureBoxByCategory(EFurnitureCategory eCategory);

	/** 검사 : 선택한 가구 이동 가능 여부 */
	bool CheckMovablePickFurniture(ETouchIndex::Type type);

	/** 검사 : 배치된 가구 선택 */
	void CheckClickEventMyroomFurnitur(ASDKFurniture* ClickedObject);

	/** 검사 : 배치 가능 타입 여부 */
	bool CheckDecorationPutableType(ECollisionChannel eCollisionType, AActor* HitActor);

	/** 검사 : 일반 가구 배치 가능 여부 */
	bool CheckGeneralPreviewPutable(FHitResult tHitResult);

	/** 검사 : 장식 가구 배치 가능 여부 */
	bool CheckDecorationPreviewPutable(FHitResult tHitResult);

	/** 검사 : 새로 생성된 예시 가구 위치 */
	bool CheckCreatePreviewPosition();

	/** 계산 : 벽장식 회전값 */
	void CalcWallFurnitureRotator(AActor* HitActor);

	/** 계산 : 회전 시, 시작 인덱스 */
	void CalcRotatorSavedIndex(float fRotatorYaw, FVector& vSavedIndex, FVector& vSize);

	/** 생성 : 목록 위젯 */
	void CreateMenuButtonWidget(int32 index);
	void CreateFurnitrueBoxWidget(FString TableID);

	/** 생성 : 미리 보여주는 가구 */
	void CreatePreviewFurniture(FString TableID);
	void CreateModifyPreviewFurniture(ASDKFurniture* pBaseFurniture, ASDKPreviewFurniture*& pPreview);

	/** 제거 : 예시 가구 */
	void RemovePreviewFurniture();

	/** 시작 : 가구 배치 */
	bool StartMovablePreviewFurniture(ETouchIndex::Type type);

	/** 갱신 : 예시 가구 위치 */
	void UpdatePreviewFurniture(ETouchIndex::Type type);

	/** 갱신 : 가구 위치 */
	bool UpdatePreviewGeneral();

	/** 갱신 : 벽지 또는 바닥재 */
	bool UpdatePreviewWallAndFloor();

	/** 갱신 : 가구 정보 */
	void UpdatePreviewFurnitureData(int32 iRoomIndex, FVector vLocation, FVector vIndex);

	/** 배치 모드 : 스위치 */
	void TogglePutMode(bool bOn, FString TableID = TEXT(""));

	/** 배치 모드 : Drop 가구 배치 완료 */
	UFUNCTION(BlueprintCallable)
	void CompletePutFurniture(bool bComplete);

	/** 수정 모드 : 스위치 */
	bool ToggleModifyMode(bool bOn, ASDKFurniture* ModifyObject = nullptr);

	/** 수정 모드 : 가구 보관 */
	void SavedModifyFurniture();

	/** 수정 모드 : 가구 회전 */
	bool RotatorModifyFurniture();

	/** 수정 모드 : Drop  가구 배치 완료 */
	void CompleteMoveModifyFurniture(bool bComplete);

	/** 수정 모드 : 종료 */
	UFUNCTION(BlueprintCallable)
	void CompleteModifyFurniture(bool bComplete);

	/** 저장 : 하우징 가구 정보 */
	void SaveHousingArrangeData();

	/** 초기화 : 변수 정보 */
	void ClearHousingParameters();

	/** 토글 : 하우징 목록 */
	void ToggleFurnitureList(bool bVisible);

	/** 토글 : 하우징 카메라 회전 */
	void ToggleRotatorHousingCamera(bool bRotate);

	/** 토글 : 가구 초기화 종료 팝업 */
	UFUNCTION()
	void ToggleResetMessage();

	/** 토글 : 하우징 종료 팝업 */
	void ToggleCloseMessage();

	void OnClickedToggleFurnitureList(bool bOpen);

	UFUNCTION()
	void OnClickedResetButton();

	UFUNCTION()
	void OnClickedClosedWidget();

	UFUNCTION()
	void OnClickedCancelClosed();

	UFUNCTION(BlueprintImplementableEvent)
	void OnPlayToggleFurnitureList(bool bVisible);

private:
	UPROPERTY(meta = (BindWidget))
	USDKHousingCategoryWidget* HousingCategoryWidget;

	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* HorizontalBoxList;

	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* HorizontalBoxMenu;

	UPROPERTY(meta = (BindWidget))
	USizeBox* SizeBoxList;

	UPROPERTY(meta = (BindWidget))
	UBorder* BorderBackground;

	UPROPERTY(meta = (BindWidget))
	UButton* ButtonReset;

private:
	// 수정하려는 가구 그룹
	UPROPERTY()
	FFurnitureGroup ModifyGroup;

	// 미리보기 가구 그룹
	UPROPERTY()
	FPreviewGroup PreviewGroup;

	// raycast 결과
	UPROPERTY()
	FHitResult PreviewHitResult;

	// 회전 중심 오프셋 인덱스
	UPROPERTY()
	int32 OffsetIndex;

	// 하우징 상태
	UPROPERTY()
	EHousingState HousingState;

	// 리스트 표시
	UPROPERTY()
	bool bListVisible;

	// 메뉴 숨김 상태 여부 (true : 표시 / false : 숨김)
	UPROPERTY()
	bool bHiddenListState;	

private:
	// 배치 가능한 가구 목록
	TMap<FString, USDKFurnitureBoxWidget*> mapFurnitures;

	// 버튼 위젯 목록
	TArray<USDKHousingButtonWidget*> MenuButtons;

	// 회전 중심 오프셋
	TArray<FVector2D> RotatorMeshOffset;

	// 클로즈 핸들
	FDelegateHandle CloseUIHandle;
};