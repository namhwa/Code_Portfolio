// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/SDKNpcComponent.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"

#include "Character/SDKHUD.h"
#include "Character/SDKCharacter.h"
#include "Character/SDKPlayerCharacter.h"
#include "Character/SDKPlayerController.h"
#include "Character/SDKInteractionBoxWidgetComponent.h"

#include "Object/SDKObject.h"
#include "Object/SDKSpawnComponent.h"


#include "UI/SDKUIScriptSceneWidget.h"

USDKNpcComponent::USDKNpcComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bUseEvent = false;
}

void USDKNpcComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(GetOwner()) == true)
	{
		ISDKTypeInterface* OwnerType = Cast<ISDKTypeInterface>(GetOwner());
		if (OwnerType != nullptr)
		{
			if (OwnerType->GetActorType() == EActorType::Character)
			{
				ASDKCharacter* OwnerCharacter = Cast<ASDKCharacter>(GetOwner());
				if (IsValid(OwnerCharacter) == true)
				{
					OwnerCharacter->GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &USDKNpcComponent::BeginOverlapComponent);
					OwnerCharacter->GetCapsuleComponent()->OnComponentEndOverlap.AddDynamic(this, &USDKNpcComponent::EndOverlapComponent);
					OwnerCharacter->InteractionActorEvent.AddDynamic(this, &USDKNpcComponent::ScriptSceneToActor);
				}
			}
			else
			{
				ASDKObject* OwnerObject = Cast<ASDKObject>(GetOwner());
				if (IsValid(OwnerObject) == true)
				{
					OwnerObject->GetCollisionComponent()->OnComponentBeginOverlap.AddDynamic(this, &USDKNpcComponent::BeginOverlapComponent);
					OwnerObject->GetCollisionComponent()->OnComponentEndOverlap.AddDynamic(this, &USDKNpcComponent::EndOverlapComponent);
					OwnerObject->InteractionActorEvent.AddDynamic(this, &USDKNpcComponent::ScriptSceneToActor);
				}
			}
		}

		SetActive(true);
	}
}

void USDKNpcComponent::SetActive(bool bNewActive, bool bReset /*= false*/)
{
	Super::SetActive(bNewActive, bReset);

	if (bNewActive == false)
	{
		UnBindScriptWidgetEvent();
	}
}

void USDKNpcComponent::BeginOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ASDKPlayerCharacter* PlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
	if (!IsValid(PlayerCharacter))
	{
		return;
	}	
	ASDKPlayerController* PlayerController = PlayerCharacter->GetController<ASDKPlayerController>();
	if (!IsValid(PlayerController))
	{
		return;
	}

	bool bInteractionBox = HasOwnerInterationBoxComponent();
	PlayerController->SetActiveNPC(GetOwner(), bInteractionBox);

	if (bInteractionBox)
	{
		SetActiveInteractionButton(true, PlayerController);
	}
}

void USDKNpcComponent::EndOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ASDKPlayerCharacter* PlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
	if (!IsValid(PlayerCharacter))
	{
		return;
	}
	ASDKPlayerController* PlayerController = PlayerCharacter->GetController<ASDKPlayerController>();
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->SetActiveNPC(nullptr);

	OverlapActor = nullptr;
	
	if (HasOwnerInterationBoxComponent())
	{
		SetActiveInteractionButton(false, PlayerController);
	}
}

void USDKNpcComponent::SetActiveInteractionButton(bool bEnable, ASDKPlayerController* TargetController)
{
	if (!IsValid(TargetController))
	{
		return;
	}

	USDKInteractionBoxWidgetComponent* InteractionComponent = Cast<USDKInteractionBoxWidgetComponent>(GetOwner()->GetComponentByClass(USDKInteractionBoxWidgetComponent::StaticClass()));
	if (!IsValid(InteractionComponent))
	{
		return;
	}

	if (bEnable)
	{
		TargetController->ClientActiveInteractionBox(InteractionComponent);
	}
	else
	{
		TargetController->ClientDeActiveInteractionBox();
	}

}

bool USDKNpcComponent::HasOwnerInterationBoxComponent()
{
	USDKInteractionBoxWidgetComponent* InteractionComponent = Cast<USDKInteractionBoxWidgetComponent>(GetOwner()->GetComponentByClass(USDKInteractionBoxWidgetComponent::StaticClass()));
	if (!IsValid(InteractionComponent))
	{
		return false;
	}

	return true;
}

