// Fill out your copyright notice in the Description page of Project Settings.


#include "Quest/SDKUIScriptSceneWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Animation/UMGSequencePlayer.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Stats/Stats.h"

#include "Character/SDKHUD.h"
#include "Character/SDKMyInfo.h"
#include "Character/SDKCharacter.h"
#include "Character/SDKPlayerCharacter.h"
#include "Character/SDKPlayerController.h"
#include "Character/SDKControllerInterface.h"

#include "Engine/SDKAssetManager.h"
#include "Engine/SDKGameInstance.h"
#include "GameMode/SDKRpgGameMode.h"
#include "Manager/SDKTableManager.h"

#include "Share/SDKData.h"
#include "Share/SDKHelper.h"


DECLARE_CYCLE_STAT(TEXT("USDKUIScriptSceneWidget::WriteScriptContents"), STAT_USDKUIScriptSceneWidget_WriteScriptContents, STATGROUP_SDKUserWidget);

USDKUIScriptSceneWidget::USDKUIScriptSceneWidget()
	: WordIndex(-1), RichTableStart(-1), RichTableEnd(-1), AutoScriptState(EAutoScriptState::Inactive), AutoRate(1.0f)
{
	ScriptContents = FText::GetEmpty();

	SetIsEnableCloseByKey(false);
}

void USDKUIScriptSceneWidget::OpenUIProcess()
{
	Super::OpenUIProcess();

	PlayWidgetAnimation(ToggleUIAnim);

	SetToggleButtons(true);

	SetAutoScript(EAutoScriptState::Inactive);

	SetActiveNextScriptAnim(true);

	SetTogglePlayerInput(false);
	SetToggleTopBar(false);
}

void USDKUIScriptSceneWidget::CloseUIProcess()
{
	CloseAnimSequence = nullptr;

	SetToggleTopBar(FSDKHelpers::GetGameMode(GetWorld()) < EGameMode::End_Lobby);

	ClearScriptScene();
	SetToggleButtons(false);

	SetActiveNextScriptAnim(false);

	SetTogglePlayerInput(true);

	if (NofityCloseScriptEvent.IsBound())
	{
		NofityCloseScriptEvent.Broadcast();
	}
}

void USDKUIScriptSceneWidget::CreateUIProcess()
{
	InitButton();
	InitDialogue();
	InitCharacterImages();

	SetAutoScript(EAutoScriptState::Inactive);

	SetWidgetVisibility(TextNameTitle, ESlateVisibility::Collapsed);

	ToggleUIAnim = GetWidgetAnimation(TEXT("on"));

	Super::CreateUIProcess();
}

void USDKUIScriptSceneWidget::SetScriptID(FString ID)
{
	ScriptID = ID;
	if(ScriptID.IsEmpty() || ScriptID == TEXT("0"))
	{
		CloseScript();
	}
	else
	{
		StartScript();
	}
}

void USDKUIScriptSceneWidget::SetDialogObject(UDialogueBuilderObject* NewDialogObject)
{
	DialogObject = NewDialogObject;
	
	if (DialogObject.IsValid())
	{
		DialogID = 0;
		if (DialogObject.Get()->DialoguesData.Num() > 0)
		{
			StartScript();
		}
	}
}

void USDKUIScriptSceneWidget::SetNextScript()
{
	GetWorld()->GetTimerManager().ClearTimer(DelayTimerHandle);

	StopAnimMotionActors();

	if (DialogObject.IsValid())
	{
		if (DialogObject.Get()->MoveToNextNodesFromNodeID(DialogID).Num() > 0)
		{
			UpdateScriptScene(DialogObject.Get()->MoveToNextNodesFromNodeID(DialogID));
		}
		else
		{
			CloseScript();
		}
	}
	else
	{
		auto ScriptTable = USDKTableManager::Get()->FindTableRow<FS_Script>(ETableType::tb_Script, ScriptID);
		if (ScriptID.IsEmpty())
		{
			UpdateScriptScene();
		}
		else
		{
			CloseScript();
		}
	}
}

