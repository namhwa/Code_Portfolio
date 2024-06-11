// Fill out your copyright notice in the Description page of Project Settings.


#include "TutorialMode/SDKRpgTutorialState.h"

#include "GameMode/SDKRpgTutorialGameMode.h"
#include "GameMode/SDKRoguelikeManager.h"


void ASDKRpgTutorialState::ClearSectorPhase()
{
	if (!IsGameActive() && !IsOpenLevelUpBuff())
	{
		return;
	}

	UE_LOG(LogGame, Log, TEXT("  RPG Stage : Completed Room Clear"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("RPG Stage : Completed Room Clear"));

	ASDKRpgTutorialGameMode* RpgTutorialMode = GetWorld()->GetAuthGameMode<ASDKRpgTutorialGameMode>();
	if (IsValid(RpgTutorialMode) == true)
	{
		CheckRemaindItems();

		SetPlayingRoomPhase(false);

		int32 CurIndex = RpgTutorialMode->GetRoguelikeManager()->GetCurrentIndex() + 1;
		int32 MaxIndex = RpgTutorialMode->GetRoguelikeManager()->GetSectorCount();

		// 룸 클리어
		if (CurIndex >= MaxIndex)
		{
			RpgTutorialMode->ActiveRoomReward(MaxIndex > 0);
			RpgTutorialMode->GetRoguelikeManager()->ClearRoguelikeSectorList();

			if (CurrentMapType == TEXT("Boss"))
			{
				CheckRpgGame(true);
			}
		}

		if (CurrentMapType == TEXT("Boss"))
		{
			RpgTutorialMode->SetClearRoom(true);
		}
	}
}
