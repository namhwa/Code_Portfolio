// Fill out your copyright notice in the Description page of Project Settings.


#include "Engine/SDKDPICustomScalingRule.h"

#include "Curves/RichCurve.h"
#include "Curves/CurveFloat.h"
#include "CoreGlobals.h"
#include "Engine/UserInterfaceSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "UObject/ObjectMacros.h"

#include "Arena/Arena.h"
#include "Engine/SDKProject.h"
#include "Manager/SDKPopupWidgetManager.h"

#if !UE_BUILD_SHIPPING
// ui scale rule 출력 및 설정
static FAutoConsoleCommand CVarModifyUIScaleRule(
	TEXT("uiscalerule"),
	TEXT("Change current ui scale rule."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		UUserInterfaceSettings* UISettings = GetMutableDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass());
		if (false == IsValid(UISettings))
		{
			return;
		}
		
		if (Args.Num() > 0 && FCString::IsNumeric(*Args[0]))
		{
			UISettings->UIScaleRule = (EUIScalingRule)FCString::Atoi(*Args[0]);
			UE_LOG(LogArena, Warning, TEXT("UIScaleRule : %d"), UISettings->UIScaleRule);
		}
		
		USDKPopupWidgetManager::Get()->OpenToastWithText(FText::FromString(FString::Printf(TEXT("UIScaleRule %d"), UISettings->UIScaleRule)));	
	})
);

// ui design screen size 출력

#endif

float USDKDPICustomScalingRule::GetDPIScaleBasedOnSize(FIntPoint Size) const
{
	if (GConfig)
	{
		LoadDPIConfig();
	}

	if (bUseCustomScalingRule == true)
	{
		return DPIScalingValue;
	}
	else
	{
		DPIScalingValue = CalculateCustomScale(Size);
		if (DefaultDPIScalingValue != DPIScalingValue)
		{
			DefaultDPIScalingValue = DPIScalingValue;
		}
	}
	
	return DPIScalingValue;
}

void USDKDPICustomScalingRule::SetCustomDPIScaleValue(float fNewValue)
{
	DPIScalingValue = fNewValue;

	SaveDPIConfig();
}

void USDKDPICustomScalingRule::SetUseCustomScalingRule(bool bUse)
{
	bUseCustomScalingRule = bUse;

	SaveDPIConfig();
}

float USDKDPICustomScalingRule::CalculateCustomScale(FIntPoint Size) const
{
	UUserInterfaceSettings* UISettings = GetMutableDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass());
	if (IsValid(UISettings))
	{
		int32 EvalPoint = FMath::Min(Size.X, Size.Y);
		const FRichCurve* DPICurve = UISettings->UIScaleCurve.GetRichCurveConst();

		DefaultDPIScalingValue = DPICurve->Eval((float)EvalPoint, 1.0f);
		DefaultDPIScalingValue = FMath::Max(DefaultDPIScalingValue * UISettings->ApplicationScale, 0.01f);
	}

	return DefaultDPIScalingValue;
}

void USDKDPICustomScalingRule::SaveDPIConfig()
{
	SaveConfig(CPF_Config, *GGameIni);
}

void USDKDPICustomScalingRule::LoadDPIConfig() const
{
	if (GConfig)
	{
		GConfig->GetBool(TEXT("/Script/Arena.SDKDPICustomScalingRule"), TEXT("bUseCustomScalingRule"), bUseCustomScalingRule, GGameIni);
		GConfig->GetFloat(TEXT("/Script/Arena.SDKDPICustomScalingRule"), TEXT("DefaultDPIScalingValue"), DefaultDPIScalingValue, GGameIni);
		GConfig->GetFloat(TEXT("/Script/Arena.SDKDPICustomScalingRule"), TEXT("DPIScalingValue"), DPIScalingValue, GGameIni);
	}
}