void USDKUIScriptSceneWidget::WriteScriptContents()
{
	SCOPE_CYCLE_COUNTER(STAT_USDKUIScriptSceneWidget_WriteScriptContents);
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("USDKUIScriptSceneWidget::WriteScriptContents"));

	if(RichTableStart > INDEX_NONE && WordIndex == RichTableStart)
	{
		WordIndex = FMath::Min((RichTableEnd + 1), ScriptLength);
	}

	FString strScript = ScriptContents.ToString().Left(FMath::Min(WordIndex, ScriptLength));
	SetRichTextBlockText(RichTextScript, FText::FromString(strScript));
	WordIndex++;

	if(WordIndex > MaxWriteCount)
	{
		WordIndex = 0;
		ScriptContents = FText::GetEmpty();

		GetWorld()->GetTimerManager().ClearTimer(WriteTimerHandle);

		if(AutoScriptState > EAutoScriptState::Inactive)
		{
			float DelayTime = 0.5f;
			DelayTime = (DelayTime <= 0.f) ? 0.02f : DelayTime;

			GetWorld()->GetTimerManager().SetTimer(DelayTimerHandle, this, &USDKUIScriptSceneWidget::SetNextScript, DelayTime, false);
		}
	}
}

void USDKUIScriptSceneWidget::OnClickedRateAuto()
{
	if (static_cast<int32>(AutoScriptState) + 1 >= static_cast<int32>(EAutoScriptState::MAX))
	{
		SetAutoScript(EAutoScriptState::Inactive);
	}
	else
	{
		SetAutoScript(static_cast<EAutoScriptState>(static_cast<int32>(AutoScriptState) + 1));
	}

	if(WriteTimerHandle.IsValid())
	{
		float fWriteDelay = (WriteTime / AutoRate) <= 0.f ? 0.2f : (WriteTime / AutoRate);
		GetWorld()->GetTimerManager().ClearTimer(WriteTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(WriteTimerHandle, this, &USDKUIScriptSceneWidget::WriteScriptContents, fWriteDelay, true);
	}
}

void USDKUIScriptSceneWidget::OnClickedSkipScript()
{
	if (IsValid(CloseAnimSequence))
	{
		return;
	}

	SetWidgetVisibility(ButtonSkip, ESlateVisibility::HitTestInvisible);

	CloseScript();
}

void USDKUIScriptSceneWidget::OnClickedNextScript()
{
	if (IsValid(CloseAnimSequence))
	{
		return;
	}

	if(WriteTimerHandle.IsValid())
	{
		SkippedScriptAnim();
	}
	else
	{
		SetNextScript();
	}
}

void USDKUIScriptSceneWidget::FinishCloseWidgetAnimation()
{
	UnbindAllFromAnimationFinished(ToggleUIAnim);

	auto SDKPlayerController = Cast<ASDKPlayerController>(GetOwningPlayer());
	if (SDKPlayerController != nullptr)
	{
		SDKPlayerController->ClientToggleScriptSceneTableID(false);
	}
}

void USDKUIScriptSceneWidget::InitButton()
{
	if (IsValid(ButtonAction))
	{
		if (!ButtonAction->OnClicked.Contains(this, FName("OnClickedNextScript")))
		{
			ButtonAction->OnClicked.AddDynamic(this, &USDKUIScriptSceneWidget::OnClickedNextScript);
		}
	}

	if (IsValid(ButtonAuto))
	{
		if (!ButtonAuto->OnClicked.Contains(this, FName("OnClickedRateAuto")))
		{
			ButtonAuto->OnClicked.AddDynamic(this, &USDKUIScriptSceneWidget::OnClickedRateAuto);
		}
	}

	if (IsValid(ButtonSkip))
	{
		if (!ButtonSkip->OnClicked.Contains(this, FName("OnClickedSkipScript")))
		{
			ButtonSkip->OnClicked.AddDynamic(this, &USDKUIScriptSceneWidget::OnClickedSkipScript);
		}
	}

	SetTextBlockTableText(TextSkip, TEXT("UIText_Quest_Skip"));
}

void USDKUIScriptSceneWidget::InitDialogue()
{
	if (IsValid(BPDialogue))
	{
		TextName = Cast<UTextBlock>(BPDialogue->GetWidgetFromName(FName("TextName")));
		TextNameTitle = Cast<UTextBlock>(BPDialogue->GetWidgetFromName(FName("TextNameTitle")));
		RichTextScript = Cast<URichTextBlock>(BPDialogue->GetWidgetFromName(FName("RichTextScript")));

		NextButtonAnim = BPDialogue->GetWidgetAnimation(TEXT("Loop"));

		ButtonNext = Cast<UButton>(BPDialogue->GetWidgetFromName(FName("ButtonNext")));
		if (IsValid(ButtonNext))
		{
			if (!ButtonNext->OnClicked.Contains(this, FName("OnClickedNextScript")))
			{
				ButtonNext->OnClicked.AddDynamic(this, &USDKUIScriptSceneWidget::OnClickedNextScript);
			}
		}
	}
}

void USDKUIScriptSceneWidget::InitCharacterImages()
{
	for (int32 i = 1; i <= 3; ++i)
	{
		USDKUserWidget* BPImageWidget = Cast<USDKUserWidget>(GetWidgetFromName(FName(FString::Printf(TEXT("BPCharacter_%d"), i))));
		if (IsValid(BPImageWidget))
		{
			BPCharacterImageList.Add(BPImageWidget);
			ImageCharacterList.Add(Cast<UImage>(BPImageWidget->GetWidgetFromName(FName("Body"))));
			ImageFaceList.Add(Cast<UImage>(BPImageWidget->GetWidgetFromName(FName("Face"))));
		}
		else
		{
			BPCharacterImageList.Add(nullptr);
			ImageCharacterList.Add(nullptr);
			ImageFaceList.Add(nullptr);
		}
	}
}

void USDKUIScriptSceneWidget::StartScript()
{
	// 시퀀스 재생 중일 때, 대사 넘김 안되도록
	USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (IsValid(SDKGameInstance))
	{
		SetAutoScript(SDKGameInstance->GetPlayingLevelSequence() ? EAutoScriptState::Inactive : AutoScriptState);
	}

	if (DialogObject.IsValid())
	{
		TArray<FDialogueDetailsStruct> StartDialogue;
		StartDialogue.Add(DialogObject.Get()->StartDialogue());

		UpdateScriptScene(StartDialogue);
	}
	else
	{
		UpdateScriptScene();
	}
}

void USDKUIScriptSceneWidget::SetToggleTopBar(bool bActive)
{
	ASDKHUD* SDKHUD = GetOwningPlayer()->GetHUD<ASDKHUD>();
	if(IsValid(SDKHUD))
	{
		USDKUserWidget* TopBarWidget = SDKHUD->GetUI(EUI::TopBar);
		if (IsValid(TopBarWidget))
		{
			if (bActive)
			{
				SetWidgetVisibility(TopBarWidget, ESlateVisibility::SelfHitTestInvisible);
			}
			else
			{
				if (TopBarWidget->GetVisibility() != ESlateVisibility::Collapsed)
				{
					SetWidgetVisibility(TopBarWidget, ESlateVisibility::Collapsed);
				}
			}
		}
	}
}

void USDKUIScriptSceneWidget::SetTogglePlayerInput(bool bEnable)
{
	ASDKPlayerCharacter* SDKCharacter = GetOwningPlayerPawn<ASDKPlayerCharacter>();
	if(IsValid(SDKCharacter))
	{
		SDKCharacter->ClientInputModeChange(bEnable);
	}
}

void USDKUIScriptSceneWidget::SetToggleButtons(bool bActive)
{
	ESlateVisibility eVisibleButton = bActive ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible;
	SetWidgetVisibility(ButtonNext, eVisibleButton);
	SetWidgetVisibility(ButtonAction, eVisibleButton);
	SetWidgetVisibility(ButtonSkip, eVisibleButton);
}

void USDKUIScriptSceneWidget::SetScriptName(FText Name)
{
	SetTextBlockText(TextName, Name);
}

void USDKUIScriptSceneWidget::SetScriptEmotion(const TArray<FName>& Emotions)
{
	OnSetScriptEmotion(Emotions);
}

void USDKUIScriptSceneWidget::SetScriptAnim(FString Tag, FString AnimPathID)
{
	AnimMotionActors.Empty();

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(Tag), AnimMotionActors);
	if (AnimMotionActors.Num() > 0)
	{
		if (AnimPathID.IsEmpty() || AnimPathID == TEXT("0"))
		{
			return;
		}

		auto AnimPathTable = USDKTableManager::Get()->FindTableRow<FS_AnimationPath>(ETableType::tb_AnimationPath, AnimPathID);
		if (AnimPathTable != nullptr)
		{
			MotionAnimPtr = AnimPathTable->AnimMontage;
		}

		for (auto iter : AnimMotionActors)
		{
			ASDKCharacter* ItCharacter = Cast<ASDKCharacter>(iter);
			if (IsValid(ItCharacter))
			{
				ItCharacter->MultiPlayAnimMontageWithPath(MotionAnimPtr.ToString(), 1.f, 0);
			}
		}
	}
}

