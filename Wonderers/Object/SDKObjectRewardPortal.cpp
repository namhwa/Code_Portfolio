// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/SDKObjectRewardPortal.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Engine/SDKGameInstance.h"
#include "Engine/SDKAssetManager.h"

#include "Character/SDKHUD.h"
#include "Character/SDKMyInfo.h"
#include "Character/SDKRpgPlayerState.h"
#include "Character/SDKPlayerCharacter.h"
#include "Character/SDKInGameController.h"

#include "GameMode/SDKRpgState.h"
#include "GameMode/SDKRpgGameMode.h"
#include "Manager/SDKTableManager.h"
#include "Manager/SDKLobbyServerManager.h"
#include "Share/SDKDataTable.h"
#include "Share/SDKHelper.h"
#include "Share/SDKEnum.h"


ASDKObjectRewardPortal::ASDKObjectRewardPortal()
	: Super()
{
	PrimaryActorTick.bCanEverTick = false;
	RelationType = ERelationType::RewardPortal;

	if (IsValid(CollisionComponent) == true)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		CollisionComponent->SetCollisionResponseToChannel(COLLISION_CHARACTER, ECR_Overlap);
	}

	if (IsValid(MeshComponent) == true)
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	// 다음방 연결 포탈 콜리전
	NextRoomPortal1 = CreateDefaultSubobject<UBoxComponent>(FName(TEXT("NextRoomPortal_1")));
	if (IsValid(NextRoomPortal1) == true)
	{
		NextRoomPortal1->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NextRoomPortal1->SetCollisionResponseToAllChannels(ECR_Ignore);
		NextRoomPortal1->SetCollisionResponseToChannel(COLLISION_CHARACTER, ECR_Overlap);
		NextRoomPortal1->SetGenerateOverlapEvents(true);
		NextRoomPortal1->SetupAttachment(RootComponent);

		if (NextRoomPortal1->OnComponentBeginOverlap.Contains(this, FName("OnNotifyBeginOverlapNextRoomPortal")) == false)
		{
			NextRoomPortal1->OnComponentBeginOverlap.AddDynamic(this, &ASDKObjectRewardPortal::OnNotifyBeginOverlapNextRoomPortal);
			NextRoomPortal1->OnComponentEndOverlap.AddDynamic(this, &ASDKObjectRewardPortal::OnNotifyEndOverlapNextRoomPortal);
		}
	}

	NextRoomPortal2 = CreateDefaultSubobject<UBoxComponent>(FName(TEXT("NextRoomPortal_2")));
	if (IsValid(NextRoomPortal2) == true)
	{
		NextRoomPortal2->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NextRoomPortal2->SetCollisionResponseToAllChannels(ECR_Ignore);
		NextRoomPortal2->SetCollisionResponseToChannel(COLLISION_CHARACTER, ECR_Overlap);
		NextRoomPortal2->SetGenerateOverlapEvents(true);
		NextRoomPortal2->SetupAttachment(RootComponent);

		if (NextRoomPortal2->OnComponentBeginOverlap.Contains(this, FName("OnNotifyBeginOverlapNextRoomPortal")) == false)
		{
			NextRoomPortal2->OnComponentBeginOverlap.AddDynamic(this, &ASDKObjectRewardPortal::OnNotifyBeginOverlapNextRoomPortal);
			NextRoomPortal2->OnComponentEndOverlap.AddDynamic(this, &ASDKObjectRewardPortal::OnNotifyEndOverlapNextRoomPortal);
		}
	}

	NextRoomPortal3 = CreateDefaultSubobject<UBoxComponent>(FName(TEXT("NextRoomPortal_3")));
	if (IsValid(NextRoomPortal3) == true)
	{
		NextRoomPortal3->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NextRoomPortal3->SetCollisionResponseToAllChannels(ECR_Ignore);
		NextRoomPortal3->SetCollisionResponseToChannel(COLLISION_CHARACTER, ECR_Overlap);
		NextRoomPortal3->SetGenerateOverlapEvents(true);
		NextRoomPortal3->SetupAttachment(RootComponent);


		if (NextRoomPortal3->OnComponentBeginOverlap.Contains(this, FName("OnNotifyBeginOverlapNextRoomPortal")) == false)
		{
			NextRoomPortal3->OnComponentBeginOverlap.AddDynamic(this, &ASDKObjectRewardPortal::OnNotifyBeginOverlapNextRoomPortal);
			NextRoomPortal3->OnComponentEndOverlap.AddDynamic(this, &ASDKObjectRewardPortal::OnNotifyEndOverlapNextRoomPortal);
		}
	}
}

