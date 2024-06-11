// Fill out your copyright notice in the Description page of Project Settings.


#include "Quest/SDKQuest.h"

#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/TargetPoint.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

#include "DialogueBuilderObject.h"

#include "AI/SDKAIController.h"
#include "Character/SDKHUD.h"
#include "Character/SDKCharacter.h"
#include "Character/SDKPlayerCharacter.h"
#include "Character/SDKNonePlayerCharacter.h"
#include "Character/SDKPlayerController.h"

#include "Engine/SDKGameInstance.h"
#include "Engine/SDKAssetManager.h"
#include "GameMode/SDKRpgState.h"
#include "Item/SDKNormalItem.h"
#include "Item/SDKMovingItem.h"

#include "Manager/SDKQuestManager.h"
#include "Manager/SDKTableManager.h"
#include "Manager/SDKLobbyServerManager.h"
#include "Manager/SDKLevelSequenceManager.h"

#include "Object/SDKObject.h"
#include "Share/SDKData.h"
#include "Share/SDKDataTable.h"
#include "Share/SDKHelper.h"

#include "UI/SDKUserWidget.h"
#include "UI/SDKNarrationWidget.h"
#include "UI/SDKUIScriptSceneWidget.h"
#include "UI/SDKMediaPlayerWidget.h"
#include "UI/SDKPopupProductionWidget.h"


ASDKQuest::ASDKQuest()
	: bUseDefaultTrigger(true)
	, bIsAutoStartDirecting(false)
	, QuestActorType(EActorType::None)
	, QuestActorID(0)
	, bIsAlwaysSpawn(false)
	, bDestoryActorEndDirectiong(false)
	, QuestID(0)
	, QuestMissionID(0)
	, QuestState(EQuestState::None)
	, DirectingIndex(0)
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Tags.Add(FName("Quest"));

	// 컴포넌트 설정
	DefaultSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("DefautlComponent"));
	if (IsValid(DefaultSphereComponent))
	{
		DefaultSphereComponent->SetSphereRadius(150.f);
		DefaultSphereComponent->SetMobility(EComponentMobility::Movable);
		DefaultSphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		DefaultSphereComponent->SetCollisionObjectType(COLLISION_OBJECT);
		DefaultSphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		DefaultSphereComponent->SetCollisionResponseToChannel(COLLISION_CHARACTER, ECR_Overlap);

		if (DefaultSphereComponent->OnComponentBeginOverlap.Contains(this, FName("NotifyQuestBeginOverlap")) == false)
		{
			DefaultSphereComponent->OnComponentBeginOverlap.AddDynamic(this, &ASDKQuest::NotifyQuestBeginOverlap);
		}

		if (DefaultSphereComponent->OnComponentEndOverlap.Contains(this, FName("NotifyQuestEndOverlap")) == false)
		{
			DefaultSphereComponent->OnComponentEndOverlap.AddDynamic(this, &ASDKQuest::NotifyQuestEndOverlap);
		}

		SetRootComponent(DefaultSphereComponent);
	}

	DirectionComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComp"));
	if (IsValid(DirectionComponent))
	{
		DirectionComponent->ArrowSize = 2.f;
		DirectionComponent->ArrowColor = FColor::Yellow;
		DirectionComponent->SetupAttachment(RootComponent);
	}

	DefaultTriggerComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox_0"));
	if (IsValid(DefaultTriggerComponent))
	{
		DefaultTriggerComponent->SetMobility(EComponentMobility::Movable);
		DefaultTriggerComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		DefaultTriggerComponent->SetCollisionObjectType(COLLISION_OBJECT);
		DefaultTriggerComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		DefaultTriggerComponent->SetCollisionResponseToChannel(COLLISION_CHARACTER, ECR_Overlap);

		DefaultTriggerComponent->SetupAttachment(RootComponent);
		TriggerBoxList.Add(DefaultTriggerComponent);
	}

	ParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleComponent"));
	if (IsValid(ParticleComponent))
	{
		ParticleComponent->SetupAttachment(RootComponent);
		ParticleComponent->SetTemplate(nullptr);
		ParticleComponent->SetVisibility(false);
		ParticleComponent->SetHiddenInGame(true);
	}

	DirectingList.Empty();

	LevelSequenceList.Empty();
	ActionDataList.Empty();

	TriggerBoxList.Empty();
}

void ASDKQuest::BeginPlay()
{
	Super::BeginPlay();
	
	InitQuestActor();

	if(bIsAlwaysSpawn)
	{
		TArray<FString> AssetNameList;
		GetName().ParseIntoArray(AssetNameList, TEXT("_"));

		if (AssetNameList.Num() >= 3)
		{
			if (AssetNameList.Find(TEXT("Ending")) > INDEX_NONE)
			{
				SetQuestState(EQuestState::End);
			}
			else
			{
				int32 index = AssetNameList.Num() - 3;
				SetQuestState(static_cast<EQuestState>(FSDKHelpers::StringToEnum(TEXT("EQuestState"), AssetNameList[index])));
			}
		}

		SpawnQuestActor();
		GetWorldTimerManager().SetTimer(SpawnHandle, this, &ASDKQuest::FinishSpawnQuestActor, 0.5f, false);
	}

	if (bIsAutoStartDirecting)
	{
		bool bHoldDirecting = false;

		// 자동 시작 전, 실행중인 시퀀스 여부
		TArray<AActor*> ActorList;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALevelSequenceActor::StaticClass(), ActorList);
		if (ActorList.Num() > 0)
		{
			for (auto iter : ActorList)
			{
				ALevelSequenceActor* itSequence = Cast<ALevelSequenceActor>(iter);
				if (!IsValid(itSequence) || !IsValid(itSequence->GetSequencePlayer()))
				{
					continue;
				}

				if (itSequence->GetSequencePlayer()->IsPlaying() == true)
				{
					bHoldDirecting = true;
					if (itSequence->GetSequencePlayer()->OnFinished.Contains(this, FName("StartDirectiongList")) == false)
					{
						itSequence->GetSequencePlayer()->OnFinished.AddDynamic(this, &ASDKQuest::StartDirectiongList);
					}
				}
			}
		}

		if(!bHoldDirecting)
		{
			StartDirectiongList();
		}
	}
}

