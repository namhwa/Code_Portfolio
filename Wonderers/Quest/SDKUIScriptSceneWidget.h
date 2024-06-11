// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SDKUserWidget.h"
#include "Share/SDKEnum.h"
#include "SDKUIScriptSceneWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNotifyCloseScriptDelegateEvent);

class UImage;
class UBorder;
class UButton;
class USizeBox;
class UTextBlock;
class UPaperSprite;
class URichTextBlock;
class UWidgetAnimation;

class UDialogueBuilderObject;


UCLASS()
class ARENA_API USDKUIScriptSceneWidget : public USDKUserWidget
{
	GENERATED_BODY()

public:
	USDKUIScriptSceneWidget();

	//Begin SDKUserWidget interface
	virtual void OpenUIProcess() override;
	virtual void CloseUIProcess() override;
	virtual void CreateUIProcess() override;
	//End SDKUserWidget interface

	UFUNCTION(BlueprintCallable)
	UButton* GetButtonSkip() const { return ButtonSkip; }

	void SetScriptID(FString ID);
	void SetDialogObject(UDialogueBuilderObject* NewDialogueObject);

	/** 버튼 바인딩 이벤트 */
	UFUNCTION()
	void SetNextScript();

	UFUNCTION()
	void WriteScriptContents();

	UFUNCTION()
	void OnClickedRateAuto();

	UFUNCTION()
	void OnClickedSkipScript();

	UFUNCTION()
	void OnClickedNextScript();

	UFUNCTION()
	void FinishCloseWidgetAnimation();

	/** 블루프린트 이벤트 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnSetScriptEmotion(const TArray<FName>& Emotions);

public:
	UPROPERTY(BlueprintAssignable)
	FNotifyCloseScriptDelegateEvent NofityCloseScriptEvent;

protected:
	void InitButton();
	void InitDialogue();
	void InitCharacterImages();

	void StartScript();

	void SetToggleTopBar(bool bActive);
	void SetTogglePlayerInput(bool bEnable);
	void SetToggleButtons(bool bActive);
	
	void SetScriptName(FText Name);
	void SetScriptEmotion(const TArray<FName>& Emotions);
	void SetScriptAnim(FString Tag, FString AnimPathID);
	void SetScriptImage(const TArray<bool>& ActiveIndex, const TArray<FName>& ImageIDs);
	void SetScriptCutSceneImage(FString ImageID);
	
	void SetScriptContents(FString TableID);
	void SetScriptContents(FText ScriptText);

	void SetAutoScript(EAutoScriptState InState);
	void SetAutoRate(EAutoScriptState InState);
	void SetActiveButtonSkip(bool bActive);
	
	void SetActiveNextScriptAnim(bool bActive);

	void SetVisibilityAllImages(bool bVisible);

	void UpdateScriptScene();
	void UpdateScriptScene(const TArray<struct FDialogueDetailsStruct>& DialogList);

	void SkippedScriptAnim();
	void StopAnimMotionActors();

	void ClearScriptScene();
	void CloseScript();

protected:
	// Npc 이미지
	UPROPERTY()
	TArray<USDKUserWidget*> BPCharacterImageList;
	UPROPERTY()
	TArray<UImage*> ImageCharacterList;
	UPROPERTY()
	TArray<UImage*> ImageFaceList;

	// 컷씬 이미지
	UPROPERTY(meta = (BindWidget))
	UImage* ImageCutScene;
	UPROPERTY(meta = (BindWidget))
	UBorder* BorderScene;

	// 대화창
	UPROPERTY(meta = (BindWidget))
	USDKUserWidget* BPDialogue;

	UPROPERTY()
	UButton* ButtonNext;

	UPROPERTY()
	UTextBlock* TextName;

	UPROPERTY()
	UTextBlock* TextNameTitle;

	UPROPERTY()
	URichTextBlock* RichTextScript;

	// 버튼
	UPROPERTY(meta = (BindWidget))
	UButton* ButtonAction;

	UPROPERTY(meta = (BindWidget))
	UButton* ButtonAuto;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextAutoRate;

	UPROPERTY(meta = (BindWidget))
	UButton* ButtonSkip;
	UPROPERTY()
	UTextBlock* TextSkip;

	// 애니메이션
	UPROPERTY()
	UWidgetAnimation* NextButtonAnim;

	UPROPERTY()
	UWidgetAnimation* ToggleUIAnim;

private:
	// Dialog Builder Plugin
	UPROPERTY()
	TWeakObjectPtr<UDialogueBuilderObject> DialogObject;

	int32 DialogIndex;
	int32 DialogID;

	// 대사
	FString ScriptID;
	FText ScriptContents;

	int32 WordIndex;
	int32 RichTableStart;
	int32 RichTableEnd;

	int32 ScriptLength;
	int32 MaxWriteCount;

	// 스크립트 자동
	EAutoScriptState AutoScriptState;

	// 대사 출력 속도
	float AutoRate;
	float WriteTime;

	// 모션하는 액터
	UPROPERTY()
	TArray<AActor*> AnimMotionActors;
	UPROPERTY()
	TSoftObjectPtr<UAnimMontage> MotionAnimPtr;

	UPROPERTY()
	UUMGSequencePlayer* CloseAnimSequence;

	EGameMode CurrentMode;
	FTimerHandle WriteTimerHandle;
	FTimerHandle DelayTimerHandle;
	FWidgetAnimationDynamicEvent CloseAnimDelegateEvent;
};
