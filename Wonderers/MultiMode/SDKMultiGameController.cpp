// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/SDKMultiGameController.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/LevelScriptActor.h"
#include "Engine/LevelStreaming.h"
#include "Misc/OutputDeviceNull.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

#include "Character/SDKHUD.h"
#include "Character/SDKMyInfo.h"
#include "Character/SDKCharacter.h"
#include "Character/SDKPlayerState.h"
#include "Character/SDKPlayerCharacter.h"
#include "Character/SDKInGameController.h"
#include "Character/SDKSpringArmComponent.h"

#include "GameMode/SDKGameState.h"
#include "GameMode/SDKGoldClashMode.h"
#include "GameMode/SDKGoldClashState.h"

#include "Share/SDKData.h"
#include "Share/SDKEnum.h"

#include "UI/SDKStaminaWidget.h"
#include "UI/SDKMiniMapWidget.h"
#include "UI/SDKUIGoldClashMatchInfoWidget.h"

#include "UI/SDKUIReadyCountDownWidget.h"
#include "UI/SDKRightButtonsWidget.h"
#include "UI/SDKInGameMainUIInterface.h"
#include "UI/SDKUIGoldClashWidget.h"
#include "UI/SDKPopupSurrenderGameWidget.h"


void ASDKMultiGameController::ClientAttachHud_Implementation()
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetHUD());
	if (!IsValid(SDKHUD))
	{
		return;
	}

	SDKHUD->OnAttachHud();
}

void ASDKMultiGameController::ClientAttachBackgroundMainWidget()
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetHUD());
	if (!IsValid(SDKHUD))
	{
		return;
	}

	const USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (!IsValid(SDKGameInstance))
	{
		return;
	}

	if (IsValid(SDKHUD->GetModeMainUI()))
	{
		ClientActiveSpectator(false);
	}
	else
	{
		// 대사중인 경우, 반환
		if (SDKHUD->IsOpenUI(EUI::UI_ScriptScene))
		{
			return;
		}

		SDKHUD->OpenUI(EUI::TopBar);
		SDKHUD->OpenModeMainUI();

		ISDKInGameMainUIInterface* ModeMainWidget = Cast<ISDKInGameMainUIInterface>(SDKHUD->GetModeMainUI());
		if (ModeMainWidget != nullptr)
		{
			if (IsValid(ModeMainWidget->GetStaminaWidget()) == true)
			{
				ModeMainWidget->GetStaminaWidget()->SetOwningActor(GetPawn());
			}

			ModeMainWidget->InitHudReferenceSetting();
		}
	}

	// 특성에 의한 공짜 레벨업 버프 활성화
	ClientOpenLevelUpBuff(true, true);

	// 영웅 스킬 스택 초기화
	ClientUpdateGage(0.f);
}

void ASDKMultiGameController::ClientUpdateGoldClashRankerTransform_Implementation(FVector RankerLoc, int32 RankerSpawnNum, int32 RankerTeamNum)
{
	if (!IsValid(GetPawn()))
	{
		return;
	}

	if (GetSpawnNumber() == RankerSpawnNum)
	{
		ClientUpdateGoldClashRankerPositionAndRotation(true);
	}
	else
	{
		ASDKPlayerCharacter* SDKMyCharacter = Cast<ASDKPlayerCharacter>(GetPawn());
		if (IsValid(SDKMyCharacter))
		{
			int32 SizeX, SizeY;
			GetViewportSize(SizeX, SizeY);

			FVector2D vScreenRank = FVector2D::ZeroVector;
			FVector MyLocation = SDKMyCharacter->GetActorLocation();
			if (ProjectWorldLocationToScreen(RankerLoc, vScreenRank, true) && CheckTargetInMyViewport(vScreenRank, FVector2D(SizeX, SizeY)))
			{
				ClientUpdateGoldClashRankerPositionAndRotation(true);
			}
			else
			{
				float CameraRotation = SDKMyCharacter->GetCameraBoom()->GetComponentRotation().Yaw;

				FVector vProjected = (RankerLoc - MyLocation).GetSafeNormal2D();
				if (vProjected.IsNearlyZero())
				{
					ClientUpdateGoldClashRankerPositionAndRotation(true);
				}
				else
				{
					if (CameraRotation != 0.f)
					{
						vProjected = vProjected.RotateAngleAxis(-CameraRotation, FVector::UpVector);
					}	
					
					FVector2D HalfViewPortSize = FVector2D(SizeX / 3.5f, SizeY / 3.f);
					float fAbsX = FMath::Abs(vProjected.Y);
					float fAbsY = FMath::Abs(vProjected.X);
					float fDist = HalfViewPortSize.Y;

					while (1)
					{
						if (fAbsX * fDist <= 0.f || fAbsY * fDist <= 0.f)
						{
							break;
						}

						if ((fAbsX * fDist >= HalfViewPortSize.X) || (fAbsY * fDist >= HalfViewPortSize.Y))
						{
							break;
						}

						fDist += 1.f;
					}

					FVector2D vIconPosition = FVector2D(vProjected.Y, -vProjected.X) * FVector2D(fDist);

					float fTheta = FMath::Atan2(vProjected.Y, vProjected.X);
					float fDegree = FMath::RadiansToDegrees(fTheta);

					ClientUpdateGoldClashRankerPositionAndRotation(false, vIconPosition, fDegree);
				}
			}
		}
	}
}

