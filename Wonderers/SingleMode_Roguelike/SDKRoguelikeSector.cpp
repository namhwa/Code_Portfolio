// Fill out your copyright notice in the Description page of Project Settings.


#include "SingleMode_Roguelike/SDKRoguelikeSector.h"

#include "Components/ActorComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "LevelSequenceActor.h"

#include "Character/SDKHUD.h"
#include "Character/SDKPlayerState.h"
#include "Character/SDKPlayerCharacter.h"
#include "Character/SDKInGameController.h"

#include "GameMode/SDKRpgState.h"
#include "GameMode/SDKRpgGameMode.h"
#include "GameMode/SDKSectorLocation.h"

#include "Manager/SDKTableManager.h"
#include "Manager/SDKLobbyServerManager.h"
#include "Manager/SDKLevelSequenceManager.h"

#include "Object/SDKObject.h"
#include "Object/SDKSpawnComponent.h"

#include "Share/SDKHelper.h"

#include "UI/SDKGlitchDungeonMissionWidget.h"


ASDKRoguelikeSector::ASDKRoguelikeSector()
{
	PrimaryActorTick.bCanEverTick = true;

	ConditionValue = 0;
	PhaseCount = 0;
	bClearRoomEvent = true;
}

void ASDKRoguelikeSector::BeginPlay()
{
	Super::BeginPlay();

	if (OnCompletedSectorSettingEvent.IsBound() == false)
	{
		OnCompletedSectorSettingEvent.BindDynamic(this, &ASDKRoguelikeSector::CompletedInitSectorSetting);
	}
}

void ASDKRoguelikeSector::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if (IsValid(Cast<ASDKPlayerCharacter>(OtherActor)) == false)
	{
		return;
	}
	
	SectorOverlappedAcotr(OtherActor);
}

void ASDKRoguelikeSector::NotifyActorEndOverlap(AActor* OtherActor)
{
	if (IsValid(OtherActor) == false)
	{
		return;
	}

	ASDKPlayerCharacter* SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
	if(IsValid(SDKPlayerCharacter) == false)
	{
		return;
	}

	if (LevelPathName.Find(TEXT("Arcade")) > INDEX_NONE)
	{
		auto SDKPlayerController = SDKPlayerCharacter->GetSDKPlayerController();
		if (IsValid(SDKPlayerController) == true)
		{
			// 공격 막기
			SDKPlayerController->SetDisableAttackLevel(false);

			// 아케이드 전용 골드 표시 
			SDKPlayerController->ClientChangeGoldWidget(false);

			// 펫 숨기기
			SDKPlayerController->ClientSetPetHiddenInGame(false);
		}
	}
}

void ASDKRoguelikeSector::SetLifeSpan(float InLifespan)
{
	Super::SetLifeSpan(InLifespan);

	GetWorldTimerManager().ClearTimer(TimeAttackHandle);
	GetWorldTimerManager().ClearTimer(PlatformTimerHandle);

	PlatformRoomData.Clear();

	ClearObjectPhases();
}

bool ASDKRoguelikeSector::IsJumpTypeLevel()
{
	if(LevelName.IsEmpty() == true)
	{
		return false;
	}

	auto LevelTable = USDKTableManager::Get()->FindTableRow<FS_Level>(ETableType::tb_Level, LevelName);
	if(LevelTable == nullptr)
	{
		return false;
	}

	return LevelTable->ObjectGroup1.Num() <= 0 && LevelTable->WaveGroup1.Num() <= 0 && LevelTable->Rewards1.Num() <= 0;
}