void ASDKQuest::SetLifeSpan(float InLifespan)
{
	Super::SetLifeSpan(InLifespan);
}

void ASDKQuest::LifeSpanExpired()
{
	ResetQuestActor();

	if (SpawnedActor.IsValid())
	{
		SpawnedActor.Get()->Destroy();
	}

	USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (IsValid(SDKGameInstance))
	{
		SDKGameInstance->QuestManager->RemoveQuestActor(this);
	}

	Super::LifeSpanExpired();
}

/** 퀘스트 액터 이벤트 */
void ASDKQuest::NotifyQuestBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != UGameplayStatics::GetPlayerCharacter(GetWorld(), 0) || !HasAuthority())
	{
		return;
	}

	if (QuestMissionID > 0 && !(QuestState == EQuestState::Progressing || QuestState == EQuestState::Available))
	{
		return;
	}

	if ((DirectingList.Num() <= 0 && QuestState != EQuestState::Progressing) || !SpawnedActor.IsValid())
	{
		return;
	}

	ASDKPlayerCharacter* SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
	if (!IsValid(SDKPlayerCharacter) || SDKPlayerCharacter->IsDie() || SDKPlayerCharacter->IsGroggy())
	{
		return;
	}

	if (FSDKHelpers::GetGameMode(GetWorld()) == EGameMode::Rpg)
	{
		ASDKRpgState* SDKRpgState = GetWorld()->GetGameState<ASDKRpgState>();
		if (IsValid(SDKRpgState) && SDKRpgState->GetPlayingRoomPhase())
		{
			return;
		}
	}
	
	ASDKPlayerController* SDKPlayerController = SDKPlayerCharacter->GetController<ASDKPlayerController>();
	if (IsValid(SDKPlayerController))
	{
		SDKPlayerController->SetActiveQuestNPC(this);
	}
}

void ASDKQuest::NotifyQuestEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor != UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
	{
		return;
	}

	if (QuestMissionID > 0 && !(QuestState == EQuestState::Progressing || QuestState == EQuestState::Available))
	{
		return;
	}

	ASDKPlayerCharacter* SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
	if (!IsValid(SDKPlayerCharacter))
	{
		return;
	}

	ASDKPlayerController* SDKPlayerController = SDKPlayerCharacter->GetController<ASDKPlayerController>();
	if (IsValid(SDKPlayerController))
	{
		SDKPlayerController->SetActiveQuestNPC(nullptr);
	}
}