void ASDKMultiGameController::ClientUpdateGoldRegenRegion_Implementation(const TArray<int32>& RegenIndex)
{
	if (RegenIndex.Num() <= 0)
	{
		return;
	}

	// 메세지 처리
	FString RegenString = TEXT("");
	for (auto itIndex = RegenIndex.CreateConstIterator(); itIndex; ++itIndex)
	{
		RegenString += FString::FromInt(*itIndex + 1);

		if (itIndex.GetIndex() < RegenIndex.Num() - 1)
		{
			RegenString += TEXT(", ");
		}
	}

	FSDKHelpers::GetTableText(TEXT("Common"), TEXT("Msg_GoldClash_ChangeGoldSpawnRegion"));

	ClientInstantMessageTableText(TEXT("Msg_GoldClash_ChangeGoldSpawnRegion"));	//RegenString

	// 미니맵 설정
	ASDKHUD* SDKHUD = GetHUD<ASDKHUD>();
	if (!IsValid(SDKHUD))
	{
		return;
	}
	USDKUIGoldClashWidget* ModeMainWidget = Cast<USDKUIGoldClashWidget>(SDKHUD->GetModeMainUI());
	if (IsValid(ModeMainWidget))
	{
		ModeMainWidget->UpdateGoldRegenRegion(RegenIndex);
	}
}

void ASDKMultiGameController::ClientChangeWaitingRoomUI_Implementation(EGameplayState eState)
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetHUD());
	if (IsValid(SDKHUD))
	{
		if (eState == EGameplayState::Ready)
		{
			SDKHUD->OpenUI(EUI::TopBar);
			SDKHUD->OpenUI(EUI::UI_ReadyCountDown);
		}
		else
		{
			ClientAttachBackgroundMainWidget();
		}
	}
}

void ASDKMultiGameController::ClientChangeIngameMainUI_Implementation()
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetHUD());
	if (!IsValid(SDKHUD))
	{
		return;
	}

	ClientAttachBackgroundMainWidget();
	ClientSetUIOpenEnable(true);		// 정보창 및 상점 버튼 활성/비활성화

	if (FSDKHelpers::GetGameMode(GetWorld()) == EGameMode::Raid)
	{
		USDKUIRaidWidget* ModeMainWidget = Cast<USDKUIRaidWidget>(SDKHUD->GetModeMainUI());
		if (IsValid(ModeMainWidget))
		{
			ModeMainWidget->SetIsEnabled(false);
			ModeMainWidget->SetActiveRaidModeIngame(true);
		}
	}
	else
	{
		ASDKPlayerState* SDKPlayerState = Cast<ASDKPlayerState>(PlayerState);
		if (IsValid(SDKPlayerState))
		{
			ClientUpdateGoldText(SDKPlayerState->GetGold());
		}
	}

	ClientInitQuickSlot(GetAllInventoryItem());
	ClientInitMiscItems(GetAllInventoryItem());

	// 인게임 HP, SP 초기화
	ASDKCharacter* MyCharacter = GetPawn<ASDKCharacter>();
	if (IsValid(MyCharacter))
	{
		ClientUpdateIngameInfoUIHp(MyCharacter->GetHp(), MyCharacter->GetPawnData()->MaxHp);
		ClientUpdateIngameInfoUISp(MyCharacter->GetSp(), MyCharacter->GetPawnData()->MaxSp);
		ClientSetHeroDodgeID(MyCharacter->GetTableID());
	}
}