void ASDKRoguelikeSector::SetLevelData(FRpgSectorData data)
{
	LevelName = data.LevelID;
	RewardID = data.RewardID;
	RoomIndex = data.RoomIndex;
	SectorType = data.RoomType;

	bIsRootSector = data.RoomIndex == 0;

	auto LevelTable = USDKTableManager::Get()->FindTableRow<FS_Level>(ETableType::tb_Level, LevelName);
	if (LevelTable != nullptr)
	{
		LevelPathName = LevelTable->Name;
		RoomEventIndex = LevelTable->EventSectorLocationIndex;

		if (LevelTable->EventRate > 0)
		{
			int32 EventChance = FMath::RandRange(1, 10000);
			if (EventChance <= LevelTable->EventRate)
			{
				SetSectorEvent(LevelName);
			}
		}
	}

	// 보스방 일 때, 레벨시퀀스를 가져와서 시네마틱 진행 후 보스 스폰위함.
	ASDKRpgGameMode* SDKRpgMode = GetWorld()->GetAuthGameMode<ASDKRpgGameMode>();
	if (IsValid(SDKRpgMode))
	{
		if (SectorType == TEXT("Boss"))
		{
			OnSetActiveCollision(false);

			UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), ALevelSequenceActor::StaticClass(), FName("Boss"), SequenceActors);
			if (SequenceActors.Num() > 0)
			{
				BossSequenceActor = Cast<ALevelSequenceActor>(SequenceActors[0]);
				IsLastSector = true;

				if (BossSequenceActor.IsValid() == true && IsValid(BossSequenceActor.Get()->GetSequencePlayer()) == true)
				{
					USDKLevelSequenceManager::Get()->SetLevelSequence(BossSequenceActor.Get());
					USDKLevelSequenceManager::Get()->OnFinishedSequence.AddDynamic(this, &ASDKRoguelikeSector::InitSectorSetting);
				}
			}
		}
	}
}

bool ASDKRoguelikeSector::SetCurrentSector()
{
	bool bSet = false;

	ASDKRpgGameMode* SDKRpgMode = GetWorld()->GetAuthGameMode<ASDKRpgGameMode>();
	if (IsValid(SDKRpgMode) == true)
	{
		//초기화된 섹터가 있으면
		//IsLevelClear() 전까지는 셋을 할수없다.
		ASDKRoguelikeSector* pCurSector = SDKRpgMode->GetRoguelikeSector();
		if (IsValid(pCurSector) == true)
		{
			if (pCurSector->IsInitSetting())
			{
				bSet = pCurSector->IsLevelClear();

				if (bSet)
				{
					bSet = this != pCurSector;
				}
			}
			else
			{
				bSet = true;
			}
		}
		else
		{
			bSet = true;
		}
	}

	if (bSet)
	{
		SDKRpgMode->SetCurrentSector(this, LevelName);
	}

	return bSet;
}

void ASDKRoguelikeSector::SetPlayingRoomPhase(bool bPlaying)
{
	ASDKRpgState* RPGState = GetWorld()->GetGameState<ASDKRpgState>();
	if(IsValid(RPGState) == false)
	{
		return;
	}

	RPGState->SetPlayingRoomPhase(bPlaying);

	if(bPlaying == false)
	{
		return;
	}

	RPGState->SetCurrentMapIndex(RoomIndex);
}

void ASDKRoguelikeSector::SetSectorLocation(ASDKSectorLocation* newObject)
{
	if (IsValid(newObject) == true)
	{
		SDKSectorLocation = newObject;

		PlatformRoomData = SDKSectorLocation->GetPlatformMissionData();
	}
	else
	{
		SDKSectorLocation = nullptr;
	}
}

void ASDKRoguelikeSector::IncreaseConditionValue(int32 value)
{
	ConditionValue += value;
}

void ASDKRoguelikeSector::AddObjectPhase(FPhaseActors newPhase)
{
	if(newPhase.Objects.Num() <= 0)
	{
		return;
	}

	ObjectPhases.Emplace(newPhase);

	for(auto& iter : ObjectPhases)
	{
		if(iter.Objects.Num() <= 0)
		{
			continue;
		}

		for(auto& itObject : iter.Objects)
		{
			if(IsValid(itObject) == false)
			{
				continue;
			}

			UActorComponent* pComponent = itObject->GetComponentByClass(USDKSpawnComponent::StaticClass());
			if(IsValid(pComponent) == false)
			{
				continue;
			}

			USDKSpawnComponent* pSpawn = Cast<USDKSpawnComponent>(pComponent);
			if(IsValid(pSpawn) == false || pSpawn->GetIsNpcType() == false)
			{
				continue;
			}

			pSpawn->SetActive(false, false);
		}
	}
}

void ASDKRoguelikeSector::ClearObjectPhases()
{
	if(ObjectPhases.Num() <= 0)
	{
		return;
	}

	for(auto& iter : ObjectPhases)
	{
		if(iter.Objects.Num() <= 0)
		{
			continue;
		}

		for(auto& itOB : iter.Objects)
		{
			if(IsValid(itOB) == false)
			{
				continue;
			}

			itOB->SetLifeSpan(0.01f);
		}

		iter.Objects.Empty();
	}

	ObjectPhases.Empty();
}

