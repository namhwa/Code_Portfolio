// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Share/SDKEnum.h"
#include "Share/SDKData.h"
#include "Share/SDKDataStruct.h"
#include "SDKQuestManager.generated.h"

class UWorld;
class AActor;

class ASDKQuest;

DECLARE_LOG_CATEGORY_EXTERN(LogQuest, Log, All);

DECLARE_MULTICAST_DELEGATE(FEndQuestActorEventCallback);


UCLASS()
class ARENA_API USDKQuestManager : public UObject
{
	GENERATED_BODY()

public:
	USDKQuestManager();
	virtual ~USDKQuestManager() override;

	// Begin UObject Interface
#if WITH_ENGINE
	virtual UWorld* GetWorld() const override;
#endif
	// End UObject Interface

public:
	bool GetIsValidData() const { return (mapQuestData.Num() > 0 || mapQuestMissionData.Num() > 0); }

public:
	/** 확인 : 퀘스트 */
	bool IsProgressingQuestMission(int32 QuestMissionID) const { return ProgressingMissionIDList.Contains(QuestMissionID); }
	bool IsEndQuestMission(int32 QuestMissionID) const { return EndMissionIDList.Contains(QuestMissionID); }
	bool IsContainsCompleteMissionID(int32 QuestMissionID) const { return CompletableMissionIDList.Contains(QuestMissionID); }
	bool IsEndQuest(int32 QuesteID) const { return EndQuestIDList.Contains(QuesteID); }

	/** 확인 : 특정 조건에 대한 퀘스트 */
	void CheckActiveConditionQuestMissionList(int32 InHeroID, int32 InChapter, int32 Step, TMap<int32, FQuestCondition>& OutmapData);

	/** 반환 : 퀘스트 목록 */
	TArray<int32> GetProgressingQuestMissionList() const { return ProgressingMissionIDList; }
	TArray<int32> GetCompletableQuestMissionList() const { return CompletableMissionIDList; }
	TArray<int32> GetEndQuestMissionList() const { return EndMissionIDList; }
	TArray<int32> GetEndQuestList() const { return EndQuestIDList; }

	/** 반환 : 퀘스트 미션 정보 */
	CQuestMissionData GetQuestMissionData(int32 QuestMissionID);

	/** 반환 : 활성화된 퀘스트 액터 개수 */
	int32 GetActivedQuestActorCount() const;

	/*** 퀘스트 ***************************************************************************************************/
	/** 설정 : 퀘스트 목록 */
	void SetQuestDataList(const TArray<int32>& QuestIDs, const TArray<CQuestMissionData>& Data);
	void SetQuestDataList(const TArray<CQuestData>& Data);

	void SetQuestCurrentIndex(const TArray<CQuestMissionData>& Data);

	/** 완료 : 완료된 퀘스트 정보 */
	void EndQuestData(int32 EndID);

	/*** 퀘스트 미션 **********************************************************************************************/
	/** 설정 : 퀘스트 목록 */
	void SetQuestMissionDataList(const TArray<CQuestMissionData>& Data);

	/** 설정 : 수락할 수 있는 퀘스트 미션 ID */
	void SetStartableQuestMissionList(int32 QuestID, int32 Index = 0);
	void SetStartableQuestMissionList();
	void SetStartableQuestMissionList(const TArray<CQuestMissionData>& InList);

	/** 설정 : 완료 가능한 퀘스트 미션 ID */
	void SetCompletableQuestMissionList(const TArray<int32>& IDList);

	/** 확인 : 진행중인 퀘스트 미션 조건 */
	void CheckProgressingQuestMissionData(EQuestMissionType MissionType, int32 TargetID, int32 Count = 1);
	void SendFinishPhaseQuestMissionCount();

	/** 갱신 : 시작된 퀘스트 미션 */
	void UpdateStartedQuestMissionData(const CQuestMissionData& Data, bool bUpdateActor = true);