void ASDKMultiGameController::ClientRespawnShopClose_Implementation()
{
	if (USDKPopupWidgetManager::From(this)->IsOpenedOrWillOpen(USDKGoldClashShopPopupWidget::StaticClass()))
	{
		USDKPopupWidgetManager::Get()->CloseAll(USDKGoldClashShopPopupWidget::StaticClass());
	}

	if (USDKPopupWidgetManager::Get()->IsOpenedOrWillOpen(USDKMacroPopupWidget::StaticClass()))
	{
		USDKPopupWidgetManager::Get()->CloseAll(USDKMacroPopupWidget::StaticClass());
	}
}

void ASDKMultiGameController::ClientInitMiscItems(const TArray<FItemSlotData>& InvenData)
{
	if (InvenData.Num() <= 0)
	{
		return;
	}

	for (int32 idx = (int32)EInventorySlot::Misc_Start; idx < (int32)EInventorySlot::Misc_End; idx++)
	{
		if (InvenData.IsValidIndex(idx))
		{
			int32 index = FMath::Max(0, idx - (int32)EInventorySlot::Misc_Start);
			ClientNotifySaveUseableItem(index, InvenData[idx].TableID, InvenData[idx].Amount);
		}
	}
}

void ASDKMultiGameController::ClientUpdateGoldClashRank_Implementation(const TArray<FGoldClashResultInfo>& RankingList)
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetHUD());
	if (IsValid(SDKHUD))
	{
		USDKUIGoldClashWidget* ModeMainWidget = Cast<USDKUIGoldClashWidget>(SDKHUD->GetModeMainUI());
		if (IsValid(ModeMainWidget))
		{
			if (IsValid(PlayerState) && !PlayerState->IsOnlyASpectator())
			{
				ModeMainWidget->UpdateGoldClashRank(RankingList, GetTeamNumber());
			}
			else
			{
				const ASDKPlayerCharacter* ViewTargetCharacter = Cast<ASDKPlayerCharacter>(GetViewTarget());
				if (IsValid(ViewTargetCharacter))
				{
					const ASDKPlayerState* ViewTargetPlayerState = Cast<ASDKPlayerState>(ViewTargetCharacter->GetPlayerState());
					if (IsValid(ViewTargetPlayerState))
					{
						ModeMainWidget->UpdateGoldClashRank(RankingList, 0);
					}
				}
			}
		}
	}
}

void ASDKMultiGameController::ClientUpdateGoldClashRankerPositionAndRotation_Implementation(bool bIsMe, FVector2D vPosition /*= FVector2D::ZeroVector*/, float fRotator /*= 0.0f*/)
{
	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetHUD());
	if (IsValid(SDKHUD))
	{
		USDKUIGoldClashWidget* ModeMainWidget = Cast<USDKUIGoldClashWidget>(SDKHUD->GetModeMainUI());
		if (IsValid(ModeMainWidget))
		{
			ModeMainWidget->UpdateGoldRankerPosition(bIsMe, vPosition, fRotator);
		}
	}
}

void ASDKMultiGameController::Client_ToggleState_Implementation()
{
	if (!bButtonUIOpenEnable)
	{
		return;
	}

	ASDKGoldClashState* SDKGameState = Cast<ASDKGoldClashState>(GetWorld()->GetGameState());
	if (!IsValid(SDKGameState))
	{
		return;
	}

	ASDKHUD* SDKHUD = Cast<ASDKHUD>(GetHUD());
	if (!IsValid(SDKHUD))
	{
		return;
	}

	const bool bMatchInfo = SDKHUD->IsOpenUI(EUI::UI_GoldClashMatchInfo);
	ISDKInGameMainUIInterface* ModeMainWidget = Cast<ISDKInGameMainUIInterface>(SDKHUD->GetModeMainUI());
	const bool bLevelUpBuff = ModeMainWidget && ModeMainWidget->GetVisibleyLevelUpCard();
	if (bMatchInfo || bLevelUpBuff)
	{
		// 상태창 또는 래밸업 버프가 열려 있다면
		return;
	}

	if (SDKGameState->GetGamePlayState() != EGameplayState::Playing)
	{
		return;
	}

	// 상태창 토글 - Close
	if (SDKHUD->IsOpenUI(EUI::UI_EquipInfo))
	{
		SDKHUD->CloseUI(EUI::UI_EquipInfo);
	}
	else
	{
		SDKHUD->OpenUI(EUI::UI_EquipInfo);
	}
}

void ASDKMultiGameController::Client_ChangeMatchInfoNo1_Implementation(int32 OldNo1, int32 CurNo1)
{
	OldRanker = OldNo1;
	CurRanker = CurNo1;
}

