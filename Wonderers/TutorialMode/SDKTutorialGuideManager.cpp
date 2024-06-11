// Fill out your copyright notice in the Description page of Project Settings.


#include "TutorialMode/SDKTutorialGuideManager.h"

#include "Kismet/GameplayStatics.h"
#include "Subsystem/SDKGameInstanceSubsystem.h"

#include "Character/SDKHUD.h"

#include "Manager/SDKTableManager.h"

#include "Share/SDKEnum.h"
#include "Share/SDKDataTable.h"
#include "UI/SDKTutorialGuideWidget.h"

#include "Engine/SDKGameDelegate.h"


DEFINE_LOG_CATEGORY(LogGuide)


USDKTutorialGuideManager::USDKTutorialGuideManager()
	: bAutoEndGuide(false), bActiveGuide(false), GuideIndex(0)
{
	GuideDataList.Empty();
	MessageIDList.Empty();
}

USDKTutorialGuideManager::~USDKTutorialGuideManager()
{
	GuideDataList.Empty();
	MessageIDList.Empty();
}

bool USDKTutorialGuideManager::GetClickedSkipButton() const
{
	return bClickedSkip;
}

void USDKTutorialGuideManager::TouchBlockMode()
{
	ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
	if (IsValid(SDKHUD))
	{
		USDKTutorialGuideWidget* TutorialGuideWidget = Cast<USDKTutorialGuideWidget>(SDKHUD->OpenUI(EUI::TutorialGuide));
		if (IsValid(TutorialGuideWidget))
		{
			TutorialGuideWidget->SetVisibilityTouchBlock(true);
			TutorialGuideWidget->SetVisibilityBorder(false);
		}
	}
}

void USDKTutorialGuideManager::StartGuide(float InDelayTime /*= 0.3f*/, bool bAutoEnd /*= false*/)
{
	GetWorld()->GetTimerManager().ClearTimer(TutorialTimer);

	if (GuideDataList.Num() > 0)
	{
		bActiveGuide = true;
		bClickedSkip = false;

		SetAutoEndGuide(bAutoEnd);

		GuideIndex = 0;

		ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
		if (IsValid(SDKHUD))
		{
			USDKTutorialGuideWidget* TutorialGuideWidget = Cast<USDKTutorialGuideWidget>(SDKHUD->OpenUI(EUI::TutorialGuide));
			if (IsValid(TutorialGuideWidget))
			{
				TutorialGuideWidget->SetVisibilityTouchBlock(true);
				TutorialGuideWidget->SetVisibilityBorder(false);
			}
		}

		FTimerDelegate TutorialDelegate;
		TutorialDelegate.BindUFunction(this, FName("OpenGuideWidget"));
		GetWorld()->GetTimerManager().SetTimer(TutorialTimer, TutorialDelegate, InDelayTime, false);
	}
	else
	{
		ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
		if (IsValid(SDKHUD))
		{
			SDKHUD->CloseUI(EUI::TutorialGuide);
		}
	}
}

void USDKTutorialGuideManager::NextGuide(float InDelayTime /*= 0.2f*/)
{
	if (bActiveGuide == false)
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(TutorialTimer);

	ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
	if (IsValid(SDKHUD))
	{
		USDKTutorialGuideWidget* TutorialGuideWidget = Cast<USDKTutorialGuideWidget>(SDKHUD->GetUI(EUI::TutorialGuide));
		if (IsValid(TutorialGuideWidget))
		{
			TutorialGuideWidget->SetVisibilityTouchBlock(true);
		}
	}

	GetWorld()->GetTimerManager().SetTimer(TutorialTimer, this, &USDKTutorialGuideManager::OpenGuideWidget, InDelayTime, false);
}

void USDKTutorialGuideManager::EndGuide()
{
	if (bActiveGuide == false)
	{
		return;
	}

	ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
	if (IsValid(SDKHUD))
	{
		SDKHUD->CloseUI(EUI::TutorialGuide);

		// 끝났음을 알리는 델리게이트
		EndGuideDelegate.ExecuteIfBound();

		FTimerDelegate TutorialDelegate;
		TutorialDelegate.BindWeakLambda(this, [&] { EndGuideDelegate.Unbind(); });
		GetWorld()->GetTimerManager().SetTimerForNextTick(TutorialDelegate);
	}

	GuideDataList.Empty();
	MessageIDList.Empty();

	GuideIndex = 0;
	bActiveGuide = false;
	bAutoEndGuide = false;
}

void USDKTutorialGuideManager::SetInstantGuideID(const int32 InID)
{
	MessageIDList.Empty();

	if (InID <= 0)
	{
		return;
	}

	FS_InstantGuideUI* InstantGuideTable = USDKTableManager::Get()->FindTableRow<FS_InstantGuideUI>(ETableType::tb_InstantGuideUI, FString::FromInt(InID));
	if (InstantGuideTable != nullptr)
	{
		if (InstantGuideTable->Text.Num() >= 0)
		{
			MessageIDList = InstantGuideTable->Text;
		}
	}
}

void USDKTutorialGuideManager::AddGuideWidget(UWidget* InWidget, bool bAuto /*= false*/)
{
	// 같은 걸 다시 해야하는 스탭이 있을 수 있음. 231122 wklee
	//if (GuideDataList.Num() > 0)
	//{
	//	for (auto& iter : GuideDataList)
	//	{
	//		if (iter.GuideWidget.Get() == InWidget)
	//		{
	//			return;
	//		}
	//	}
	//}

	GuideDataList.Add(FGuideData(InWidget, bAuto));
}

void USDKTutorialGuideManager::OpenGuideWidget()
{
	GetWorld()->GetTimerManager().ClearTimer(TutorialTimer);

	ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
	if (IsValid(SDKHUD))
	{
		if ((bActiveGuide && bAutoEndGuide && GuideDataList.Num() <= 0) || GuideDataList.IsValidIndex(0) == false)
		{
			EndGuide();
			return;
		}

		FString GuideID = TEXT("");
		if (MessageIDList.IsValidIndex(GuideIndex))
		{
			GuideID = MessageIDList[GuideIndex].ToString();
		}

		USDKTutorialGuideWidget* TutorialGuideWidget = Cast<USDKTutorialGuideWidget>(SDKHUD->GetUI(EUI::TutorialGuide));
		if (IsValid(TutorialGuideWidget))
		{
			TutorialGuideWidget->SetGuideWidget(GuideDataList[0].GuideWidget.Get(), GuideID, GuideDataList[0].bIsAuto);
		}
	}

	if (GuideDataList.Num() > 0 && GuideDataList[0].bIsAuto)
	{
		GetWorld()->GetTimerManager().SetTimer(TutorialTimer, this, &USDKTutorialGuideManager::OpenGuideWidget, 2.f, false);
	}

	CheckNextGuide();
}

void USDKTutorialGuideManager::CheckNextGuide()
{
	if (GuideDataList.Num() > 0)
	{
		GuideDataList.RemoveAt(0);
	}

	GuideIndex++;
}

USDKTutorialGuideManager* USDKTutorialGuideManager::Get()
{
	return USDKGameInstanceSubsystem::Get()->GetTutorialGuideManager();
}