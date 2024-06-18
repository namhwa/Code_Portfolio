// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Share/SDKEnum.h"
#include "flatbuffers/lobby_generated.h"

#define FBSTRING_TO_STRING(str) str != nullptr ? UTF8_TO_TCHAR(str->c_str()) : TEXT("")

/*** 퀘스트 *********************************************************************************************************/
struct FMissionCountData
{
public:
	uint32 m_MissionID = 0;
	uint32 m_MissionCount = 0;
	EMissionType m_MissionType = EMissionType::None;

public:
	FMissionCountData() {}

	FMissionCountData(uint32 ID, uint32 Count, EMissionType Type)
	{
		m_MissionID = ID;
		m_MissionCount = Count;
		m_MissionType = Type;
	}

	FMissionCountData(const Packet::Lobby::MissionCountData* Data)
	{
		if(Data == nullptr)
			return;

		m_MissionID = Data->MissionID();
		m_MissionCount = Data->MissionCount();
		m_MissionType = static_cast<EMissionType>(Data->MissionType());
	}
};

struct FQuestData
{
public:
	uint32 m_QuestID;
	bool m_IsComplete;
	bool m_IsRewardReceived;

	EMissionType m_MissionType;
	TArray<FMissionCountData> m_MissionList;

public:
	FQuestData() {}

	FQuestData(const Packet::Lobby::QuestData* Data)
	{
		if(Data == nullptr)
			return;

		m_MissionType = EMissionType::None;

		m_QuestID = Data->QuestID();
		m_IsComplete = Data->IsComplete();
		m_IsRewardReceived = Data->IsRewardReceived();

		if(Data->MissionList() != nullptr)
		{
			auto MissionData = Data->MissionList();
			if(MissionData->Length() > 0)
			{
				for(uint32 i = 0; i < MissionData->Length(); i++)
				{
					if(MissionData->Get(i) != nullptr)
					{
						if(MissionData->Get(i) == nullptr)
							continue;

						m_MissionList.Add(FMissionCountData(MissionData->Get(i)));

						if(m_MissionType == EMissionType::None)
						{
							m_MissionType = m_MissionList[i].m_MissionType;
						}
					}
				}
			}
		}
	}
};

struct FGetQuestList
{
public:
	TArray<FQuestData> m_QuestList;

public:
	FGetQuestList() {}

	FGetQuestList(const Packet::Lobby::SA_GetQuestList* Data)
	{
		if(Data == nullptr)
			return;

		Init(Data->QuestList());
	}

	FGetQuestList(const Packet::Lobby::SA_UpdateMissionCount* Data)
	{
		if(Data == nullptr)
			return;

		Init(Data->ChangeQuestList());
	}

	FGetQuestList(const flatbuffers::Vector<flatbuffers::Offset<Packet::Lobby::QuestData>>* Data)
	{
		Init(Data);
	}

private:
	void Init(const flatbuffers::Vector<flatbuffers::Offset<Packet::Lobby::QuestData>>* Data)
	{
		if (Data == nullptr)
			return;

		if (Data->Length() > 0)
		{
			for (uint32 i = 0; i < Data->Length(); i++)
			{
				if (Data->Get(i) == nullptr)
					continue;

				m_QuestList.Add(FQuestData(Data->Get(i)));
			}
		}
	}
};