/** 설정 : 활성화된 퀘스트 아이디 */
void ASDKQuest::SetQuestID(int32 ID)
{
	QuestID = ID;

	if(QuestID > 0)
	{
		bUseDefaultTrigger = false;
		DefaultTriggerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ASDKQuest::SetQuestMissionID(int32 NewID)
{
	QuestMissionID = NewID;
}

void ASDKQuest::SetQuestState(EQuestState NewState)
{
	QuestState = NewState;

	// 퀘스트 상태에 따른 파티클 설정
	if (QuestID <= 0 && QuestMissionID <= 0)
	{
		return;
	}

	// 진행중 상태 && 자동 실행인 경우 (조건 모순 : 액터 리셋과 자동 실행 취소)
	if (NewState == EQuestState::Progressing && bIsAutoStartDirecting)
	{
		bIsAutoStartDirecting = false;
		ResetQuestActor();
	}

	if (IsValid(ParticleComponent))
	{
		if (QuestParticle.Contains(NewState))
		{
			if (IsValid(QuestParticle[NewState]))
			{
				ParticleComponent->SetTemplate(QuestParticle[NewState]);
				ParticleComponent->SetHiddenInGame(false);
			}
			else
			{
				ParticleComponent->SetHiddenInGame(true);
			}
		}
	}
}

/** 트리거 이벤트 */
void ASDKQuest::OnBeginOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ASDKPlayerCharacter* SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
	if (!IsValid(SDKPlayerCharacter))
	{
		return;
	}

	if (OtherActor != UGameplayStatics::GetPlayerCharacter(GetWorld(), 0) || !HasAuthority())
	{
		return;
	}

	if (QuestID > 0 && QuestState == EQuestState::End)
	{
		return;
	}

	if (DirectingList.Num() <= 0 && QuestState != EQuestState::Progressing)
	{
		return;
	}

	if (DirectingList.Num() <= 0 && QuestState == EQuestState::Progressing)
	{
		SetNotClearScriptID();
	}
	else
	{
		if (IsValid(OverlappedComponent))
		{
			UBoxComponent* TriggerComponent = Cast<UBoxComponent>(OverlappedComponent);
			if (IsValid(TriggerComponent))
			{
				TriggerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}

		StartDirectiongList();
	}
}

void ASDKQuest::OnEndOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor != UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
	{
		return;
	}

	if (QuestID > 0 && QuestState == EQuestState::End)
	{
		return;
	}

	if (DirectingList.Num() <= 0)
	{
		return;
	}

	ASDKPlayerCharacter* SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
	if (!IsValid(SDKPlayerCharacter))
	{
		return;
	}

	if (IsValid(OverlappedComponent))
	{
		UBoxComponent* TriggerComponent = Cast<UBoxComponent>(OverlappedComponent);
		if (IsValid(TriggerComponent))
		{
			TriggerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

/** 연출 시작 */
void ASDKQuest::StartDirectiongList()
{
	if (DirectingList.Num() <= 0)
	{
		UE_LOG(LogGame, Log, TEXT("Empty : Directing List "));

		// 바로 완료 처리
		EndSelectDirecting();
		return;
	}

	UE_LOG(LogGame, Log, TEXT("Start : Directing List "));

	DirectingIndex = 0;
	ActiveDirectingData(DirectingList[0]);
}

/** 인터렉션 */
void ASDKQuest::InteractionPlayer()
{
	// 진행중인 퀘스트인 경우
	if (QuestState == EQuestState::Progressing)
	{
		ResetQuestActor();

		auto QuestMissionTable = USDKTableManager::Get()->FindTableRow<FS_QuestMission>(ETableType::tb_QuestMission, FString::FromInt(QuestMissionID));
		if (QuestMissionTable != nullptr)
		{
			if (!QuestMissionTable->NotClearScriptPath.IsEmpty() && QuestMissionTable->NotClearScriptPath != TEXT("0"))
			{
				DialogueList.Add(QuestMissionTable->NotClearScriptPath);
				DialogueTypeList.Add(false);

				DirectingList.Add(FDirectingData(EDirectingType::Dialogue));
			}
			else
			{
				UE_LOG(LogGame, Log, TEXT("Not Found Script ID : %s"), *QuestMissionTable->NotClearScriptPath);
			}
		}
	}

	StartDirectiongList();
}

/** 카메라 페이드 아웃 */
void ASDKQuest::StartCameraFade(float Duration)
{
	ASDKPlayerController* SDKPlayerController = Cast<ASDKPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKPlayerController) == true)
	{
		if (IsValid(SDKPlayerController->PlayerCameraManager) == true)
		{
			if (Duration > 0.f)
			{
				SDKPlayerController->PlayerCameraManager->StartCameraFade(1.0f, 0.f, Duration, FLinearColor::White);
			}
			else
			{
				SDKPlayerController->PlayerCameraManager->StopCameraFade();
			}
		}
	}
}

void ASDKQuest::SetQuestActorTableID()
{
	auto QActorTable = USDKTableManager::Get()->FindTableRow<FS_QuestActor>(ETableType::tb_QuestActor, QuestActorTableID);
	if (QActorTable == nullptr)
	{
		UE_LOG(LogGame, Warning, TEXT("Not Found Quest Actor TableData : %s"), *QuestActorTableID);
		return;
	}

	bIsAutoStartDirecting = QActorTable->IsAutoStart;
	bUseDefaultTrigger = QActorTable->UseDefaultTrigger;
	DirectingList = QActorTable->Directing;
	ActionDataList = QActorTable->Action;

	DialogueList = QActorTable->Dialogue;
	DialogueTypeList = QActorTable->IsNarration;

	LevelSequenceList = QActorTable->Sequence;

	MediaPlayTableIDs = QActorTable->Media;

	QuestSpawnActor = QActorTable->SpawnActor;
}

/** 초기화 : 시작할 시점에 데이터 정리 */
void ASDKQuest::InitQuestActor()
{
	if (IsValid(RootComponent) == false)
	{
		return;
	}

	GetComponents(TriggerBoxList);
	TriggerBoxList.Insert(DefaultTriggerComponent, 0);

	if (TriggerBoxList.Num() > 0)
	{
		for (auto itBox : TriggerBoxList)
		{
			if (!itBox->OnComponentBeginOverlap.Contains(this, FName("OnBeginOverlapComponent")))
			{
				itBox->OnComponentBeginOverlap.AddDynamic(this, &ASDKQuest::OnBeginOverlapComponent);
			}

			if (!itBox->OnComponentEndOverlap.Contains(this, FName("OnEndOverlapComponent")))
			{
				itBox->OnComponentEndOverlap.AddDynamic(this, &ASDKQuest::OnEndOverlapComponent);
			}
		}
	}

	if(bUseTableID)
	{
		if (!QuestActorTableID.IsEmpty())
		{
			SetQuestActorTableID();
		}
	}
}

void ASDKQuest::ResetQuestActor()
{
	if (TriggerBoxList.Num() > 0)
	{
		for (auto itBox : TriggerBoxList)
		{
			if (IsValid(itBox))
			{
				itBox->DestroyComponent();
			}
		}

		TriggerBoxList.Empty();
	}

	DirectingList.Empty();
	ActionDataList.Empty();
	QuestSpawnActor.Empty();

	if (DialogueList.Num() > 0)
	{
		ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
		if (IsValid(SDKHUD))
		{
			USDKNarrationWidget* NarrationWidget = Cast<USDKNarrationWidget>(SDKHUD->GetUI(EUI::Narration));
			if (IsValid(NarrationWidget))
			{
				if (NarrationWidget->NofityCloseNarrationEvent.Contains(this, FName("FinishDialogue")))
				{
					NarrationWidget->NofityCloseNarrationEvent.RemoveDynamic(this, &ASDKQuest::FinishDialogue);
				}
			}
			
			USDKUIScriptSceneWidget* ScriptWidget = Cast<USDKUIScriptSceneWidget>(SDKHUD->GetUI(EUI::UI_ScriptScene));
			if (IsValid(ScriptWidget))
			{
				if (ScriptWidget->NofityCloseScriptEvent.Contains(this, FName("FinishDialogue")))
				{
					ScriptWidget->NofityCloseScriptEvent.RemoveDynamic(this, &ASDKQuest::FinishDialogue);
				}
			}
		}

		DialogueList.Empty();
		DialogueTypeList.Empty();
	}
}

/** 마지막 연출목록인지 확인 */
void ASDKQuest::CheckEndLastDirecting()
{
	FDirectingData PrevData = FDirectingData();
	if (QuestMissionID > 0 && DirectingList.Num() > 0)
	{
		PrevData = DirectingList[0];
		DirectingList.RemoveAt(0);
	}

	// Quest End Actor 타입이거나 연출 목록이 다 실행된 경우 마무리
	if (DirectingList.Num() <= 0 || (QuestMissionID <= 0 && DirectingList.Num() == DirectingIndex))
	{
		EndSelectDirecting();
	}
	else
	{
		if (PrevData.bIsAutoLinkNext)
		{
			int32 index = QuestMissionID > 0 ? 0 : DirectingIndex;
			ActiveDirectingData(DirectingList[index]);
		}
	}
}

/** 연출 끝 */
void ASDKQuest::EndSelectDirecting()
{
	if (QuestMissionID > 0)
	{
		USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
		if (IsValid(SDKGameInstance))
		{
			if (QuestState == EQuestState::Available)
			{
				FS_QuestMission* QMissionTable = USDKTableManager::Get()->FindTableRow<FS_QuestMission>(ETableType::tb_QuestMission, FString::FromInt(QuestMissionID));
				if (QMissionTable != nullptr && QMissionTable->ServerAutoActivate == false)
				{
					UE_LOG(LogTemp, Log, TEXT("ASDKQuest::EndSelectDirecting() - CQ_QuestMissionStart(%d)"), QuestMissionID);
					SDKGameInstance->LobbyServerManager->CQ_QuestMissionStart(QuestMissionID);
				}
			}
			else if (QuestState == EQuestState::Condition)
			{
				SDKGameInstance->QuestManager->CheckProgressingQuestMissionData(EQuestMissionType::Script, bUseTableID && QuestSpawnActor.Num() > 0 ? QuestSpawnActor[0].SpawnID : QuestActorID);
			}
			else if (QuestState == EQuestState::Complete)
			{
				UE_LOG(LogTemp, Log, TEXT("ASDKQuest::EndSelectDirecting() - CQ_QuestMissionEnd(%d)"), QuestMissionID);
				SDKGameInstance->LobbyServerManager->CQ_QuestMissionEnd(QuestMissionID);
			}
			else if (QuestState == EQuestState::Progressing)
			{
				FS_QuestMission* QuestMissionTable = USDKTableManager::Get()->FindTableRow<FS_QuestMission>(ETableType::tb_QuestMission, FString::FromInt(QuestMissionID));
				if (QuestMissionTable != nullptr && QuestMissionTable->LinkUI != EUI::None)
				{
					SDKGameInstance->QuestManager->SetProgressingQuestMissionOperateUIType(QuestMissionID, QuestMissionTable->LinkUI);
				}
			}

			SDKGameInstance->QuestManager->EndQuestActorEventCallback.Broadcast();
			SDKGameInstance->QuestManager->RemoveDelegateHandle(QuestMissionID);
		}
	}

	if (bDestoryActorEndDirectiong)
	{
		SetLifeSpan(0.01f);
	}
	else
	{
		if (QuestID > 0)
		{
			DirectingIndex = 0;
			OpenContentsUI();
		}
		else
		{
			if (QuestState != EQuestState::Progressing)
			{
				ResetQuestActor();
			}
		}
	}

	// 바인딩 되어 있던 부분 삭제
	RemoveLevelSequenceBinding();
}

/** 진행중인 상태에서 이벤트가 나와야하는 경우(퀘스트를 준 액터가) */
void ASDKQuest::SetNotClearScriptID()
{
	if (DirectingList.Num() > 0 || QuestState != EQuestState::Progressing)
	{
		return;
	}

	auto QuestMissionTable = USDKTableManager::Get()->FindTableRow<FS_QuestMission>(ETableType::tb_QuestMission, FString::FromInt(QuestMissionID));
	if (QuestMissionTable != nullptr)
	{
		if (!QuestMissionTable->NotClearScriptPath.IsEmpty() && QuestMissionTable->NotClearScriptPath != TEXT("0"))
		{
			DialogueList.Add(QuestMissionTable->NotClearScriptPath);
			DialogueTypeList.Add(false);

			DirectingList.Add(FDirectingData(EDirectingType::Dialogue));

			StartDirectiongList();
		}
		else
		{
			UE_LOG(LogGame, Log, TEXT("Not Found Script ID : %s"), *QuestMissionTable->NotClearScriptPath);
		}
	}
}

/** 선택된 연출 활성화 */
void ASDKQuest::ActiveDirectingData(FDirectingData& DirectingData)
{
	if (DirectingData.DirectingType == EDirectingType::None)
	{
		CheckEndLastDirecting();
	}

	DirectingIndex++;

	bool bSuccess = false;
	ASDKPlayerController* SDKPlayerController = Cast<ASDKPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(IsValid(SDKPlayerController))
	{
		switch (DirectingData.DirectingType)
		{
		case EDirectingType::Dialogue:
		{
			if(DialogueList.Num() > 0)
			{
				bSuccess = true;
				if (DialogueTypeList.Num() > 0 && DialogueTypeList[0])
				{
					SDKPlayerController->ClientToggleNarrationTableID(true, DialogueList[0]);
				}
				else
				{
					SDKPlayerController->ClientToggleScriptSceneTableID(true, DialogueList[0]);
				}
			}
		}
		break;
		case EDirectingType::Sequence:
		{
			if(LevelSequenceList.Num() > 0)
			{
				bSuccess = true;
				PlayLevelSequence(LevelSequenceList[0]);
			}
		}
		break;
		case EDirectingType::Media:
		{
			if (MediaPlayTableIDs.Num() > 0)
			{
				bSuccess = true;
				PlayMediaPlayer(MediaPlayTableIDs[0]);
			}
		}
		break;
		case EDirectingType::Action:
		{
			if (ActionDataList.Num() > 0)
			{
				bSuccess = true;
				ActiveActionData(ActionDataList[0]);
				return;
			}
		}
		break;
		case EDirectingType::SpawnActor:
		{
			bSuccess = true;
			SpawnQuestActor();
			return;
		}
		break;
		case EDirectingType::Sequence_Dialogue:
		{
			if (LevelSequenceList.Num() > 0)
			{
				bSuccess = true;
				PlayLevelSequence(LevelSequenceList[0]);
			}

			if (DialogueList.Num() > 0)
			{
				if (DialogueTypeList.Num() > 0 && DialogueTypeList[0])
				{
					SDKPlayerController->ClientToggleNarrationTableID(true, DialogueList[0]);
				}
				else
				{
					SDKPlayerController->ClientToggleScriptSceneTableID(true, DialogueList[0]);
				}
			}
		}
		break;
		default:
			break;
		}

		if (DirectingData.DirectingType == EDirectingType::Dialogue || DirectingData.DirectingType == EDirectingType::Sequence_Dialogue)
		{
			if (DialogueList.Num() > 0)
			{
				bool bPlayingType = DialogueTypeList.Num() > 0 ? DialogueTypeList[0] : false;
				
				ASDKHUD* SDKHUD = SDKPlayerController->GetHUD<ASDKHUD>();
				if (IsValid(SDKHUD))
				{
					if (bPlayingType)
					{
						USDKNarrationWidget* NarrationWidget = Cast<USDKNarrationWidget>(SDKHUD->GetUI(EUI::Narration));
						if (IsValid(NarrationWidget))
						{
							if (NarrationWidget->NofityCloseNarrationEvent.Contains(this, FName("FinishDialogue")) == false)
							{
								NarrationWidget->NofityCloseNarrationEvent.AddDynamic(this, &ASDKQuest::FinishDialogue);
							}
						}
					}
					else
					{
						USDKUIScriptSceneWidget* ScriptWidget = Cast<USDKUIScriptSceneWidget>(SDKHUD->GetUI(EUI::UI_ScriptScene));
						if (IsValid(ScriptWidget))
						{
							if (ScriptWidget->NofityCloseScriptEvent.Contains(this, FName("FinishDialogue")) == false)
							{
								ScriptWidget->NofityCloseScriptEvent.AddDynamic(this, &ASDKQuest::FinishDialogue);
							}
						}
					}
				}
			}
		}

		if (!bSuccess)
		{
			CheckEndLastDirecting();
		}
	}
}

void ASDKQuest::ActiveActionData(FActionData& Data)
{
	if (Data.ActionType == EActionType::None)
	{
		return;
	}

	ASDKPlayerController* SDKPlayerController = Cast<ASDKPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKPlayerController))
	{
		switch (Data.ActionType)
		{
		case EActionType::Teleport:
		{
			StartCameraFade(0.5f);

			ASDKPlayerCharacter* SDKCharacter = SDKPlayerController->GetPawn<ASDKPlayerCharacter>();
			if (IsValid(SDKCharacter))
			{
				FVector TeleportPoint = Data.Location;
				if (FSDKHelpers::GetGameMode(GetWorld()) == EGameMode::Rpg)
				{
					TeleportPoint += GetActorLocation();
				}

				SDKCharacter->ServerBeginTeleport(TeleportPoint);
				SDKCharacter->MultiSetActorRotation(Data.Rotation);
			}
		}
		break;
		case EActionType::VisibleActor:
		{
			if (SpawnedActor.IsValid())
			{
				SpawnedActor->SetActorHiddenInGame(Data.bHidden);

				if (QuestActorType == EActorType::Character)
				{
					ASDKCharacter* SpawnedNpc = Cast<ASDKCharacter>(SpawnedActor.Get());
					if (IsValid(SpawnedNpc))
					{
						if (IsValid(SpawnedNpc->GetCapsuleComponent()))
						{
							SpawnedNpc->GetCapsuleComponent()->SetCollisionEnabled(Data.bHidden ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
						}
					}
				}
			}
		}
		break;
		case EActionType::VisibleParticleSign:
		{
			if (IsValid(ParticleComponent))
			{
				ParticleComponent->SetHiddenInGame(Data.bHidden);
			}
		}
		break;
		case EActionType::RotateCamera:
		{
			ASDKPlayerCharacter* SDKCharacter = SDKPlayerController->GetPawn<ASDKPlayerCharacter>();
			if (IsValid(SDKCharacter))
			{
				SDKPlayerController->SetControlRotation(Data.Rotation);
			}
		}
		default:
			break;
		}
	}

	if (ActionDataList.Num() > 0)
	{
		ActionDataList.RemoveAt(0);
	}

	CheckEndLastDirecting();
}

/** 대사 */
void ASDKQuest::FinishDialogue()
{
	if (DialogueList.Num() > 0 && QuestMissionID > 0)
	{
		DialogueList.RemoveAt(0);
		
		if (DialogueTypeList.Num() > 0)
		{
			DialogueTypeList.RemoveAt(0);
		}
	}

	if (DirectingList.Num() > 0 && DirectingList[0].DirectingType != EDirectingType::Sequence_Dialogue)
	{
		CheckEndLastDirecting();
	}
	else
	{
#if PLATFORM_ANDROID || PLATFORM_IOS
		ASDKHUD* SDKHUD = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD<ASDKHUD>();
		if (IsValid(SDKHUD) == true)
		{
			SDKHUD->SetVisibilityJoystick(false);
		}
#endif
	}
}

/** 시퀀스 재생 */
void ASDKQuest::PlayLevelSequence(FName SequenceTag)
{
	TArray<AActor*> SequenceList;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), ALevelSequenceActor::StaticClass(), LevelSequenceList[0], SequenceList);

	bool bSuccess = false;
	if (SequenceList.Num() > 0)
	{
		CurrentLevelSequence = Cast<ALevelSequenceActor>(SequenceList[0]);
		if (CurrentLevelSequence.IsValid())
		{
			ASDKPlayerCharacter* SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
			if (IsValid(SDKPlayerCharacter))
			{
				if (IsValid(SDKPlayerCharacter->GetCapsuleComponent()))
				{
					SDKPlayerCharacter->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				}
			}

			bSuccess = true;

			USDKLevelSequenceManager::Get()->SetLevelSequence(CurrentLevelSequence.Get());
			USDKLevelSequenceManager::Get()->OnFinishedSequence.AddDynamic(this, &ASDKQuest::FinishLevelSequence);

			OnPlaySequence();
			StartCameraFade(0.5f);
			USDKLevelSequenceManager::Get()->PlayLevelSequence();
		}
	}

	if (!bSuccess)
	{
#if WITH_EDITOR
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, FString::Printf(TEXT("Not Found Sequece Tag : %s"), *LevelSequenceList[0].ToString()));
#endif
		UE_LOG(LogGame, Log, TEXT("Not Found Sequece Tag : %s"), *LevelSequenceList[0].ToString());

		CheckEndLastDirecting();
	}
}