void ASDKObjectRewardPortal::BeginPlay()
{
	Super::BeginPlay();

	SetVisibilityNextRoomPortal(NextRoomPortal1, false);
	SetVisibilityNextRoomPortal(NextRoomPortal2, false);
	SetVisibilityNextRoomPortal(NextRoomPortal3, false);
}

void ASDKObjectRewardPortal::NotifyActorBeginOverlap(AActor* OtherActor)
{
	USDKGameInstance* SDKGameInstance = GetGameInstance<USDKGameInstance>();
	if (!IsValid(SDKGameInstance))
	{
		return;
	}
	ASDKPlayerCharacter* SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
	if (!IsValid(SDKPlayerCharacter))
	{
		return;
	}
	ASDKInGameController* SDKIngameController = SDKPlayerCharacter->GetSDKPlayerController();
	if (!IsValid(SDKIngameController))
	{
		return;
	}
	// 오버랩 리스트에 발판 넣기 - 그래야 입장권 사용 패킷을 받은 후 포탈 활성화를 진행할 수 있음
	ASDKRpgState* SDKRpgState = GetWorld()->GetGameState<ASDKRpgState>();
	if (!IsValid(SDKRpgState))
	{
		return;
	}

	if (GetActiveObject() == false)
	{
		return;
	}
}

void ASDKObjectRewardPortal::NotifyActorEndOverlap(AActor* OtherActor)
{
	ASDKPlayerCharacter* SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
	if (IsValid(SDKPlayerCharacter) == false)
	{
		return;
	}

	ASDKInGameController* SDKIngameController = SDKPlayerCharacter->GetSDKPlayerController();
	if (IsValid(SDKIngameController) == true)
	{
		ASDKRpgState* SDKRpgState = GetWorld()->GetGameState<ASDKRpgState>();
		if (IsValid(SDKRpgState) == true)
		{
			if (GetActiveObject() == false)
			{
				return;
			}
		}
	}
}

void ASDKObjectRewardPortal::InitObject()
{
	Super::InitObject();

	ASDKRpgState* SDKRpgState = GetWorld()->GetGameState<ASDKRpgState>();
	if (IsValid(SDKRpgState) == true)
	{
		LevelType = SDKRpgState->GetCurrentMapType();

		if (LevelType == FName("Start"))
		{
			if (IsValid(CollisionComponent) == true)
			{
				CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			}
		}

		if (LevelType == FName("Store") && SDKRpgState->GetPlayingRoomPhase() == false)
		{
			UE_LOG(LogGame, Log, TEXT("[Object_Reward_Portal] InitObject : Store Type"));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("[Object_Reward_Portal] InitObject : Store Type"));

			SetActiveObject(true);
			return;
		}
	}

	SetActiveGetAllItems(false);
	SetActiveObject(false);
}