void ASDKRoguelikeSector::CheckPhase()
{
	--ConditionValue;

#if WITH_EDITOR
	UE_LOG(LogGame, Log, TEXT("[CheckPhase] %d"), ConditionValue);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, *FString::Printf(TEXT("[CheckPhase] %d"), ConditionValue));
#endif

	if (ConditionValue > 0)
	{
		return;
	}

	if (bInitSetting == false)
	{
		UE_LOG(LogGame, Log, TEXT("RPG not initialize!!"));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("RPG not initialize!!"), true, FVector2D(1.f));
		return;
	}
		
	if (bLevelClear == true)
	{
		UE_LOG(LogGame, Log, TEXT("RPG aleady clear!!"));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("RPG aleady clear!!"), true, FVector2D(1.f));
		return;
	}
		
	if (bLevelPlaying == false)
	{
		UE_LOG(LogGame, Log, TEXT("RPG not playing!!"));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("RPG not playing!!"), true, FVector2D(1.f));
		return;
	}

	bLevelPlaying = false;

	if(PhaseCount >= ObjectPhases.Num())
	{
		ClearPhase();
	}
	else
	{
		StartPhase();
	}
}

void ASDKRoguelikeSector::CompletedSetSectorObejctList()
{
	OnCompletedSectorSettingEvent.ExecuteIfBound();
}

void ASDKRoguelikeSector::CompletedInitSectorSetting()
{
	TArray<AActor*> OverlapedList;
	GetOverlappingActors(OverlapedList, ASDKPlayerCharacter::StaticClass());
	if (OverlapedList.Num() > 0)
	{
		SectorOverlappedAcotr(OverlapedList[0]);
	}
}

void ASDKRoguelikeSector::StartEventPhase()
{
	StartPhase();

	if (IsValid(SDKSectorLocation) == true)
	{
		SDKSectorLocation->OnStartPhase();
	}

	if (RoomEventType == ERoguelikeEventType::TimeAllKill)
	{
		EventTimeLimt++;
		GetWorldTimerManager().SetTimer(TimeAttackHandle, this, &ASDKRoguelikeSector::OnRoomEventTimeAttack, 1.f, true, 0.f);

		ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
		if (IsValid(SDKHUD) == true)
		{
			USDKGlitchDungeonMissionWidget* MissionWidget = Cast<USDKGlitchDungeonMissionWidget>(SDKHUD->OpenUI(EUI::GlitchDungeonMission));
			if (IsValid(MissionWidget) == true)
			{
				MissionWidget->StartCountDown();
			}
		}
	}
}

void ASDKRoguelikeSector::ClearEventPhase(bool bClear)
{
	if ((bClearRoomEvent == false && bClear == true) || RoomEventType == ERoguelikeEventType::None)
	{
		return;
	}

	bClearRoomEvent = bClear;

	// UI 표시
	if (bClear == true)
	{
		// RpgGameMode에 전달
		ASDKRpgState* RpgState = GetWorld()->GetGameState<ASDKRpgState>();
		if (IsValid(RpgState) == true)
		{
			RpgState->SetClearCurrentSectorEvent(bClearRoomEvent);
		}
	}

	if (RoomEventType == ERoguelikeEventType::TimeAllKill)
	{
		GetWorldTimerManager().ClearTimer(TimeAttackHandle);
	}
	else
	{
		ASDKPlayerCharacter* PlayerCharacter = Cast<ASDKPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if (IsValid(PlayerCharacter) == true)
		{
			if (RoomEventType == ERoguelikeEventType::NoHitAllKill)
			{
				PlayerCharacter->OnTakeAnyDamage.Remove(this, FName("OnTakeAnyDamage"));
			}
			else if (RoomEventType == ERoguelikeEventType::NoEvasionAllKill)
			{
				PlayerCharacter->OnUseDodgeEvent.Remove(this, FName("OnFailedEventAction"));
			}
			else if (RoomEventType == ERoguelikeEventType::NoSkillAllKill)
			{
				PlayerCharacter->OnUseSkillEvent.Remove(this, FName("OnFailedEventAction"));
			}
		}
	}

	ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
	if (IsValid(SDKHUD) == true)
	{
		USDKGlitchDungeonMissionWidget* MissionWidget = Cast<USDKGlitchDungeonMissionWidget>(SDKHUD->OpenUI(EUI::GlitchDungeonMission));
		if (IsValid(MissionWidget) == true)
		{
			MissionWidget->SetMissionClear(bClear);
		}
	}
}