void ASDKQuest::FinishLevelSequence()
{
	ASDKPlayerCharacter* SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (IsValid(SDKPlayerCharacter))
	{
		if (IsValid(SDKPlayerCharacter->GetCapsuleComponent()))
		{
			SDKPlayerCharacter->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
	}

	if (LevelSequenceList.Num() > 0)
	{
		LevelSequenceList.RemoveAt(0);
	}

	if (CurrentLevelSequence.IsValid())
	{
		CurrentLevelSequence.Reset();
	}

	StartCameraFade(0.5f);
	CheckEndLastDirecting();
}

/** 영상 재생 */
void ASDKQuest::PlayMediaPlayer(FString ID)
{
	ASDKPlayerController* SDKPlayerController = Cast<ASDKPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKPlayerController))
	{
		ASDKHUD* SDKHUD = SDKPlayerController->GetHUD<ASDKHUD>();
		if (IsValid(SDKHUD))
		{
			USDKMediaPlayerWidget* MediaPlayerWidget = Cast<USDKMediaPlayerWidget>(SDKHUD->OpenUI(EUI::MediaPlayer));
			if (IsValid(MediaPlayerWidget))
			{
				MediaPlayerWidget->SetMediaPlayerID(ID);
				if(MediaPlayerWidget->NotifyFinishMediaPlayerEvent.Contains(this, FName("FinishMediaPlayer")) == false)
				{
					MediaPlayerWidget->NotifyFinishMediaPlayerEvent.AddDynamic(this, &ASDKQuest::FinishMediaPlayer);
				}
			}
		}
	}
}

