// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Share/SDKEnum.h"
#include "SDKNpcComponent.generated.h"

class UPrimitiveComponent;
class ASDKPlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE (FFinishNpcScript);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARENA_API USDKNpcComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USDKNpcComponent();

public:
	// Begin UActorComponent Interface
	virtual void BeginPlay() override;
	virtual void SetActive(bool bNewActive, bool bReset = false);
	// End UActorComponent Interface

	/** Overlap 이벤트 */
	UFUNCTION()
	void BeginOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void EndOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	/** 인터렉션 시, 대화 UI 활성화 */
	UFUNCTION()
	void ScriptSceneToActor(AActor* OtherAcotr);
	
	/** 대화 UI 완료 후 처리 */
	UFUNCTION()
	void FinishScriptScene();

	/** 대화 UI 완료 후 처리 */
	UFUNCTION()
	void OnClosedUIReActiveRelaion();

protected:
	void UnBindScriptWidgetEvent();

	void SetActiveInteractionButton(bool bEnable, ASDKPlayerController* TargetController);

	bool HasOwnerInterationBoxComponent();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUI InteractUIType;

	UPROPERTY(EditAnywhere)
	TArray<FString> ActiveScirptID;

	UPROPERTY(EditAnywhere)
	TArray<FString> InactiveScriptID;

	/** 대화 UI 완료 후 델리게이트 이벤트 설정 유무입니다.
	OnFinishScriptActive 혹은 OnFinishScriptInactive 델리게이트 이벤트를 받을 수 있도록 합니다.*/
	UPROPERTY(EditAnywhere)
	bool bUseEvent;

	UPROPERTY(BlueprintAssignable)
	FFinishNpcScript OnFinishScriptActive;

	UPROPERTY(BlueprintAssignable)
	FFinishNpcScript OnFinishScriptInactive;

private:
	UPROPERTY()
	AActor* OverlapActor;
};