void ASDKRoguelikeSector::OnRoomEventTimeAttack()
{
	EventTimeLimt--;

	if (EventTimeLimt == 0)
	{
		GetWorldTimerManager().ClearTimer(TimeAttackHandle);
		ClearEventPhase(false);
	}
}

void ASDKRoguelikeSector::OnTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (RoomEventType == ERoguelikeEventType::NoHitAllKill)
	{
		ISDKTypeInterface* DamageCauserType = Cast<ISDKTypeInterface>(DamageCauser);
		if (DamageCauserType != nullptr && DamageCauserType->GetActorType() == EActorType::Character)
		{
			ClearEventPhase(false);
		}
	}
	
	if (PlatformRoomData.bUseHitCount == true)
	{
		ISDKTypeInterface* DamageCauserType = Cast<ISDKTypeInterface>(DamageCauser);
		if (DamageCauserType != nullptr && DamageCauserType->GetActorType() == EActorType::Object)
		{
			--PlatformRoomData.MaxHitCount;

			if (PlatformRoomData.MaxHitCount <= 0)
			{
				ClearPlatformRoomEvent(false);
			}

			ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
			if (IsValid(SDKHUD) == true)
			{
				USDKGlitchDungeonMissionWidget* MissionWidget = Cast<USDKGlitchDungeonMissionWidget>(SDKHUD->OpenUI(EUI::GlitchDungeonMission));
				if (IsValid(MissionWidget) == true)
				{
					MissionWidget->SetMissionHpCount(PlatformRoomData.MaxHitCount);
				}
			}
		}
	}
}

void ASDKRoguelikeSector::OnFailedEventAction()
{
	ClearEventPhase(false);
}

void ASDKRoguelikeSector::StartPlatformRoomEvent()
{
	StartPhase();

	if (IsValid(SDKSectorLocation) == true)
	{
		SDKSectorLocation->OnStartPhase();
	}

	// 활성화 시켜야할 액터가 있는 경우
	if (PlatformRoomData.ActiveList.Num() > 0)
	{
		for (auto& itActor : PlatformRoomData.ActiveList)
		{
			ASDKObject* Object = Cast<ASDKObject>(itActor.Get());
			if (IsValid(Object) == true)
			{
				Object->SetActiveObject(true);
			}
		}
	}

	if (PlatformRoomData.bUseCountDown)
	{
		PlatformRoomData.CountDown++;
		GetWorldTimerManager().SetTimer(PlatformTimerHandle, this, &ASDKRoguelikeSector::OnPlatformEventTimeAttack, 1.f, true, 0.f);

		ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
		if (IsValid(SDKHUD) == true)
		{
			USDKGlitchDungeonMissionWidget* MissionWidget = Cast<USDKGlitchDungeonMissionWidget>(SDKHUD->OpenUI(EUI::GlitchDungeonMission));
			if (IsValid(MissionWidget) == true)
			{
				MissionWidget->StartCountDown();
			}
		}
	}
}

void ASDKRoguelikeSector::ClearPlatformRoomEvent(bool bClear)
{
	GetWorldTimerManager().ClearTimer(PlatformTimerHandle);

	// 활성화된 액터 비활성화
	if (PlatformRoomData.ActiveList.Num() > 0)
	{
		for (auto& itActor : PlatformRoomData.ActiveList)
		{
			ASDKObject* Object = Cast<ASDKObject>(itActor.Get());
			if (IsValid(Object) == true)
			{
				Object->SetActiveObject(false);
			}
		}
	}

	ASDKRpgState* RpgState = GetWorld()->GetGameState<ASDKRpgState>();
	if (IsValid(RpgState) == true)
	{
		RpgState->SetClearCurrentSectorEvent(bClear);
	}

	ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
	if (IsValid(SDKHUD) == true)
	{
		USDKGlitchDungeonMissionWidget* MissionWidget = Cast<USDKGlitchDungeonMissionWidget>(SDKHUD->OpenUI(EUI::GlitchDungeonMission));
		if (IsValid(MissionWidget) == true)
		{
			MissionWidget->SetMissionClear(bClear);
		}
	}

	ClearPhase();
}