void ASDKMultiGameController::ClientUpdateChangeNo1Team_Implementation(int32 InTeamNum, bool bSpectator /*= false*/)
{
	ASDKHUD* SDKHUD = GetHUD<ASDKHUD>();
	if (IsValid(SDKHUD))
	{
		USDKUIGoldClashWidget* GoldClashWidget = Cast<USDKUIGoldClashWidget>(SDKHUD->GetModeMainUI());
		if (IsValid(GoldClashWidget))
		{
			if (bSpectator)
			{
				GoldClashWidget->UpdateNo1TeamSign(InTeamNum == 1);
			}
			else
			{
				GoldClashWidget->UpdateNo1TeamSign(InTeamNum == GetTeamNumber());
			}
		}
	}
}

void ASDKMultiGameController::UpdatePartyMemberData()
{
	if (!HasAuthority())
	{
		return;
	}

	TMap<int32, FPartyMemberData> MapRemain;

	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		auto MemberController = Cast<ASDKInGameController>(It->Get());
		if (!IsValid(MemberController) || MemberController->IsPendingKill())
		{
			continue;
		}

		if (MemberController == this || MemberController->GetTeamNumber() != GetTeamNumber())
		{
			continue;
		}

		ASDKPlayerState* MemberState = Cast<ASDKPlayerState>(MemberController->PlayerState);
		if (!IsValid(MemberState))
		{
			continue;
		}

		int32 MemberNumber = MemberState->GetSpawnNumber();
		for (auto& iter : PartyMemberData)
		{
			if (iter.SpawnNumber == MemberNumber)
			{
				MapRemain.Add(iter.SpawnNumber, iter);

				iter.Level = MemberState->GetLevel();
				ASDKCharacter* MemberCharacter = Cast<ASDKCharacter>(MemberController->GetPawn());
				if (IsValid(MemberCharacter))
				{
					iter.Hp = MemberCharacter->GetHp();
					iter.MaxHp = MemberCharacter->GetPawnData()->MaxHp;
					iter.Sp = MemberCharacter->GetSp();
					iter.MaxSp = MemberCharacter->GetPawnData()->MaxSp;
					iter.State = MemberCharacter->GetCurrentState();
					iter.Location = MemberCharacter->GetActorLocation();
					break;
				}
			}
		}
	}

	if (MapRemain.Num() > 0 && MapRemain.Num() != PartyMemberData.Num())
	{
		for (auto Iter = PartyMemberData.CreateIterator(); Iter; ++Iter)
		{
			if (MapRemain.Contains(Iter->SpawnNumber))
			{
				continue;
			}

			Iter.RemoveCurrent();
		}
	}

	ClientUpdatePartyMemberData(PartyMemberData);
}

void ASDKMultiGameController::ClientNotifyPenaltyInput()
{
	PopupOpen<USDKConfirmPopupWidget>()->InitWith<USDKConfirmPopupWidgetArg>([this](auto Arg)
	{
		Arg->TitleId = TEXT("UIText_Penalty_PenaltyStateText_Notice");
		Arg->MessageId = TEXT("UIText_PenaltyPopup_DodgeNotice");
	});

	UE_LOG(LogGame, Log, TEXT("Client Notify Penalty Input"));
}

void ASDKMultiGameController::ClientEnterToDodgeReconnect_Implementation()
{
	UE_LOG(LogGame, Log, TEXT("Client Enter To DodgeReconnect Level"));

	USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (IsValid(SDKGameInstance))
	{
		SDKGameInstance->OnEnterDodgeReconnect();
	}
}

void ASDKMultiGameController::ClientNotifyLeaveUser_Implementation(const FString& InNick, bool bVisibleButton)
{
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, TEXT("ClientNotifyLeaveUser"));
	
	FText TableText = FSDKHelpers::GetTableText(TEXT("Common"), TEXT("UIText_GoldClash_DodgePlayer"));
	FFormatNamedArguments FormatArg;
	FormatArg.Add(TEXT("NickName"), FText::FromString(InNick));

	float InstantMessageTime = 3.0f;
	FS_GlobalDefine* GlobalDefineTable = USDKTableManager::Get()->FindTableGlobalDefine(EGlobalDefine::GoldClashInstantMessageTime);
	if (GlobalDefineTable)
	{
		if (GlobalDefineTable->Value.IsValidIndex(0))
		{
			if (!GlobalDefineTable->Value[0].IsEmpty())
			{
				InstantMessageTime = FCString::Atoi(*GlobalDefineTable->Value[0]);
			}
		}
	}

	ClientInstantMessageText(FText::Format(TableText, FormatArg), InstantMessageTime);

	ASDKHUD* SDKHUD = GetHUD<ASDKHUD>();
	if (IsValid(SDKHUD))
	{
		USDKUIGoldClashWidget* MainWidget = Cast<USDKUIGoldClashWidget>(SDKHUD->GetModeMainUI());
		if (IsValid(MainWidget))
		{
			MainWidget->SetVisibilityButtonSurrender(bVisibleButton);
		}
	}
}

