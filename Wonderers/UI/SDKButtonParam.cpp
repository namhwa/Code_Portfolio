// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SDKButtonParam.h"
#include "Components/ButtonSlot.h"

#include "UI/SSDKButton.h"
#include "Manager/SDKTableManager.h"
#include "Share/SDKHelper.h"

USDKButtonParam::USDKButtonParam(const FObjectInitializer& ObjectInitializer)
	: UButton(ObjectInitializer)
{
}

TSharedRef<SWidget> USDKButtonParam::RebuildWidget()
{
	MyButton = SNew(SSDKButton)
		.OnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, SlateHandleClickedParam))
		.OnPressed(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandlePressedParam))
		.OnReleased(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandleReleasedParam))
		.OnHovered(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandleHoveredParam))
		.OnUnhovered(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandleUnhoveredParam))
		.ButtonStyle(&WidgetStyle)
		.ClickMethod(ClickMethod)
		.TouchMethod(TouchMethod)
		.PressMethod(PressMethod)
		.IsFocusable(IsFocusable)
		;

	if (GetChildrenCount() > 0)
	{
		Cast<UButtonSlot>(GetContentSlot())->BuildSlot(MyButton.ToSharedRef());
	}

#if PLATFORM_DESKTOP
	IsFocusable = false;
#else
	IsFocusable = true;
#endif

	if (bEnableRightMouseButtonClick)
	{
		auto& OnRightMouseButtonDown = StaticCastSharedPtr<SSDKButton>(MyButton)->OnRightMouseButtonDown;
		if (false == OnRightMouseButtonDown.IsBoundToObject(this))
		{
			OnRightMouseButtonDown.AddWeakLambda(this, [this]()
			{
				if ((PressedCheckSecondTime > 0.f && GetWorld()->GetTimeSeconds() >= PressedCheckStartTime + PressedCheckSecondTime))
				{
					if (bUseSameTime_ClickPressEvent == false)
					{
						return;
					}
				}
				OnClickedMouseRightButtonDown.Broadcast(ClickedWidgetParam);
			});
		}
	}
	return MyButton.ToSharedRef();
}

FReply USDKButtonParam::SlateHandleClickedParam()
{
 	if ((PressedCheckSecondTime > 0.f && GetWorld()->GetTimeSeconds() >= PressedCheckStartTime + PressedCheckSecondTime))
 	{
		if(bUseSameTime_ClickPressEvent == false)
		{
			return FReply::Handled();
		}
 	}

	if (OnClickedParam.IsBound())
		OnClickedParam.Broadcast(ClickedWidgetParam);

	if (OnClicked.IsBound())
		OnClicked.Broadcast();

	return FReply::Handled();
}

void USDKButtonParam::SlateHandlePressedParam()
{
	if (PressedCheckSecondTime > 0.f)
	{
		PressedCheckStartTime = GetWorld()->GetTimeSeconds();
		GetWorld()->GetTimerManager().SetTimer(TimerHendlerPressedCheck, FTimerDelegate::CreateUObject(this, &USDKButtonParam::SlateHandlePressedTimeCheckedParam), PressedCheckSecondTime, false);
	}

	if (OnPressedParam.IsBound())
		OnPressedParam.Broadcast(ClickedWidgetParam);

	if (OnPressed.IsBound())
		OnPressed.Broadcast();
}

void USDKButtonParam::SlateHandleReleasedParam()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHendlerPressedCheck);

	if (OnReleasedParam.IsBound())
		OnReleasedParam.Broadcast(ClickedWidgetParam);

	if (OnReleased.IsBound())
		OnReleased.Broadcast();
}

void USDKButtonParam::SlateHandleHoveredParam()
{
	if (OnHoveredParam.IsBound())
		OnHoveredParam.Broadcast(ClickedWidgetParam);

	if (OnHovered.IsBound())
		OnHovered.Broadcast();
}

void USDKButtonParam::SlateHandleUnhoveredParam()
{
	if (OnUnhoveredParam.IsBound())
		OnUnhoveredParam.Broadcast(ClickedWidgetParam);

	if (OnUnhovered.IsBound())
		OnUnhovered.Broadcast();
}

void USDKButtonParam::SlateHandlePressedTimeCheckedParam()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHendlerPressedCheck);

	if (OnPressedTimeCheckedParam.IsBound())
		OnPressedTimeCheckedParam.Broadcast(ClickedWidgetParam);
}

void USDKButtonParam::SetPressedTimeChecker(uint8 GlobalDefineKey)
{
	auto tableDataGD = USDKTableManager::Get()->FindTableGlobalDefine((EGlobalDefine)GlobalDefineKey);
	if (tableDataGD != nullptr)
	{
		SetPressedTimeChecker(FSDKHelpers::ArrayStringToFloat(0, tableDataGD->Value));
	}
}

void USDKButtonParam::SetPressedTimeChecker(float fPressTime)
{
	PressedCheckSecondTime = fPressTime;
}

void USDKButtonParam::SetUseSameTimeClickPressEvent(bool bUse)
{
	bUseSameTime_ClickPressEvent = bUse;
}

bool USDKButtonParam::IsPressed()
{
	if (MyButton != nullptr)
	{
		return MyButton->IsPressed();
	}

	return false;
}