void ASDKQuest::FinishMediaPlayer()
{
	StartCameraFade(0.5f);
	CheckEndLastDirecting();
}

/** 활성화된 퀘스트의 액터 타입 스폰 */
void ASDKQuest::SpawnQuestActor()
{
	EActorType SpawnType = EActorType::None;
	FString TableID = TEXT("");
	if (bUseTableID)
	{
		if (QuestSpawnActor.Num() <= 0)
		{
			return;
		}

		SpawnType = QuestSpawnActor[0].ActorType;
		TableID = FString::FromInt(QuestSpawnActor[0].SpawnID);

		bIsAlwaysSpawn = QuestSpawnActor[0].bIsAlwaysSpawn;
		bDestoryActorEndDirectiong = QuestSpawnActor[0].bDestoryActorEndDirectiong;
	}
	else
	{
		if (QuestActorType == EActorType::None || QuestActorID == 0 || SpawnedActor.IsValid() == true)
		{
			return;
		}

		SpawnType = QuestActorType;
		TableID = FString::FromInt(QuestActorID);
	}

	switch (SpawnType)
	{
	case EActorType::Character:	SpawnQuestNpc(TableID);		break;
	case EActorType::Object:	SpawnQuestObject(TableID);	break;
	case EActorType::Item:		SpawnQuestItme(TableID);	break;
	default:
		break;
	}

	if (QuestMissionID > 0)
	{
		CheckEndLastDirecting();
	}
}