void ASDKObjectRewardPortal::SetActiveObject(bool bActive)
{
	Super::SetActiveObject(bActive);

	UE_LOG(LogGame, Log, TEXT("[Object_Reward_Portal] SetActiveObject : %s"), bActive ? TEXT("T") : TEXT("F"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, FString::Printf(TEXT("[Object_Reward_Portal] SetActiveObject : %s"), bActive ? TEXT("T") : TEXT("F")));

	if(bActive == true)
	{
		if (bActiveGetAllItems)
		{
			ASDKRpgState* SDKRpgState = GetWorld()->GetGameState<ASDKRpgState>();
			if (IsValid(SDKRpgState) == true)
			{
				SDKRpgState->CheckRemaindItems();
			}
		}

		SetActiveNextRoomPortal();
	}
}

/** 남아있는 아이템 모두 흡수하는 기능 활성화 설정 */
void ASDKObjectRewardPortal::SetActiveGetAllItems(bool bActive)
{
	bActiveGetAllItems = bActive;
}

/** 보상 처리 활성화 */
void ASDKObjectRewardPortal::ActiveRewardPortal()
{
	UE_LOG(LogGame, Log, TEXT("[%s] ActiveRewardPortal"), *GetName());

	if (IsValid(CollisionComponent) == true)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetActiveObject(true);
}

/** 보상 스폰 */
void ASDKObjectRewardPortal::SpawnRewardObject(FName RewardObjectID)
{
	if (RewardObjectID.IsNone() == false && RewardObject.IsValid() == false)
	{
		auto ObjectTable = USDKTableManager::Get()->FindTableRow<FS_Object>(ETableType::tb_Object, RewardObjectID.ToString());
		if (ObjectTable != nullptr)
		{
			TSubclassOf<ASDKObject> ObjectClass(*USDKAssetManager::Get().LoadSynchronous(ObjectTable->Path));
			if (IsValid(ObjectClass) == true)
			{
				RewardObject = GetWorld()->SpawnActorDeferred<ASDKObject>(ObjectClass, GetActorTransform(), this, nullptr);
				if (RewardObject.IsValid() == true)
				{
					RewardObject.Get()->SetActiveObject(true);

					FVector vLocation = GetActorLocation();
					vLocation.Z += 50.f;

					if (IsValid(UGameplayStatics::FinishSpawningActor(RewardObject.Get(), FTransform(GetActorRotation(), vLocation))) == false)
					{
						UE_LOG(LogGame, Warning, TEXT("[%s] Failed FinishSpawningActor : %s"), *GetName(), *RewardObjectID.ToString());
						SetActiveObject(true);
					}

					RewardObject->OnDestroyed.AddDynamic(this, &ASDKObjectRewardPortal::DestoryReward);
				}
			}
		}
	}

	if (RewardObject.IsValid() == false)
	{
		SetActiveObject(true);
		UE_LOG(LogGame, Log, TEXT("[%s] Not Found Object Table ID : %s"), *GetName(), *RewardObjectID.ToString());
	}
}

/** 다음 방 포탈 오버랩 알림 */
void ASDKObjectRewardPortal::OnNotifyBeginOverlapNextRoomPortal(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (GetActiveObject() == false || IsValid(OverlappedComponent) == false)
	{
		return;
	}

	ASDKPlayerCharacter* SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
	if (IsValid(SDKPlayerCharacter) == false)
	{
		return;
	}

	ASDKInGameController* IngameController = SDKPlayerCharacter->GetSDKPlayerController();
	if (IsValid(IngameController) == false)
	{
		return;
	}

	IngameController->SetActiveRewardPortalObject(this);

	ASDKRpgPlayerState* RpgPlayerState = IngameController->GetPlayerState<ASDKRpgPlayerState>();
	if (IsValid(RpgPlayerState) == true)
	{
		int32 SelectIndex = 0;
		if (OverlappedComponent == NextRoomPortal1)
		{
			if (NextRoomCount > 1)
			{
				SelectIndex = 1;
			}
			else
			{
				SelectIndex = 0;
			}
		}
		else if (OverlappedComponent == NextRoomPortal2)
		{
			SelectIndex = 0;
		}
		else
		{
			if (NextRoomCount > 2)
			{
				SelectIndex = 2;
			}
			else
			{
				SelectIndex = 1;
			}
		}

		RpgPlayerState->SetSelectNextRoomRow(SelectIndex);
	}
}

/** 다음 방 포탈 오버랩 알림 */
void ASDKObjectRewardPortal::OnNotifyEndOverlapNextRoomPortal(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (GetActiveObject() == false)
	{
		return;
	}

	ASDKPlayerCharacter* SDKPlayerCharacter = Cast<ASDKPlayerCharacter>(OtherActor);
	if (IsValid(SDKPlayerCharacter) == false)
	{
		return;
	}

	ASDKInGameController* IngameController = SDKPlayerCharacter->GetSDKPlayerController();
	if (IsValid(IngameController) == false)
	{
		return;
	}

	IngameController->SetDeactiveRewardPortalObject(this);
}

/** 선택한 방으로 이동 */
void ASDKObjectRewardPortal::MoveToSelectNextRoom()
{
	ASDKRpgState* SDKRpgState = GetWorld()->GetGameState<ASDKRpgState>();
	if (IsValid(SDKRpgState) && SDKRpgState->IsGameActive() == false)
	{
		UE_LOG(LogGame, Log, TEXT("ASDKObjectRewardPortal::MoveToSelectNextRoom : SDKRpgState Is Invalid Or GameActive Is False"));
		return;
	}

	ASDKInGameController* IngameController = Cast<ASDKInGameController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!IsValid(IngameController))
	{
		UE_LOG(LogGame, Log, TEXT("ASDKObjectRewardPortal::MoveToSelectNextRoom : IngameController Is Invalid"));
		return;
	}

	IngameController->SetDeactiveRewardPortalObject(this);
	IngameController->ClientMoveToNextRoom(true);
}

/** 보상 제거 */
void ASDKObjectRewardPortal::DestoryReward(AActor* DestroyedActor)
{
	OnDestoryReward.Broadcast();
}