void USDKUIScriptSceneWidget::SetScriptImage(const TArray<bool>& ActiveIndex, const TArray<FName>& ImageIDs)
{
	if (ImageIDs.Num() <= 0)
	{
		SetVisibilityAllImages(false);
		return;
	}

	for (int32 i = 0; i < ImageIDs.Num(); ++i)
	{
		FString ImageBodyPath = TEXT("");
		FString ImageFacePath = TEXT("");

		if(ImageIDs[i] != FName("0") && !ImageIDs[i].IsNone())
		{
			auto ScriptDataTable = USDKTableManager::Get()->FindTableRow<FS_ScriptData>(ETableType::tb_ScriptData, ImageIDs[i].ToString());
			if (ScriptDataTable != nullptr)
			{
				ImageBodyPath = ScriptDataTable->CharacterImage.ToString();
				ImageFacePath = ScriptDataTable->CharacterFaceImage.ToString();
			}
		}

		bool bActiveImage = ActiveIndex.IsValidIndex(i) && ActiveIndex[i];
		if (ImageCharacterList.IsValidIndex(i) && IsValid(ImageCharacterList[i]))
		{
			SetWidgetVisibility(ImageCharacterList[i], ImageBodyPath.IsEmpty() ? ESlateVisibility::Hidden : ESlateVisibility::SelfHitTestInvisible);
			SetImageTexturePath(ImageCharacterList[i], ImageBodyPath);
			SetImageTintColor(ImageCharacterList[i], DesignFormatToLinearColor(bActiveImage ? EDesignFormat::Default_Active : EDesignFormat::Default_Inactive_Dark));
		}

		if (ImageFaceList.IsValidIndex(i) && IsValid(ImageFaceList[i]))
		{
			SetWidgetVisibility(ImageFaceList[i], ImageFacePath.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
			SetImageTexturePath(ImageFaceList[i], ImageFacePath);
			SetImageTintColor(ImageFaceList[i], DesignFormatToLinearColor(bActiveImage ? EDesignFormat::Default_Active : EDesignFormat::Default_Inactive_Dark));
		}
	}

	SetVisibilityAllImages(true);
}

void USDKUIScriptSceneWidget::SetScriptCutSceneImage(FString ImageID)
{
	bool bEnableID = !ImageID.IsEmpty() && ImageID != TEXT("0");
	if (bEnableID)
	{
		SetImageTextureID(ImageCutScene, ImageID);
	}

	SetWidgetVisibility(ImageCutScene, bEnableID ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	SetWidgetVisibility(BorderScene, bEnableID ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void USDKUIScriptSceneWidget::SetScriptContents(FString TableID)
{
	WordIndex = 0;
	ScriptLength = 0;
	MaxWriteCount = 0;

	FFormatNamedArguments Arguments;
	FText TableText = FSDKHelpers::GetTableText(TEXT("Common"), TableID);

	USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (IsValid(SDKGameInstance))
	{
		Arguments.Add(TEXT("Nickname"), FText::FromString(SDKGameInstance->MyInfoManager->GetAccountData().GetNickname()));
	}

	ScriptContents = FText::Format(TableText, Arguments);
	if(!ScriptContents.IsEmpty())
	{
		FString strScript = ScriptContents.ToString();

		RichTableStart = strScript.Find(TEXT("<"));
		if (RichTableStart > INDEX_NONE)
		{
			RichTableEnd = strScript.Find(TEXT(">"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		}

		ScriptLength = ScriptContents.ToString().Len();
		MaxWriteCount = ScriptContents.ToString().Len();
	}
}

void USDKUIScriptSceneWidget::SetScriptContents(FText ScriptText)
{
	WordIndex = 0;
	ScriptLength = 0;
	MaxWriteCount = 0;

	FFormatNamedArguments Arguments;
	USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (IsValid(SDKGameInstance))
	{
		Arguments.Add(TEXT("Nickname"), FText::FromString(SDKGameInstance->MyInfoManager->GetAccountData().GetNickname()));
	}

	ScriptContents = FText::Format(ScriptText, Arguments);
	if (!ScriptContents.IsEmpty())
	{
		FString strScript = ScriptContents.ToString();

		RichTableStart = strScript.Find(TEXT("<"));
		if (RichTableStart > INDEX_NONE)
		{
			RichTableEnd = strScript.Find(TEXT(">"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		}

		ScriptLength = ScriptContents.ToString().Len();
		MaxWriteCount = ScriptContents.ToString().Len();
	}
}

void USDKUIScriptSceneWidget::SetAutoScript(EAutoScriptState InState)
{
	AutoScriptState = InState;

	SetAutoRate(AutoScriptState);
	SetActiveNextScriptAnim(InState == EAutoScriptState::Inactive);

	SetWidgetVisibility(ButtonNext, InState > EAutoScriptState::Inactive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Visible);
	SetWidgetVisibility(ButtonAction, InState > EAutoScriptState::Inactive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Visible);

	if(InState > EAutoScriptState::Inactive && !WriteTimerHandle.IsValid())
	{
		SetNextScript();
	}
}

void USDKUIScriptSceneWidget::SetAutoRate(EAutoScriptState InState)
{
	switch (InState)
	{
	case EAutoScriptState::Inactive:
		{
			AutoRate = 1.0f;
			SetTextBlockTableText(TextAutoRate, TEXT("UIText_Quest_Auto"));
		}
		break;
	case EAutoScriptState::Speed_x1:
		{
			AutoRate = 1.0f;

			FText TableText = FSDKHelpers::GetTableText(TEXT("Common"), TEXT("UIText_Script_Rate"));
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Rate"), NumberToText(AutoRate, 0, 1));

			SetTextBlockText(TextAutoRate, FText::Format(TableText, Arguments));
		}
		break;
	case EAutoScriptState::Speed_x2:
		{
			AutoRate = 2.0f;

			FText TableText = FSDKHelpers::GetTableText(TEXT("Common"), TEXT("UIText_Script_Rate"));
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Rate"), NumberToText(AutoRate, 0, 1));

			SetTextBlockText(TextAutoRate, FText::Format(TableText, Arguments));
		}
		break;
	default:
		break;
	}
}

void USDKUIScriptSceneWidget::SetActiveButtonSkip(bool bActive)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// 테스트일 때, 스킵 가능하도록
	bActive = true;
#endif

	ButtonSkip->SetIsEnabled(bActive);

	SetWidgetVisibility(ButtonSkip, bActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void USDKUIScriptSceneWidget::SetActiveNextScriptAnim(bool bActive)
{
	if(IsValid(BPDialogue))
	{
		if (bActive)
		{
			BPDialogue->PlayWidgetAnimationLoop(NextButtonAnim);
		}
		else
		{
			BPDialogue->StopWidgetAnimation(NextButtonAnim);
		}
	}
}

void USDKUIScriptSceneWidget::SetVisibilityAllImages(bool bVisible)
{
	if (BPCharacterImageList.Num() > 0)
	{
		for (auto iter : BPCharacterImageList)
		{
			SetWidgetVisibility(iter, bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

void USDKUIScriptSceneWidget::UpdateScriptScene()
{
	auto ScriptTable = USDKTableManager::Get()->FindTableRow<FS_Script>(ETableType::tb_Script, ScriptID);
	if (ScriptTable == nullptr)
	{
		CloseScript();
		return;
	}

	SetScriptName(FSDKHelpers::GetTableText(TEXT("Common"), ScriptTable->Name));
	SetScriptImage(ScriptTable->Speaker, ScriptTable->CharacterImage);
	SetScriptEmotion(ScriptTable->Emotion);
	SetScriptContents(ScriptTable->Description);
	SetScriptCutSceneImage(ScriptTable->CutSceneImage);

	SetScriptAnim(ScriptTable->Tag, ScriptTable->Tag_Motion.ToString());

	WriteTime = ScriptTable->WriteTime;

	SetActiveButtonSkip(ScriptTable->Skippable);

	// 다음 ScriptID 설정
	if (ScriptTable->LinkID.IsEmpty() || ScriptTable->LinkID == TEXT("0"))
	{
		ScriptID.Empty();
	}
	else
	{
		ScriptID = ScriptTable->LinkID;
	}

	// 대사 시간 타이머 설정
	if (WriteTime > 0.f && ScriptLength > 0)
	{
		float fWriteDelay = (WriteTime / AutoRate) <= 0.f ? 0.02f : (WriteTime / AutoRate);
		GetWorld()->GetTimerManager().SetTimer(WriteTimerHandle, this, &USDKUIScriptSceneWidget::WriteScriptContents, fWriteDelay, true, 0.f);
	}
	else
	{
		SetRichTextBlockText(RichTextScript, ScriptContents);
	}
}

void USDKUIScriptSceneWidget::UpdateScriptScene(const TArray<FDialogueDetailsStruct>& DialogList)
{
	if (DialogList.Num() <= 0)
	{
		CloseScript();
		return;
	}

	auto FirstDialog = DialogList.CreateConstIterator();
	if (DialogID > FirstDialog->NodeId)
	{
		CloseScript();
		return;
	}

	DialogID = FirstDialog->NodeId;

	if (FirstDialog->bUseTableID)
	{
		auto ScriptTable = USDKTableManager::Get()->FindTableRow<FS_Script>(ETableType::tb_Script, FirstDialog->ScriptID);
		if (ScriptTable != nullptr)
		{
			SetScriptName(FSDKHelpers::GetTableText(TEXT("Common"), ScriptTable->Name));
			SetScriptImage(ScriptTable->Speaker, ScriptTable->CharacterImage);
			SetScriptEmotion(ScriptTable->Emotion);
			SetScriptContents(ScriptTable->Description);
			SetScriptCutSceneImage(ScriptTable->CutSceneImage);

			SetScriptAnim(ScriptTable->Tag, ScriptTable->Tag_Motion.ToString());

			WriteTime = ScriptTable->WriteTime;

			SetActiveButtonSkip(ScriptTable->Skippable);
		}
	}
	else
	{
		SetNextScript();
	}

	// 대사 시간 타이머 설정
	if (WriteTime > 0.f && ScriptLength > 0)
	{
		float fWriteDelay = (WriteTime / AutoRate) <= 0.f ? 0.02f : (WriteTime / AutoRate);
		GetWorld()->GetTimerManager().SetTimer(WriteTimerHandle, this, &USDKUIScriptSceneWidget::WriteScriptContents, fWriteDelay, true, 0.f);
	}
	else
	{
		SetRichTextBlockText(RichTextScript, ScriptContents);
	}
}

void USDKUIScriptSceneWidget::SkippedScriptAnim()
{
	GetWorld()->GetTimerManager().ClearTimer(WriteTimerHandle);
	SetRichTextBlockText(RichTextScript, ScriptContents);

	WordIndex = 0;
	ScriptContents = FText::GetEmpty();
}

void USDKUIScriptSceneWidget::StopAnimMotionActors()
{
	if (AnimMotionActors.Num() > 0)
	{
		for (auto iter : AnimMotionActors)
		{
			ASDKCharacter* ItCharacter = Cast<ASDKCharacter>(iter);
			if (IsValid(ItCharacter))
			{
				ItCharacter->ClientStopAnimMontage();
			}
		}
	}
}

void USDKUIScriptSceneWidget::ClearScriptScene()
{
	SetAutoScript(EAutoScriptState::Inactive);
	SetScriptName(FText::GetEmpty());

	DialogID = 0;
	WordIndex = 0;

	ScriptID.Empty();

	RichTableStart = 0;
	RichTableEnd = 0;

	ScriptLength = 0;
	MaxWriteCount = 0;

	WriteTime = 0.f;

	ScriptContents = FText::GetEmpty();
	SetRichTextBlockText(RichTextScript, ScriptContents);

	GetWorld()->GetTimerManager().ClearTimer(WriteTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(DelayTimerHandle);
}

void USDKUIScriptSceneWidget::CloseScript()
{
	if (IsValid(CloseAnimSequence))
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(WriteTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(DelayTimerHandle);

	CloseAnimDelegateEvent.BindDynamic(this, &USDKUIScriptSceneWidget::FinishCloseWidgetAnimation);
	BindToAnimationFinished(ToggleUIAnim, CloseAnimDelegateEvent);

	CloseAnimSequence = PlayWidgetAnimation(ToggleUIAnim, EUMGSequencePlayMode::Reverse);
}