void ASDKQuest::FinishSpawnQuestActor()
{
	GetWorldTimerManager().ClearTimer(SpawnHandle);

	if (QuestID > 0 && QuestState == EQuestState::End)
	{
		if (QuestActorType == EActorType::Character)
		{
			ASDKCharacter* SpawnedNpc = Cast<ASDKCharacter>(SpawnedActor);
			if (IsValid(SpawnedNpc) && IsValid(SpawnedNpc->GetCapsuleComponent()))
			{
				SpawnedNpc->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				return;
			}
		}
	}

	CheckEndLastDirecting();
}

void ASDKQuest::SpawnQuestNpc(FString TableID)
{
	auto CharacterTable = USDKTableManager::Get()->FindTableRow<FS_Character>(ETableType::tb_Character, TableID);
	if (CharacterTable != nullptr)
	{
		TSubclassOf<ASDKCharacter> CharacterClass(*USDKAssetManager::Get().LoadSynchronous(CharacterTable->PawnClass));
		if (IsValid(CharacterClass))
		{
			// 충돌 체크 방식 설정
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnInfo.Owner = this;

			FVector vLocation = GetActorLocation();
			if (FSDKHelpers::GetGameMode(GetWorld()) < EGameMode::Beg_Ingame)
			{
				// 월드에서만 높이값 좀 크게 주기
				vLocation.Z += 10.f;
			}

			// Npc 스폰
			APawn* NewPawn = UAIBlueprintHelperLibrary::SpawnAIFromClass(GetWorld(), CharacterClass, nullptr, vLocation, GetActorRotation(), false, this);
			if (IsValid(NewPawn))
			{
				ASDKCharacter* NewCharacter = Cast<ASDKNonePlayerCharacter>(NewPawn);
				if (IsValid(NewCharacter))
				{
					SpawnedActor = NewCharacter;

					NewCharacter->SetTableID(FCString::Atoi(*TableID));
					NewCharacter->SetNpcType(CharacterTable->NpcType);
					NewCharacter->SetSpawnOwnerObject(this);
					NewCharacter->SetOwner(this);

					// 컨트롤러 빙의
					FActorSpawnParameters SpawnControllerInfo;
					SpawnControllerInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					SpawnControllerInfo.Instigator = NewCharacter;

					ASDKAIController* NewAIController = GetWorld()->SpawnActor<ASDKAIController>(ASDKAIController::StaticClass(), vLocation, GetActorRotation(), SpawnControllerInfo);
					if (IsValid(NewAIController))
					{
						NewAIController->Possess(NewCharacter);
					}
				}
			}
		}
	}
}