void USDKNpcComponent::ScriptSceneToActor(AActor* OtherActor)
{
	OverlapActor = OtherActor;

	ASDKPlayerCharacter* PlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
	if (IsValid(PlayerCharacter) == false)
	{
		return;
	}

	ASDKPlayerController* PlayerController = PlayerCharacter->GetController<ASDKPlayerController>();
	if (IsValid(PlayerController) == true)
	{
		int32 index = 0;
		FString ScriptID = TEXT("");
		if (IsActive() == true)
		{
			if (ActiveScirptID.Num() <= 0)
			{
				FinishScriptScene();
				return;
			}

			index = FMath::RandRange(0, ActiveScirptID.Num() - 1);
			ScriptID = ActiveScirptID[index];
		}
		else
		{
			if (InactiveScriptID.Num() <= 0)
			{
				FinishScriptScene();
				return;
			}

			index = FMath::RandRange(0, InactiveScriptID.Num() - 1);
			ScriptID = InactiveScriptID[index];
		}

		// 대사창 UI 활성화
		PlayerController->ClientToggleScriptSceneTableID(true, ScriptID);

		// 대화 UI 바인딩
		if (IsActive() == true)
		{
			ASDKHUD* SDKHUD = PlayerController->GetHUD<ASDKHUD>();
			if (IsValid(SDKHUD) == true)
			{
				USDKUIScriptSceneWidget* ScriptWidget = Cast<USDKUIScriptSceneWidget>(SDKHUD->GetUI(EUI::UI_ScriptScene));
				if (IsValid(ScriptWidget) == true)
				{
					ScriptWidget->NofityCloseScriptEvent.AddDynamic(this, &USDKNpcComponent::FinishScriptScene);
				}
			}
		}
	}
}

void USDKNpcComponent::FinishScriptScene()
{
	ASDKPlayerCharacter* PlayerCharacter = Cast<ASDKPlayerCharacter>(OverlapActor);
	if (IsValid(PlayerCharacter) == false)
	{
		return;
	}

	ASDKPlayerController* PlayerController = PlayerCharacter->GetController<ASDKPlayerController>();
	if (IsValid(PlayerController) == false)
	{
		return;
	}

	if (IsActive() == true)
	{
		if (InteractUIType != EUI::None)
		{
			ASDKHUD* SDKHUD = PlayerController->GetHUD<ASDKHUD>();
			if (IsValid(SDKHUD) == true)
			{
				if(SDKHUD->IsOpenUI(InteractUIType))
				{
					SDKHUD->CloseUI(InteractUIType);
				}

				USDKUserWidget* OpenedUI = SDKHUD->OpenUI(InteractUIType);
				if (IsValid(OpenedUI))
				{
					OpenedUI->OnClosedUIEvent.AddUniqueDynamic(this, &USDKNpcComponent::OnClosedUIReActiveRelaion);
				}
			}
		}
		else
		{
			if (IsValid(GetOwner()) == true)
			{
				USDKSpawnComponent* SpawnComponent = Cast<USDKSpawnComponent>(GetOwner()->GetComponentByClass(USDKSpawnComponent::StaticClass()));
				if (IsValid(SpawnComponent) == true)
				{
					SpawnComponent->SetActive(true);
					SpawnComponent->RemoteSpawn(OverlapActor);
				}

				SetActive(false);
			}
		}
	}

	if (bUseEvent)
	{
		IsActive() ? OnFinishScriptActive.Broadcast() : OnFinishScriptInactive.Broadcast();
	}

	PlayerController->SetActiveNPC(nullptr);

	UnBindScriptWidgetEvent();
}

void USDKNpcComponent::OnClosedUIReActiveRelaion()
{
	// UI 종료 후 오버랩이 계속되고 있다면, Relation 재활성화 (다시 상호작용 가능하게)
	ASDKPlayerCharacter* PlayerCharacter = Cast<ASDKPlayerCharacter>(OverlapActor);
	if (!IsValid(PlayerCharacter))
	{
		return;
	}
	ASDKPlayerController* PlayerController = PlayerCharacter->GetController<ASDKPlayerController>();
	if (!IsValid(PlayerController))
	{
		return;
	}

	PlayerController->SetActiveNPC(GetOwner(), HasOwnerInterationBoxComponent());
}

void USDKNpcComponent::UnBindScriptWidgetEvent()
{
	ASDKPlayerCharacter* PlayerCharacter = Cast<ASDKPlayerCharacter>(OverlapActor);
	if (IsValid(PlayerCharacter) == false)
	{
		return;
	}

	ASDKPlayerController* PlayerController = PlayerCharacter->GetController<ASDKPlayerController>();
	if (IsValid(PlayerController) == true)
	{
		ASDKHUD* SDKHUD = PlayerController->GetHUD<ASDKHUD>();
		if (IsValid(SDKHUD) == true)
		{
			USDKUIScriptSceneWidget* ScriptWidget = Cast<USDKUIScriptSceneWidget>(SDKHUD->GetUI(EUI::UI_ScriptScene));
			if (IsValid(ScriptWidget) == true)
			{
				ScriptWidget->NofityCloseScriptEvent.Remove(this, FName("FinishScriptScene"));
			}
		}
	}
}