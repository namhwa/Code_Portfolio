// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Share/SDKEnum.h"
#include "Share/SDKStruct.h"
#include "SDKQuest.generated.h"

class ATargetPoint;
class ALevelSequenceActor;

class UBoxComponent;
class ULevelSequence;
class USphereComponent;
class UArrowComponent;
class UParticleSystem;
class UParticleSystemComponent;

class UDialogueBuilderObject;


UCLASS(Blueprintable)
class ARENA_API ASDKQuest : public AActor
{
	GENERATED_BODY()
	
public:	
	ASDKQuest();

	// Being AActor Interface
	virtual void BeginPlay() override;
	virtual void SetLifeSpan(float InLifespan) override;
	virtual void LifeSpanExpired() override;
	// End AActor Interface

	/** 퀘스트 액터 이벤트 */
	UFUNCTION()
	void NotifyQuestBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void NotifyQuestEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** 반환 : 퀘스트 아이디 및 목록 */
	int32 GetQusetMissionID() const { return QuestMissionID; }
	EQuestState GetQuestState() const { return QuestState; }
	AActor* GetQuestSpawnActor() const { return SpawnedActor.Get(); }

	/** 설정 : 활성화된 퀘스트 아이디 */
	void SetQuestID(int32 ID);
	void SetQuestMissionID(int32 ID);
	void SetQuestState(EQuestState NewState);

	/** 트리거 이벤트 */
	UFUNCTION()
	void OnBeginOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnEndOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** 연출 시작 */
	UFUNCTION()
	void StartDirectiongList();

	/** 인터렉션 */
	void InteractionPlayer();
	
	/** 카메라 페이드 아웃 */
	void StartCameraFade(float Duratio);

	UFUNCTION(BlueprintImplementableEvent)
	void OnPlaySequence();

protected:
	void SetQuestActorTableID();

	/** 초기화 : 시작할 시점에 데이터 정리 */
	void InitQuestActor();
	void ResetQuestActor();

	/** 마지막 연출목록인지 확인 */
	void CheckEndLastDirecting();

	/** 연출 끝 */
	void EndSelectDirecting();

	/** 진행중인 상태에서 이벤트가 나와야하는 경우(퀘스트를 준 액터가) */
	void SetNotClearScriptID();

	/** 선택된 연출 활성화 */
	void ActiveDirectingData(FDirectingData& DirectingData);
	void ActiveActionData(FActionData& Data);

	/** 대사 */
	UFUNCTION()
	void FinishDialogue();

	/** 시퀀스 재생 */
	void PlayLevelSequence(FName SequenceTag);
	UFUNCTION()
	void FinishLevelSequence();

	/** 영상 재생 */
	void PlayMediaPlayer(FString ID);
	UFUNCTION()
	void FinishMediaPlayer();

	/** 활성화된 퀘스트의 액터 타입 스폰 */
	void SpawnQuestActor();
	UFUNCTION()
	void FinishSpawnQuestActor();

	void SpawnQuestNpc(FString TableID);
	void SpawnQuestItme(FString TableID);
	void SpawnQuestObject(FString TableID);

	/** 반환 : 스폰시킬 위치값 */
	FVector GetSpawnLocation();

	/** 컨텐츠 UI 열기 */
	void OpenContentsUI();

	/** 시퀀스 바인드 해제 */
	void RemoveLevelSequenceBinding();

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	UPROPERTY(EditAnywhere, Category = "SDKQuest")
	USphereComponent* DefaultSphereComponent;

	UPROPERTY()
	UArrowComponent* DirectionComponent;

	UPROPERTY(EditAnywhere, Category = "SDKQuest|Particle")
	UParticleSystemComponent* ParticleComponent;

	UPROPERTY(EditAnywhere, Category = "SDKQuest|Trigger", meta = (EditCondition = "bUseDefaultTrigger", EditConditionHides))
	UBoxComponent* DefaultTriggerComponent;

protected:
	UPROPERTY(EditAnywhere, Category = "SDKQuest")
	bool bUseTableID;

	UPROPERTY(EditAnywhere, Category = "SDKQuest", meta = (EditCondition = "bUseTableID"))
	FString QuestActorTableID;

protected:
	UPROPERTY(EditAnywhere, Category = "SDKQuest|Trigger", meta = (EditCondition = "!bUseTableID"))
	bool bUseDefaultTrigger;

	UPROPERTY(EditAnywhere, Category = "SDKQuest|Particle", meta = (EditCondition = "!bUseTableID"))
	TMap<EQuestState, UParticleSystem*> QuestParticle;

	// Start Directing List without any trigger box event
	UPROPERTY(EditAnywhere, Category = "SDKQuest", meta = (EditCondition = "!bUseTableID"))
	bool bIsAutoStartDirecting;

	// 연출 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SDKQuest|Directing", meta = (EditCondition = "!bUseTableID"))
	TArray<FDirectingData> DirectingList;

	// 액션 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SDKQuest|Directing|Action", meta = (EditCondition = "!bUseTableID"))
	TArray<FActionData> ActionDataList;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SDKQuest|Directing|Dialogue")
	TArray<FString> DialogueList;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SDKQuest|Directing|Dialogue")
	TArray<bool> DialogueTypeList;

	// 시퀀스 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SDKQuest|Directing|Sequence", meta = (EditCondition = "!bUseTableID"))
	TArray<FName> LevelSequenceList;
	UPROPERTY()
	TWeakObjectPtr<ALevelSequenceActor> CurrentLevelSequence;

	// 미디어 플레이어 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SDKQuest|Directing|Media", meta = (EditCondition = "!bUseTableID"))
	TArray<FString> MediaPlayTableIDs;

	// 스폰할 액터 목록
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SDKQuest|Directing|Spawn", meta = (EditCondition = "bUseTableID"))
	TArray<FSpawnData> QuestSpawnActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SDKQuest|Directing|Spawn", meta = (EditCondition = "!bUseTableID"))
	EActorType QuestActorType;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SDKQuest|Directing|Spawn", meta = (EditCondition = "!bUseTableID"))
	int32 QuestActorID;

	// 상시 스폰되어 있어야하는지에 대한 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SDKQuest|Directing|Spawn", meta = (EditCondition = "!bUseTableID"))
	bool bIsAlwaysSpawn;

	// 연출 완료 후, 퀘스트 액터를 삭제할건지에 대한 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SDKQuest|Directing|Spawn", meta = (EditCondition = "!bUseTableID"))
	bool bDestoryActorEndDirectiong;

	// 퀘스트가 갖고 있는 트리거 박스 목록
	UPROPERTY()
	TArray<UBoxComponent*> TriggerBoxList;

private:
	UPROPERTY()
	int32 QuestID;
	
	// 해당 액터의 퀘스트 미션 ID
	UPROPERTY()
	int32 QuestMissionID;

	// 퀘스트 상태 타입
	UPROPERTY()
	EQuestState QuestState;

	// 스폰된 액터
	UPROPERTY()
	TWeakObjectPtr<AActor> SpawnedActor;

	UPROPERTY()
	int32 DirectingIndex;

	// 타이머 핸들러
	FTimerHandle SpawnHandle;
};
