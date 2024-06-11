// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/SDKPlayerController.h"
#include "Character/SDKCamera.h"
#include "Share/SDKDataStruct.h"
#include "Share/SDKStruct.h"
#include "Share/SDKData.h"
#include "Share/SDKEnum.h"
#include "SDKLobbyController.generated.h"

class ASDKFurniture;
class ASDKPlayerCharacter;


UCLASS()
class ARENA_API ASDKLobbyController : public ASDKPlayerController
{
	GENERATED_BODY()

public:
	ASDKLobbyController();

	// Begin APlayerController Interface
	virtual bool InputTouch(uint32 Handle, ETouchType::Type Type, const FVector2D& TouchLocation, float Force, FDateTime DeviceTimestamp, uint32 TouchpadIndex) override;
	// End APlayerController Interface

	// Begin SDKPlayerController Interface
	virtual void ClientAttachHud() override;
	// End SDKPlayerController Interface
	
	/** 오브젝트 클릭 이벤트 설정 */
	void SetEnableClickEvent(bool bEnable);

	//*** Input *************************************************************************************************************//

	/** 인풋 모드 설정 */
	void SetInputGameMode(bool bGameOnly);
	
	void InputTouchHousing(ETouchType::Type Type, uint32 TouchpadIndex);

	//*** MyRoom ************************************************************************************************************//
	/** 마이룸 정보 : 배치된 가구 정보 반환 */
	ASDKFurniture* GetmapFurnitureByUniqueID(const FString strUniqueID) const;

	/** 마이룸 정보 : 각 방의 시작포인트 반환 */
	FVector GetRoomStartLocation(int32 iRoomIndex) const;

	/** 마이룸 정보 : 벽 및 바닥 오브젝트 초기화 */
	void InitMyroomBuildingData();

	/** 마이룸 정보 : 타일 초기화 */
	void InitMyroomTilesData(int32 RoomIndex);

	/** 마이룸 정보 : 저장 */
	UFUNCTION(BlueprintCallable)
	void SaveMyroomHousingData();

	/** 마이룸 정보 : 불러오기 */
	UFUNCTION(BlueprintCallable)
	void LoadMyroomHousingData();

	/** 마이룸 정보 : 원복 */
	void RevertMyroomHousignData();

	/** 마이룸 정보 : 프리셋 적용 */
	void ApplayMyroomPresetData(FString PresetID);

	/** 마이룸 정보 : 불러오기 가구 스폰 */
	void SpawnLoadFurniture(const FString strUniqueID, const CFurnitureData tFurnitureData);

	//*** Housing ***********************************************************************************************************//
	/** 하우징 : 카메라 반환 */
	UFUNCTION(BlueprintCallable)
	ASDKCamera* GetHousingViewCamera() const { return HousingViewCamera; }

	/** 수정중인 가구를 움직이는 중인지 */
	UFUNCTION(BlueprintCallable)
	bool GetIsMovingFurniture() { return IsMovingFurniture; }
	UFUNCTION(BlueprintCallable)
	void SetIsMovingFurniture(bool bMoving) { IsMovingFurniture = bMoving; }

	/** 하우징 : 모드 설정 */
	UFUNCTION(BlueprintCallable)
	void SetIsPutableFurnitrue(bool bPutable) { IsPutableState = bPutable; }

	/** 하우징 확인 : 가구 위 장식 있는지 */
	void CheckDecorationOnFurniture(int32 iRoomKey, FString strUniqueID, FVector vIndex, FVector vSize, TArray<ASDKFurniture*>& ReturnDecorations);

	/** 하우징 계산 : 피킹된 타일 인덱스 */
	FVector CalcPickingTileIndex(int32 iRoomKey, FVector vMousePoint, bool bIsInt = true);

	/** 하우징 계산 : 장식 가구 피킹된 타일 인덱스 */
	FVector CalcDecorationPickingTileIndex(int32 iRoomKey, FVector vMousePoint);

	/** 하우징 계산 : 배치중인 가구 중심위치값 */
	FVector CalcFurnitureCenterLocation(int32 iRoomKey, FVector vIndex, FVector vSize);