void ASDKRoguelikeSector::OnPlatformEventTimeAttack()
{
	PlatformRoomData.CountDown--;

#if WITH_EDITOR
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, FString::FromInt(PlatformRoomData.CountDown), true, FVector2D(1.5f, 1.5f));
#endif

	if (PlatformRoomData.CountDown == 0)
	{
		GetWorldTimerManager().ClearTimer(PlatformTimerHandle);

		if (PlatformRoomData.GoalActor.IsValid() == true)
		{
			ClearPlatformRoomEvent(false);
		}
		else
		{
			ClearPlatformRoomEvent(true);
		}
	}
}

void ASDKRoguelikeSector::OnOverlapPlatformGoal(AActor* OverlappedActor, AActor* OtherActor)
{
	if (PlatformRoomData.bUseMissionCount == true)
	{
		--PlatformRoomData.MissionCount;

		ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
		if (IsValid(SDKHUD) == true)
		{
			USDKGlitchDungeonMissionWidget* MissionWidget = Cast<USDKGlitchDungeonMissionWidget>(SDKHUD->OpenUI(EUI::GlitchDungeonMission));
			if (IsValid(MissionWidget) == true)
			{
				MissionWidget->SetMissionCount(PlatformRoomData.MissionCount);
			}
		}

		if (PlatformRoomData.MovedActorList.Num() > 0)
		{
			if (PlatformRoomData.MovedActorList.Contains(OtherActor))
			{
				PlatformRoomData.MovedActorList.Remove(OtherActor);
			}
		}
	}

	// 미션카운트를 사용하지 않거나, 미션 카운트 조건에 맞았을 때
	if (PlatformRoomData.bUseMissionCount == false || PlatformRoomData.MissionCount <= 0)
	{
		if (PlatformRoomData.MovedActorTag.IsNone() == true)
		{
			if (OtherActor == UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
			{
				ClearPlatformRoomEvent(true);
			}
		}
		else
		{
			if (PlatformRoomData.bUseMissionCount == false)
			{
				// 미션 카운트를 사용하지 않는 경우
				if (PlatformRoomData.MovedActorList.Contains(OtherActor))
				{
					ClearPlatformRoomEvent(true);
				}
			}
			else
			{
				ClearPlatformRoomEvent(true);
			}
		}
	}
}

void ASDKRoguelikeSector::InitRootSectorSetting()
{
	bInitRootSetting = true;

	if (ObjectPhases.Num() <= 0)
	{
		// SDKObject 찾아서 설정
		UE_LOG(LogGame, Log, TEXT("Failed Object Phase Setting : %s"), *LevelName);

		if (PlatformRoomData.PlatformType != EPlatformRoomType::None)
		{
			TArray<AActor*> FindList;
			if (PlatformRoomData.ActiveActorTag.IsNone() == false)
			{
				UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), ASDKObject::StaticClass(), PlatformRoomData.ActiveActorTag, FindList);
				if (FindList.Num() > 0)
				{
					for (auto& itActor : FindList)
					{
						PlatformRoomData.ActiveList.Add(itActor);
					}
				}
			}

			FindList.Empty();
			if(PlatformRoomData.InactiveActorTag.IsNone() == false)
			{
				UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), ASDKObject::StaticClass(), PlatformRoomData.InactiveActorTag, FindList);
				if (FindList.Num() > 0)
				{
					for (auto& itActor : FindList)
					{
						PlatformRoomData.InactiveList.Add(itActor);
					}
				}
			}
		}
	}
	else
	{
		for (auto& itPhase : ObjectPhases)
		{
			if (itPhase.Objects.Num() <= 0)
			{
				continue;
			}

			for (auto& itObject : itPhase.Objects)
			{
				if (IsValid(itObject) == false)
				{
					continue;
				}

				UActorComponent* pComponent = itObject->GetComponentByClass(USDKSpawnComponent::StaticClass());
				if (IsValid(pComponent) == false)
				{
					continue;
				}

				USDKSpawnComponent* pSpawn = Cast<USDKSpawnComponent>(pComponent);
				if (IsValid(pSpawn) == false || pSpawn->GetIsNpcType() == false)
				{
					continue;
				}

				pSpawn->SetActive(false, true);
			}
		}
	}

	// 보스룸이여서 시퀀스가 있는 경우
	if (SectorType == FName("Boss") && USDKLevelSequenceManager::Get()->IsValidPlay())
	{
		USDKLevelSequenceManager::Get()->PlayLevelSequence();

		// 혹 Delegate가 실행안될 경우
		float fTime = BossSequenceActor.Get()->GetSequencePlayer()->GetEndTime().AsSeconds() + 0.5f;
		GetWorld()->GetTimerManager().SetTimer(SequenceTimerHandle, this, &ASDKRoguelikeSector::InitSectorSetting, fTime, false);

		ASDKInGameController* IngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		if (IsValid(IngameController) == true)
		{
			int32 BossID = FCString::Atoi(*BossSequenceActor.Get()->Tags.Last().ToString());
			IngameController->ClientBossSpawnStart(BossID);
		}

		UE_LOG(LogGame, Log, TEXT("Boss Sequence Play"));
	}
	else
	{
		InitSectorSetting();
	}
}

