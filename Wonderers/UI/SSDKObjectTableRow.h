// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Slate/SObjectWidget.h"

#include "Blueprint/IUserObjectListEntry.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Framework/Views/ITypedTableView.h"

#include "Slate/SObjectTableRow.h"

#include "UI/SDKTileView.h"


class UListViewBase;

template <typename ItemType>
class SSDKObjectTableRow : public SObjectTableRow<ItemType>
{
public:
	SLATE_BEGIN_ARGS(SSDKObjectTableRow<ItemType>) {}
		SLATE_EVENT(FOnRowHovered, OnHovered)
		SLATE_EVENT(FOnRowHovered, OnUnhovered)
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, UUserWidget& InChildWidget, UListViewBase* InOwnerListView)
	{
		SObjectTableRow<ItemType>::Construct(
			typename SObjectTableRow<ItemType>::FArguments()
			.OnHovered(InArgs._OnHovered)
			.OnUnhovered(InArgs._OnUnhovered)
			[
				InArgs._Content.Widget
			],
			InOwnerTableView, InChildWidget, InOwnerListView);
	}

	// Begin ITableRow interface
	virtual ESelectionMode::Type GetSelectionMode() const override { return SObjectTableRow<ItemType>::OwnerTablePtr.Pin()->Private_GetSelectionMode(); }
	// End ITableRow interface

	// Begin SWidget Interface
	virtual bool SupportsKeyboardFocus() const override { return true; }

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		SObjectTableRow<ItemType>::OnMouseEnter(MyGeometry, MouseEvent);
	}

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override
	{
		SObjectTableRow<ItemType>::OnMouseLeave(MouseEvent);
	}

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override
	{
		return SObjectTableRow<ItemType>::OnMouseButtonDown(InMyGeometry, InMouseEvent);
	}

	virtual FReply OnTouchStarted(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent) override
	{
		USDKTileView* SDKTileView = Cast<USDKTileView>(SObjectTableRow<ItemType>::OwnerListView.Get());
		bool bUseScroll = SDKTileView != nullptr ? SDKTileView->GetEnableScroll() : true;

		if (bUseScroll)
		{
			return SObjectTableRow<ItemType>::OnTouchStarted(MyGeometry, InTouchEvent);
		}

		return FReply::Unhandled();
	}

	virtual FReply OnTouchEnded(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent) override
	{
		return SObjectTableRow<ItemType>::OnTouchEnded(MyGeometry, InTouchEvent);
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		USDKTileView* SDKTileView = Cast<USDKTileView>(SObjectTableRow<ItemType>::OwnerListView.Get());
		bool bUseScroll = SDKTileView != nullptr ? SDKTileView->GetEnableScroll() : true;

		if (bUseScroll)
		{
			return SObjectTableRow<ItemType>::OnMouseButtonDown(MyGeometry, MouseEvent);
		}
		else
		{
			SetChangedSelectOnMouseDown(false);

			// 인풋 시작할 때, 선택된 아이템
			FReply Reply = SObjectTableRow<ItemType>::OnMouseButtonDown(MyGeometry, MouseEvent);
			if (Reply.IsEventHandled() == true)
			{
				TSharedRef<ITypedTableView<ItemType>> OwnerTable = SObjectTableRow<ItemType>::OwnerTablePtr.Pin().ToSharedRef();

				const ESelectionMode::Type SelectionMode = GetSelectionMode();
				if (SObjectTableRow<ItemType>::IsItemSelectable() == true)
				{
					const ItemType* MyItemPtr = SObjectTableRow<ItemType>::GetItemForThis(OwnerTable);
					if (MyItemPtr != nullptr)
					{
						const ItemType& MyItem = *MyItemPtr;
						if (OwnerTable->Private_IsItemSelected(MyItem) == true)
						{
							if (SelectionMode != ESelectionMode::Multi)
							{
								OwnerTable->Private_ClearSelection();
							}

							OwnerTable->Private_SetItemSelection(MyItem, true, true);
							SetChangedSelectOnMouseDown(true);
						}

						if (GetChangedSelectOnMouseDown() == true)
						{
							OwnerTable->Private_SignalSelectionChanged(ESelectInfo::OnMouseClick);
						}
					}
				}
			}
		}

		return FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		FReply Reply = SObjectWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
		
		USDKTileView* SDKTileView = Cast<USDKTileView>(SObjectTableRow<ItemType>::OwnerListView.Get());
		bool bUseScroll = SDKTileView != nullptr ? SDKTileView->GetEnableScroll() : true;

		if (bUseScroll)
		{
			return SObjectTableRow<ItemType>::OnMouseButtonUp(MyGeometry, MouseEvent);
		}
		else
		{
			TSharedRef<ITypedTableView<ItemType>> OwnerTable = SObjectTableRow<ItemType>::OwnerTablePtr.Pin().ToSharedRef();

			const ItemType* MyItemPtr = SObjectTableRow<ItemType>::GetItemForThis(OwnerTable);
			if (MyItemPtr != nullptr)
			{
				if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					bool bSignalSelectionChanged = GetChangedSelectOnMouseDown();
					if (!GetChangedSelectOnMouseDown() && SObjectTableRow<ItemType>::IsItemSelectable() && MyGeometry.IsUnderLocation(MouseEvent.GetScreenSpacePosition()))
					{
						const ESelectionMode::Type SelectionMode = SObjectTableRow<ItemType>::GetSelectionMode();
						if (SelectionMode == ESelectionMode::SingleToggle)
						{
							OwnerTable->Private_ClearSelection();
							bSignalSelectionChanged = true;
						}
						else if (SelectionMode == ESelectionMode::Multi &&
							OwnerTable->Private_GetNumSelectedItems() > 1 && OwnerTable->Private_IsItemSelected(*MyItemPtr))
						{
							OwnerTable->Private_ClearSelection();
							OwnerTable->Private_SetItemSelection(*MyItemPtr, true, true);
							bSignalSelectionChanged = true;
						}
					}

					if (bSignalSelectionChanged)
					{
						OwnerTable->Private_SignalSelectionChanged(ESelectInfo::OnMouseClick);
						Reply = FReply::Handled();
					}

					if (OwnerTable->Private_OnItemClicked(*MyItemPtr))
					{
						Reply = FReply::Handled();
					}

					Reply = Reply.ReleaseMouseCapture();
				}
			}
		}
		
		return Reply;
	}
	// End SWidget Interface

protected:
	virtual void InitializeObjectRow() override
	{
		SObjectTableRow<ItemType>::InitializeObjectRow();
	}

	virtual void ResetObjectRow() override
	{
		SObjectTableRow<ItemType>::ResetObjectRow();
	}

	virtual void OnItemSelectionChanged(bool bIsItemSelected) override
	{
		SObjectTableRow<ItemType>::OnItemSelectionChanged(bIsItemSelected);
	}

	void SetChangedSelectOnMouseDown(bool bChangedSelect) { bChangedSelectOnMouseDown = bChangedSelect; }
	bool GetChangedSelectOnMouseDown() const { return bChangedSelectOnMouseDown; }

private:
	/** 마우스 이벤트 관련 변수가 Private이라 만든 변수 */ 
	bool bChangedSelectOnMouseDown = false;
};