	/** 하우징 계산 : 배치중인 장식 가구 중심위치값 */
	FVector CalcDecorationCenterLocation(int32 iRoomKey, FVector vIndex, FVector vSize, float fHitActorHeight);

	/** 하우징 계산 : 배치 가능한 타일인지 */
	bool CalcPossibleArrangeTile(int32 iRoomKey, FVector vIndex, FVector vSize, FString strUniqueID = TEXT(""));

	/** 하우징 계산 : 배치 가능한 타입인지 */
	bool CalcPossibleArrangeType(int32 iRoomKey, FVector vIndex, FVector vSize, FString TableID);

	/** 하우징 정보 : 타일 정보 저장 */
	void SaveMyroomTileData(int32 iRoomIndex, FVector vIndex, FVector vSize, FString strUniqueID);

	/** 하우징 정보 : 타일 정보 삭제 */
	void RemoveMyroomTileData(int32 iRoomIndex, FVector vIndex, FVector vSize);

	/** 하우징 : 가구 배치 */
	ASDKFurniture* PutFurniture(ASDKFurniture* PreviewObject);

	/** 하우징 : 가구 위치 수정 */
	void CompleteModifyFurniture(ASDKFurniture* pModifyFurniture, ASDKFurniture* pPreviewFurniture, bool bIsDeco);

	/** 하우징 : 벽지 적용 */
	void ApplyWallpaperFlooring();

	/** 하우징 : 가구 보관 */
	UFUNCTION(BlueprintCallable)
	void SaveFurniture(ASDKFurniture* SaveObject);

	/** 하우징 : 전체 배치 초기화 */
	UFUNCTION(BlueprintCallable)
	void ClearMyroomAllFurnitureData();

	/** 하우징 카메라 : 초기화 */
	void InitHousingCamera(bool bOpen);

	/** 하우징 카메라 : 스폰 */
	void SpawnHousingViewCamera();

	/** 하우징 카메라 : 위치 설정 */
	void SetLocationHousingCamera(FVector vLocation);

	UFUNCTION(BlueprintCallable)
	void MoveHousingViewCamera(FVector2D vMouseDelta);

	UFUNCTION(BlueprintCallable)
	void RotateHousingViewCamera(bool bIsTop);

	UFUNCTION(BlueprintImplementableEvent)
	void OnStartFurnitureCamera();

	UFUNCTION(BlueprintImplementableEvent)
	void OnEndFurnitureCamera();

	UFUNCTION(BlueprintImplementableEvent)
	void OnSetModifyFurnitureType(EFurnitureType eType);

	//*** HUD ***************************************************************************************************************// 
	/** UI Toggle */
	UFUNCTION(BlueprintCallable)
	void ClientToggleLobbyUIMenu(EUI eMenuType, bool bOpen);

	/** 벽지 및 바닥재 적용 알림 */
	void ClientApplyWallpaperFlooring(bool bResult, FString ID = TEXT(""), FString PreID = TEXT(""));

	/** 저장완료 알림 */
	void ClientySuccessedSaveHounsingData();

	/** 방확장 팝업 */
	UFUNCTION(BlueprintCallable)
	void ClientToggleMyroomUpgradePopup(bool bOpen, int32 iRoomIndex = 0);

private:
	/*** 하우징 관련 변수 ******************************/
	// 바닥 오브젝트
	UPROPERTY()
	TMap<FString, ASDKFurniture*> mapObjectWalls;

	// 벽 오브젝트
	UPROPERTY()
	TMap<FString, ASDKFurniture*> mapObjectFloors;

	// 배치된 가구들
	UPROPERTY()
	TMap<FString, ASDKFurniture*> mapFurnitures;

	// 마이룸 타일 정보
	TMap<int32, CRoomTilesData> mapTilesData;
	TMap<int32, FVector> mapBaseLocation;

	// 카메라 : 하우징 전체 뷰
	UPROPERTY()
	ASDKCamera* HousingViewCamera;

	// 카메라 : 기본 회전값
	UPROPERTY()
	FRotator DefaultRotator;

	// 카메라 : 탑다운 회전값
	UPROPERTY()
	FRotator TopDownRotator;

	// 배치 가능 여부
	bool IsPutableState;

	// 수정하려는 가구를 움직이는 중인지
	bool IsMovingFurniture;
};