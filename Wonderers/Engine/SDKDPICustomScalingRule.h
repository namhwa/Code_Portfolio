// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DPICustomScalingRule.h"
#include "SDKDPICustomScalingRule.generated.h"

/**
 * 
 */
UCLASS(Config = Game)
class ARENA_API USDKDPICustomScalingRule : public UDPICustomScalingRule
{
	GENERATED_BODY()
	
public:
	virtual float GetDPIScaleBasedOnSize(FIntPoint Size) const override;

	float GetCustomDPIScaleValue() const { return DPIScalingValue; }
	float GetDefaultDPIScaleValue() const { return DefaultDPIScalingValue; }
	bool GetUseCustomScaling() const { return bUseCustomScalingRule; }

	void SetCustomDPIScaleValue(float fNewValue);
	void SetUseCustomScalingRule(bool bUse);

protected:
	float CalculateCustomScale(FIntPoint Size) const;

	void SaveDPIConfig();
	void LoadDPIConfig() const;

private:
	UPROPERTY(config)
	mutable bool bUseCustomScalingRule = false;

	UPROPERTY(config)
	mutable float DefaultDPIScalingValue = 1.f;

	UPROPERTY(config)
	mutable float DPIScalingValue = 0.f;
};