void ASDKRoguelikeSector::InitSectorSetting()
{
	GetWorld()->GetTimerManager().ClearTimer(SequenceTimerHandle);
	if (BossSequenceActor.IsValid())
	{
		if (IsValid(BossSequenceActor->GetSequencePlayer()))
		{
			if (BossSequenceActor->GetSequencePlayer()->IsPlaying())
			{
				GetWorld()->GetTimerManager().SetTimer(SequenceTimerHandle, this, &ASDKRoguelikeSector::InitSectorSetting, 0.1f, false);
				return;
			}
		}
	}

	if (bInitSetting == false)
	{
		bInitSetting = true;

		if (RoomEventType != ERoguelikeEventType::None)
		{
			InitEventMissionSetting();
		}
		else if (PlatformRoomData.PlatformType != EPlatformRoomType::None)
		{
			InitPlatformMissionSetting();
		}
		else
		{
			StartPhase();

			if (IsValid(SDKSectorLocation) == true)
			{
				SDKSectorLocation->OnStartPhase();
			}
		}
	}
}

void ASDKRoguelikeSector::InitEventMissionSetting()
{
	if (RoomEventType == ERoguelikeEventType::None)
	{
		StartEventPhase();
		return;
	}

	ASDKPlayerCharacter* PlayerCharacter = Cast<ASDKPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (IsValid(PlayerCharacter) == true)
	{
		if (RoomEventType == ERoguelikeEventType::NoHitAllKill)
		{
			PlayerCharacter->OnTakeAnyDamage.AddDynamic(this, &ASDKRoguelikeSector::OnTakeAnyDamage);
		}
		else if (RoomEventType == ERoguelikeEventType::NoEvasionAllKill)
		{
			PlayerCharacter->OnUseDodgeEvent.AddDynamic(this, &ASDKRoguelikeSector::OnFailedEventAction);
		}
		else if (RoomEventType == ERoguelikeEventType::NoSkillAllKill)
		{
			PlayerCharacter->OnUseSkillEvent.AddDynamic(this, &ASDKRoguelikeSector::OnFailedEventAction);
		}
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ASDKRoguelikeSector::StartEventPhase, 2.f, false);

	ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
	if (IsValid(SDKHUD) == true)
	{
		USDKGlitchDungeonMissionWidget* MissionWidget = Cast<USDKGlitchDungeonMissionWidget>(SDKHUD->OpenUI(EUI::GlitchDungeonMission));
		if (IsValid(MissionWidget) == true)
		{
			MissionWidget->SetMissionData(RoomEventType, EventTimeLimt);
		}
	}
}