void ASDKQuest::SpawnQuestItme(FString TableID)
{
	auto ItemTable = USDKTableManager::Get()->FindTableRow<FS_Item>(ETableType::tb_Item, TableID);
	if (ItemTable != nullptr)
	{
		if (!ItemTable->Enable)
		{
#if WITH_EDITOR
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, FString::Printf(TEXT("Disable Item! : %s"), *TableID));
#endif
			UE_LOG(LogGame, Error, TEXT("Disable Item! : %d"), QuestActorID);
			return;
		}

		if (ItemTable->ItemType != EItemType::Weapon)
		{
			FTransform const SpawnTransform(GetActorRotation(), GetActorLocation());

			if (ItemTable->MovingItem)
			{
				ASDKMovingItem* const SDKMovingItem = FSDKHelpers::SpawnMovingItem(GetWorld(), QuestActorID, ItemTable->ItemType, SpawnTransform);
				if (IsValid(SDKMovingItem))
				{
					SDKMovingItem->SetOwner(this);
					SDKMovingItem->SetMovingDistance(250.f);

					SpawnedActor = SDKMovingItem;
				}
			}
			else
			{
				ASDKNormalItem* const SDKNormalItem = FSDKHelpers::SpawnNormalItem(GetWorld(), QuestActorID, ItemTable->ItemType, SpawnTransform);
				if (IsValid(SDKNormalItem))
				{
					SDKNormalItem->SetOwner(this);
					SpawnedActor = SDKNormalItem;
				}
			}
		}
	}
}