void ASDKMultiGameController::ClientNotifyDisconnectUser_Implementation(const FString& InNick)
{
	FText TableText = FSDKHelpers::GetTableText(TEXT("Common"), TEXT("Msg_GoldClash_Leave"));
	FFormatNamedArguments FormatArg;
	FormatArg.Add(TEXT("NickName"), FText::FromString(InNick));

	float InstantMessageTime = 3.0f;
	FS_GlobalDefine* GlobalDefineTable = USDKTableManager::Get()->FindTableGlobalDefine(EGlobalDefine::GoldClashInstantMessageTime);
	if (GlobalDefineTable)
	{
		if (GlobalDefineTable->Value.IsValidIndex(0))
		{
			if (!GlobalDefineTable->Value[0].IsEmpty())
			{
				InstantMessageTime = FCString::Atoi(*GlobalDefineTable->Value[0]);
			}
		}
	}

	ClientInstantMessageText(FText::Format(TableText, FormatArg), InstantMessageTime);
}

void ASDKMultiGameController::ClientNotifyReconnectUser_Implementation(const FString& InNick)
{
	FText TableText = FSDKHelpers::GetTableText(TEXT("Common"), TEXT("Msg_GoldClash_Reconnect"));
	FFormatNamedArguments FormatArg;
	FormatArg.Add(TEXT("NickName"), FText::FromString(InNick));

	float InstantMessageTime = 3.0f;
	FS_GlobalDefine* GlobalDefineTable = USDKTableManager::Get()->FindTableGlobalDefine(EGlobalDefine::GoldClashInstantMessageTime);
	if (GlobalDefineTable)
	{
		if (GlobalDefineTable->Value.IsValidIndex(0))
		{
			if (!GlobalDefineTable->Value[0].IsEmpty())
			{
				InstantMessageTime = FCString::Atoi(*GlobalDefineTable->Value[0]);
			}
		}
	}

	ClientInstantMessageText(FText::Format(TableText, FormatArg), InstantMessageTime);
}

void ASDKMultiGameController::ServerClickedSurrenderVote_Implementation()
{
	ASDKGoldClashState* GoldClashState = GetWorld()->GetGameState<ASDKGoldClashState>();
	if (IsValid(GoldClashState))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, FString::Printf(TEXT("Team Number: %d"), GetTeamNumber()));
		GoldClashState->NotifyOpenSurrenderVote(GetTeamNumber());
	}
}

void ASDKMultiGameController::ClientToggleSurrenderVote_Implementation(bool bOpen)
{
	ASDKHUD* SDKHUD = GetHUD<ASDKHUD>();
	if (IsValid(SDKHUD))
	{
		if (bOpen)
		{
			SDKHUD->OpenUI(EUI::Popup_SurrenderGame);

			USDKUIGoldClashWidget* MainWidget = Cast<USDKUIGoldClashWidget>(SDKHUD->GetModeMainUI());
			if (IsValid(MainWidget))
			{
				ASDKGoldClashState* SDKGoldClashState = GetWorld()->GetGameState<ASDKGoldClashState>();
				if (IsValid(SDKGoldClashState))
				{
					if (!SDKGoldClashState->IsCustomMatch())
					{
						MainWidget->SetVisibilityButtonSurrender(false);
					}
				}
			}
		}
		else
		{
			SDKHUD->CloseUI(EUI::Popup_SurrenderGame);
		}
	}
}

void ASDKMultiGameController::ServerSendSelectSurrenderVote_Implementation(bool bAccept)
{
	ASDKGameState* SDKGameState = GetWorld()->GetGameState<ASDKGameState>();
	if (IsValid(SDKGameState))
	{
		SDKGameState->UpdateSurrenderVote(GetTeamNumber(), GetSpawnNumber(), bAccept);
	}
}

void ASDKMultiGameController::ClientNotifyAllMemberSurrenderVote_Implementation()
{
	ASDKHUD* SDKHUD = GetHUD<ASDKHUD>();
	if (IsValid(SDKHUD))
	{
		USDKPopupSurrenderGameWidget* SurrenderWidget = Cast<USDKPopupSurrenderGameWidget>(SDKHUD->GetUI(EUI::Popup_SurrenderGame));
		if (IsValid(SurrenderWidget))
		{
			SurrenderWidget->UpdateAllVoteState();
		}
	}
}