	/** 갱신 : 진행중인 퀘스트 미션 정보 */
	void UpdateProgressingQuestMissionData(const TArray<CQuestMissionData>& Data);

	/** 완료 : 완료된 퀘스트 미션 정보 */
	void EndQuestMissionData(int32 QuestMissionID, bool bIsLast, const CQuestMissionData& Data);

	/** 미션 타입 : UI 동작인 경우 */
	void SetProgressingQuestMissionOperateUIType(int32 QuestMissionID, EUI InUI);

	/** 퀘스트 액터 완료 후, 실행되야하는 경우 */
	void CompletedEndQuestActorEvent(int32 InQuestMissionID);

	/*** 퀘스트 액터 **********************************************************************************************/
	/** 활성화 : 퀘스트 액터 목록 */
	void ActiveQuestActorList();

	/** 제거 : 퀘스트 액터 목록 */
	void RemoveQuestActor(ASDKQuest* Actor);

	/** 초기화 : 액터 목록 목록 */
	void ResetQuestActorList();

	/** 갱신 : 퀘스트 목록 UI */
	void UpdateQuestBoardUI(bool bNew = false);

	/*** 델리게이트 ***********************************************************************************************/
	void AddDelegateHandle(int32 InID, const FDelegateHandle& InHandle);
	void RemoveDelegateHandle(int32 InID);

	/** 초기화 : 퀘스트 매니저 */
	void ClearQuestManager();

	/*** 에디터 치트 */
	void EDITOR_ResetCheatClearQuest();

public:
	// 퀘스트 액터 연출 끝난 뒤, 진행해야되는 이벤트 델리게이트
	FEndQuestActorEventCallback EndQuestActorEventCallback;

protected:
 	/** 설정 : 퀘스트 데이터 */
 	void SetQuestData(const CQuestData& Data);
	void SetQuestMissionData(const CQuestMissionData& Data);

	/** 완료된 퀘스트 컨텐츠 해금 정보 */
	void SetUnlockContentsByEndQuest(int32 QuestID);

	/** 설정 : 퀘스트 액터 활성화 */
	void SetActiveQuestActors(EQuestState State, const TArray<int32>& IDList);

	/** 설정 : 퀘스트 미션 액터 활성화 */
	void SetActiveQuestMissionActors(EQuestState State, const TArray<int32>& IDList);

	/** 퀘스트 타입 순으로 정리 */
	void SortQuestMissioIDListByQuestType(TArray<int32>& IDList);

private:
	// 퀘스트 액터 목록
	UPROPERTY()
	TMap<int32, TWeakObjectPtr<ASDKQuest>> mapQuestActor;

	// 퀘스트 미션 액터 목록
	UPROPERTY()
	TMap<int32, TWeakObjectPtr<ASDKQuest>> mapQuestMissionActor;

	// 전투 중에 체크되서 끝나고 보내야하는 경우
	TMap<EQuestMissionType, TMap<int32, int32>> mapSavedMissions;

	/** 정리된 정보 목록 */
	// 수락할 수 있는 퀘스트 ID 목록
	UPROPERTY()
	TArray<int32> StartableMissionIDList;

	// 현재 진행중인 퀘스트 ID 목록
	UPROPERTY()
	TArray<int32> ProgressingQuestIDList;
	UPROPERTY()
	TArray<int32> ProgressingMissionIDList;

	// 완료 가능한 퀘스트 ID 목록
	UPROPERTY()
	TArray<int32> CompletableQuestIDList;
	UPROPERTY()
	TArray<int32> CompletableMissionIDList;

	// 완료 가능한 퀘스트 ID 목록
	UPROPERTY()
	TArray<int32> EndQuestIDList;
	UPROPERTY()
	TArray<int32> EndMissionIDList;

	// 전체 퀘스트 목록
 	TMap<int32, CQuestData> mapQuestData;
	TMap<int32, CQuestMissionData> mapQuestMissionData;

	// 현재 게임 모드
	EGameMode CurrentMode;

	TMap<int32, FDelegateHandle> DelegateHandleList;
};