void ASDKQuest::SpawnQuestObject(FString TableID)
{
	auto ObjectTable = USDKTableManager::Get()->FindTableRow<FS_Object>(ETableType::tb_Object, TableID);
	if (ObjectTable != nullptr)
	{
		TSubclassOf<ASDKObject> ObjectClass(*USDKAssetManager::Get().LoadSynchronous(ObjectTable->Path));
		if (ObjectClass)
		{
			FTransform const SpawnTransform(GetActorRotation(), GetActorLocation());

			ASDKObject* SDKObject = GetWorld()->SpawnActorDeferred<ASDKObject>(ObjectClass, SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (IsValid(SDKObject))
			{
				UGameplayStatics::FinishSpawningActor(SDKObject, SpawnTransform);

				SDKObject->SetOwner(this);
				SDKObject->SetActiveObject(true);
				SpawnedActor = SDKObject;
			}
		}
	}
}

/** 반환 : 스폰시킬 위치값 */
FVector ASDKQuest::GetSpawnLocation()
{
	if (IsValid(this))
	{
		FVector StartLoc = GetActorLocation() + FVector(0.f, 0.f, 200.f);
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = { ObjectTypeQuery1, ObjectTypeQuery2, OBJECT_CUBE };
		TArray<AActor*> IgnoreActors = { this };

		FHitResult HitResult;
		UKismetSystemLibrary::LineTraceSingleForObjects(GetWorld(), StartLoc, StartLoc - FVector(0.f, 0.f, 10000.f), ObjectTypes, false, IgnoreActors, EDrawDebugTrace::None, HitResult, true);
		if (HitResult.bBlockingHit)
		{
			return HitResult.Location;
		}
	}

	return GetActorLocation();
}

/** 컨텐츠 UI 열기 */
void ASDKQuest::OpenContentsUI()
{
	if (QuestID <= 0)
	{
		return;
	}

	auto SDKPlayerController = Cast<ASDKPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (IsValid(SDKPlayerController) == false || IsValid(SDKPlayerController->GetActiveQuestNPC()) == false)
	{
		return;
	}

	EMenuIconType OpenUIMenu = EMenuIconType::None;
	auto QuestTable = USDKTableManager::Get()->FindTableRow<FS_Quest>(ETableType::tb_Quest, FString::FromInt(QuestID));
	if (QuestTable != nullptr)
	{
		OpenUIMenu = QuestTable->UnlokedUI;
	}

	if(OpenUIMenu != EMenuIconType::None)
	{
		auto MenuIconTable = USDKTableManager::Get()->FindTableMainMenu(OpenUIMenu);
		if (MenuIconTable != nullptr && MenuIconTable->LinkUI != EUI::None)
		{
			auto SDKHUD = SDKPlayerController->GetHUD<ASDKHUD>();
			if (IsValid(SDKHUD))
			{
				auto OpenWidget = SDKHUD->OpenUI(MenuIconTable->LinkUI);
				if (IsValid(OpenWidget) && (OpenUIMenu == EMenuIconType::MoveFarm || OpenUIMenu == EMenuIconType::MoveMine || OpenUIMenu == EMenuIconType::MoveRanch))
				{
					USDKPopupProductionWidget* ProductionWidget = Cast<USDKPopupProductionWidget>(OpenWidget);
					if (IsValid(ProductionWidget))
					{
						FString ProductionName = FSDKHelpers::EnumToString(TEXT("EMenuIconType"), static_cast<int32>(OpenUIMenu));
						ProductionName.RemoveFromStart(TEXT("Move"));

						ProductionWidget->SetProducBaseInfoByName(ProductionName);
					}
				}
			}
		}

		SDKPlayerController->SetActiveQuestNPC(nullptr);
	}
}

/** 시퀀스 바인드 해제 */
void ASDKQuest::RemoveLevelSequenceBinding()
{
	TArray<AActor*> ActorList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALevelSequenceActor::StaticClass(), ActorList);
	if (ActorList.Num() > 0)
	{
		for (auto iter : ActorList)
		{
			CurrentLevelSequence = Cast<ALevelSequenceActor>(iter);
			if (!CurrentLevelSequence.IsValid() || !IsValid(CurrentLevelSequence.Get()->GetSequencePlayer()))
			{
				continue;
			}

			if (CurrentLevelSequence.Get()->GetSequencePlayer()->OnFinished.Contains(this, FName("StartDirectiongList")))
			{
				CurrentLevelSequence.Get()->GetSequencePlayer()->OnFinished.Remove(this, FName("StartDirectiongList"));
			}
		}
	}
}

#if WITH_EDITOR
void ASDKQuest::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (MemberPropertyName == TEXT("bUseDefaultTrigger") && PropertyName == TEXT("bUseDefaultTrigger"))
	{
		if (IsValid(DefaultTriggerComponent))
		{
			DefaultTriggerComponent->SetCollisionEnabled(bUseDefaultTrigger ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		}
	}

	if(QuestState == EQuestState::None)
	{
		TArray<FString> AssetNameList;
		GetName().ParseIntoArray(AssetNameList, TEXT("_"));

		if (AssetNameList.Num() >= 3)
		{
			if (AssetNameList[2].Equals(TEXT("Ending")))
			{
				/*SetQuestID(FCString::Atoi(*IDList[1]));*/
				SetQuestState(EQuestState::End);
			}
			else
			{
				/*SetQuestMissionID(FCString::Atoi(*IDList[1]));*/
				int32 index = AssetNameList.Num() - 3;
				SetQuestState(static_cast<EQuestState>(FSDKHelpers::StringToEnum(TEXT("EQuestState"), AssetNameList[index])));
			}
		}
	}

	if (PropertyName == FName("bUseTableID"))
	{
		if (!bUseTableID)
		{
			QuestActorTableID.Empty();

			DirectingList.Empty();
			ActionDataList.Empty();
			QuestSpawnActor.Empty();

			DialogueList.Empty();
			DialogueTypeList.Empty();
		}
	}

	if (PropertyName == FName("QuestActorTableID"))
	{
		SetQuestActorTableID();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