/** 튜토리얼 전용 포탈 활성화 함수 */
void ASDKObjectRewardPortal::SetActiveTutorialNextRoomPortal()
{
	ASDKRpgGameMode* RpgGameMode = GetWorld()->GetAuthGameMode<ASDKRpgGameMode>();
	if (!IsValid(RpgGameMode))
	{
		return;
	}

	const FRoomData CurrentData = RpgGameMode->GetCurrentRoomData();
	TArray<FName> RoomTypeArray;

	if (CurrentData.NextRoomTypes.Num() > 0)
	{
		NextRoomCount = CurrentData.NextRoomTypes.Num();
		CurrentData.NextRoomTypes.GenerateValueArray(RoomTypeArray);
	}

	if (IsValid(NextRoomPortal1) == true)
	{
		NextRoomPortal1->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		OnSetNextRoomType(1, RoomTypeArray[0]);
		SetVisibilityNextRoomPortal(NextRoomPortal1, true);
	}

	OnActiveNextRoomPortal.Broadcast();
}

/** 다음 방 포탈 활성화 */
void ASDKObjectRewardPortal::SetActiveNextRoomPortal()
{
	UE_LOG(LogGame, Log, TEXT("[Object_Reward_Portal] SetActiveNextRoomPortal"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("[Object_Reward_Portal] SetActiveNextRoomPortal"));

	if (!bActiveObject)
	{
		return;
	}

	ASDKRpgGameMode* RpgGameMode = GetWorld()->GetAuthGameMode<ASDKRpgGameMode>();
	if (!IsValid(RpgGameMode))
	{
		return;
	}

	const FRoomData CurrentData = RpgGameMode->GetCurrentRoomData();
	TArray<FName> RoomTypeArray;

	if (CurrentData.NextRoomTypes.Num() > 0)
	{
		NextRoomCount = CurrentData.NextRoomTypes.Num();
		CurrentData.NextRoomTypes.GenerateValueArray(RoomTypeArray);
	}

	switch (RoomTypeArray.Num())
	{
	case 1:
	{
		if (IsValid(NextRoomPortal1) == true)
		{
			NextRoomPortal1->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			OnSetNextRoomType(1, RoomTypeArray[0]);
			SetVisibilityNextRoomPortal(NextRoomPortal1, true);
		}
	}
	break;
	case 2:
	{
		if (IsValid(NextRoomPortal2) == true)
		{
			NextRoomPortal2->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			OnSetNextRoomType(2, RoomTypeArray[0]);
			SetVisibilityNextRoomPortal(NextRoomPortal2, true);
		}

		if (IsValid(NextRoomPortal3) == true)
		{
			NextRoomPortal3->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			OnSetNextRoomType(3, RoomTypeArray[1]);
			SetVisibilityNextRoomPortal(NextRoomPortal3, true);
		}
	}
	break;
	case 3:
	{
		if (IsValid(NextRoomPortal1) == true)
		{
			NextRoomPortal1->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			OnSetNextRoomType(1, RoomTypeArray[1]);
			SetVisibilityNextRoomPortal(NextRoomPortal1, true);
		}

		if (IsValid(NextRoomPortal2) == true)
		{
			NextRoomPortal2->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			OnSetNextRoomType(2, RoomTypeArray[0]);
			SetVisibilityNextRoomPortal(NextRoomPortal2, true);
		}

		if (IsValid(NextRoomPortal3) == true)
		{
			NextRoomPortal3->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			OnSetNextRoomType(3, RoomTypeArray[2]);
			SetVisibilityNextRoomPortal(NextRoomPortal3, true);
		}
	}
	break;
	default:
		return;
	}

	OnActiveNextRoomPortal.Broadcast();

	UE_LOG(LogGame, Log, TEXT("[Object_Reward_Portal] SetActiveNextRoomPortal Next Count : %d"), RoomTypeArray.Num());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, FString::Printf(TEXT("[Object_Reward_Portal] SetActiveNextRoomPortal Next Count : %d"), RoomTypeArray.Num()));
}

/** 다음방 포탈 표시 여부 */
void ASDKObjectRewardPortal::SetVisibilityNextRoomPortal(UBoxComponent* InComp, bool bVisible)
{
	if (IsValid(InComp) == false)
	{
		return;
	}

	InComp->SetVisibility(bVisible);

	TArray<USceneComponent*> ChildCompList;
	InComp->GetChildrenComponents(false, ChildCompList);

	if (ChildCompList.Num() > 0)
	{
		for (auto itChild : ChildCompList)
		{
			itChild->SetVisibility(bVisible);
		}
	}
}