void ASDKRoguelikeSector::InitPlatformMissionSetting()
{
	if (PlatformRoomData.PlatformType != EPlatformRoomType::None)
	{
		EPlatformRoomType Type = PlatformRoomData.PlatformType;
		if (Type == EPlatformRoomType::GoToGoal || Type == EPlatformRoomType::MoveItemToGoal)
		{
			// 목표 액터
			TArray<AActor*> FindGoals;
			UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), AActor::StaticClass(), PlatformRoomData.GoalActorTag, FindGoals);
			if (FindGoals.Num() > 0)
			{
				PlatformRoomData.GoalActor = FindGoals[0];
			}

			// 목표까지 옮겨야하는 액터
			TArray<AActor*> FindItems;
			UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), AActor::StaticClass(), PlatformRoomData.MovedActorTag, FindItems);
			if (FindItems.Num() > 0)
			{
				for (auto itItem : FindItems)
				{
					PlatformRoomData.MovedActorList.AddUnique(itItem);
				}
			}

			if (PlatformRoomData.GoalActor.IsValid() == true)
			{
				PlatformRoomData.GoalActor->OnActorBeginOverlap.AddDynamic(this, &ASDKRoguelikeSector::OnOverlapPlatformGoal);
			}
		}

		// Hit 에 대한 조건이 있는 경우
		if (PlatformRoomData.bUseHitCount == true)
		{
			ASDKPlayerCharacter* PlayerCharacter = Cast<ASDKPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
			if (IsValid(PlayerCharacter) == true)
			{
				PlayerCharacter->OnTakeAnyDamage.AddDynamic(this, &ASDKRoguelikeSector::OnTakeAnyDamage);
			}
		}

		// 비활성화 시켜야할 액터가 있는 경우
		if (PlatformRoomData.InactiveList.Num() > 0)
		{
			for (auto& itActor : PlatformRoomData.InactiveList)
			{
				ASDKObject* Object = Cast<ASDKObject>(itActor.Get());
				if (IsValid(Object) == true)
				{
					Object->SetActiveObject(false);
				}
			}
		}
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ASDKRoguelikeSector::StartPlatformRoomEvent, 2.f, false);

	ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
	if (IsValid(SDKHUD) == true)
	{
		USDKGlitchDungeonMissionWidget* MissionWidget = Cast<USDKGlitchDungeonMissionWidget>(SDKHUD->OpenUI(EUI::GlitchDungeonMission));
		if (IsValid(MissionWidget) == true)
		{
			MissionWidget->SetPlatformData(PlatformRoomData);
		}
	}
}

void ASDKRoguelikeSector::SectorOverlappedAcotr(AActor* OtherActor)
{
	if (IsValid(OtherActor) == false || bLevelPlaying == true)
	{
		return;
	}

	auto SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
	if (IsValid(SDKPlayerCharacter) == true)
	{
		bool bSet = SetCurrentSector();
		if (!bSet)
		{
			return;
		}

		bool bBattleRoom = LevelPathName.Find(TEXT("Arcade")) <= INDEX_NONE;

		if (ObjectPhases.Num() == 0)
		{
			if (bBattleRoom == false)
			{
				auto SDKPlayerController = SDKPlayerCharacter->GetSDKPlayerController();
				if (IsValid(SDKPlayerController) == true)
				{
					// 공격 막기
					SDKPlayerController->SetDisableAttackLevel(true);

					// 아케이드 전용 골드 표시 
					SDKPlayerController->ClientChangeGoldWidget(true);

					// 펫 숨기기
					SDKPlayerController->ClientSetPetHiddenInGame(true);

					// 소환수 제거
					SDKPlayerController->RemoveAllSummons();
				}
			}
		}

		if (bBattleRoom == true)
		{
			if (bInitRootSetting == false)
			{
				InitRootSectorSetting();
			}
			else
			{
				if (IsLastSector == false)
				{
					InitSectorSetting();
				}
			}

			auto SDKIngameController = SDKPlayerCharacter->GetSDKPlayerController();
			if (IsValid(SDKIngameController) == true)
			{
				SDKIngameController->ServerTeleportSummon();
			}
		}
	}
}

void ASDKRoguelikeSector::StartPhase()
{
#if WITH_EDITOR
	UE_LOG(LogGame, Log, TEXT("[StartPhase] LevelName : %s"), *GetLevelName());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, *FString::Printf(TEXT("[StartPhase] LevelName : %s"), *GetLevelName()));
#endif

	if (bLevelPlaying == true)
	{
		return;
	}

	ConditionValue = 0;
	bLevelPlaying = true;

	SetPlayingRoomPhase(true);

	int32 MaxPhase = ObjectPhases.Num();
	if(MaxPhase <= 0 && PlatformRoomData.PlatformType == EPlatformRoomType::None)
	{
		ClearPhase();
		return;
	}
	
	if (MaxPhase > 0)
	{
		for (int32 i = 0; i < MaxPhase; i++)
		{
			if (PhaseCount != i)
			{
				continue;
			}

			if (ObjectPhases[i].Objects.Num() > 0)
			{
				for (auto& iter : ObjectPhases[i].Objects)
				{
					UActorComponent* pComponent = iter->GetComponentByClass(USDKSpawnComponent::StaticClass());
					if (IsValid(pComponent) == true)
					{
						pComponent->SetActive(true, true);
						ConditionValue++;
					}

					iter->UseObject(nullptr);
				}
			}
		}

		PhaseCount++;
	}

	auto Controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (IsValid(Controller) == true)
	{
		// 특성
		if (SectorType == FName("Boss"))
		{
			auto SDKIngameController = Cast<ASDKInGameController>(Controller);
			if (SDKIngameController != nullptr)
			{
				SDKIngameController->ServerApplyAttributeAbility(EParameter::BossRoomHealRate);
			}
		}
	}

#if WITH_EDITOR
	UE_LOG(LogGame, Log, TEXT("[StartPhase] MaxCount : %d"), ConditionValue);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, *FString::Printf(TEXT("[StartPhase] MaxCount : %d"), ConditionValue));
#endif
}

void ASDKRoguelikeSector::ClearPhase()
{
#if WITH_EDITOR
	UE_LOG(LogGame, Log, TEXT("[ClearPhase]"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, TEXT("[ClearPhase]"));
#endif

	ASDKRpgState* RPGState = GetWorld()->GetGameState<ASDKRpgState>();
	if (IsValid(RPGState) == false)
	{
		return;
	}

	if (bLevelClear == true)
	{
		return;
	}

	ClearEventPhase(true);

	bLevelClear = true;
	RPGState->ClearSectorPhase();

	/*ClearPhaseReward();*/
	OnClearGuidImage();

	auto Controller = GEngine->GetFirstLocalPlayerController(GetWorld());
	if (IsValid(Controller) == true)
	{
		auto SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(Controller->GetPawn());
		if (IsValid(SDKPlayerCharacter) == true)
		{
			auto MapType = RPGState->GetCurrentMapType();
			if (MapType != TEXT("Start") && MapType != TEXT("Store") && MapType != TEXT("Blackmarket"))
			{
				SDKPlayerCharacter->UpdatePassiveData(EPassiveSkillType::ClearRoom);
			}

			// 특성
			auto SDKPlayerState = SDKPlayerCharacter->GetPlayerState<ASDKPlayerState>();
			if (IsValid(SDKPlayerState) == true)
			{
				float fAttributeValue = SDKPlayerState->GetAttAbilityRate(EParameter::ClearHealRate);
				if (fAttributeValue > 0.f)
				{
					float fChance = UKismetMathLibrary::RandomIntegerInRange(0, 100);
					if (fAttributeValue * 100 >= fChance)
					{
						FTransform SpawnTransform = FTransform();
						FVector Location = GetActorLocation();
						Location.Z = SDKPlayerCharacter->GetActorLocation().Z + 50.f;
						SpawnTransform.SetLocation(Location);

						// 체력 오브 스폰
						FSDKHelpers::SpawnMovingItem(GetWorld(), 170101, EItemType::Orb, SpawnTransform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
					}
				}
			}
		}
	}

	if (IsValid(SDKSectorLocation) == true)
	{
		SDKSectorLocation->OnClearPhase();
	}

	FTimerDelegate TimerCallback;
	TimerCallback.BindUFunction(this, FName("ClearRemainItems"));

	GetWorld()->GetTimerManager().SetTimerForNextTick(TimerCallback);
}

void ASDKRoguelikeSector::ClearPhaseReward()
{
	OnSpawnClearReward();

	GetWorld()->GetTimerManager().SetTimer(ClearTimerHandle, this, &ASDKRoguelikeSector::ClearRemainItems, 1.f, false);
}

void ASDKRoguelikeSector::ClearRemainItems()
{
	GetWorld()->GetTimerManager().ClearTimer(ClearTimerHandle);

	ASDKRpgState* SDKRpgState = GetWorld()->GetGameState<ASDKRpgState>();
	if (IsValid(SDKRpgState) == true)
	{
		SDKRpgState->CheckRemaindItems();
	}
}

void ASDKRoguelikeSector::SetSectorEvent(FString LevelID)
{
	auto LevelTable = USDKTableManager::Get()->FindTableRow<FS_Level>(ETableType::tb_Level, LevelName);
	if (LevelTable != nullptr)
	{
		TArray<ERoguelikeEventType> EventList = LevelTable->EventType;
		int32 EventCount = EventList.Num() - 1;

		for (int32 i = 0; i < EventList.Num(); ++i)
		{
			int32 Index = FMath::RandRange(i, EventCount);
			if (i != Index)
			{
				EventList.Swap(i, Index);
			}
		}

		int32 EventIndex = FMath::RandRange(0, EventCount);
		RoomEventType = EventList[EventIndex];

		EventTimeLimt = LevelTable->TimeAllKillLimit;
	}
}
