// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Widgets/SWidget.h"
#include "Widgets/Views/STileView.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/Views/STableViewBase.h"

#include "Framework/Layout/Overscroll.h"
#include "Input/Events.h"

#include "UI/SDKUserWidget.h"


/**
 * A TileView widget is a list which arranges its items horizontally until there is no more space then creates a new row.
 * Items are spaced evenly horizontally.
 */
template <typename ItemType>
class ARENA_API SSDKTileView : public STileView<ItemType>
{
public:
	typedef typename TListTypeTraits< ItemType >::NullableType NullableItemType;

	typedef typename TSlateDelegates<ItemType>::FOnGenerateRow FOnGenerateRow;
	typedef typename TSlateDelegates<ItemType>::FOnItemScrolledIntoView FOnItemScrolledIntoView;
	typedef typename TSlateDelegates<ItemType>::FOnMouseButtonClick FOnMouseButtonClick;
	typedef typename TSlateDelegates<ItemType>::FOnMouseButtonDoubleClick FOnMouseButtonDoubleClick;
	typedef typename TSlateDelegates<NullableItemType>::FOnSelectionChanged FOnSelectionChanged;
	typedef typename TSlateDelegates<ItemType>::FIsSelectableOrNavigable FIsSelectableOrNavigable;
	typedef typename SListView<ItemType>::FOnItemToString_Debug FOnItemToString_Debug;

	using FOnWidgetToBeRemoved = typename SListView<ItemType>::FOnWidgetToBeRemoved;

	SLATE_BEGIN_ARGS(SSDKTileView<ItemType>)
		: _OnGenerateTile()
		, _OnTileReleased()
		, _ListItemsSource(static_cast<TArray<ItemType>*>(nullptr)) //@todo Slate Syntax: Initializing from nullptr without a cast
		, _ItemHeight(128)
		, _ItemWidth(128)
		, _ItemAlignment(EListItemAlignment::EvenlyDistributed)
		, _OnContextMenuOpening()
		, _OnMouseButtonClick()
		, _OnMouseButtonDoubleClick()
		, _OnSelectionChanged()
		, _OnIsSelectableOrNavigable()
		, _SelectionMode(ESelectionMode::Single)
		, _ClearSelectionOnClick(true)
		, _ExternalScrollbar()
		, _Orientation(Orient_Vertical)
		, _EnableAnimatedScrolling(false)
		, _ScrollbarVisibility(EVisibility::Visible)
		, _ScrollbarDragFocusCause(EFocusCause::Mouse)
		, _AllowOverscroll(EAllowOverscroll::Yes)
		, _ConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible)
		, _WheelScrollMultiplier(GetGlobalScrollAmount())
		, _HandleGamepadEvents(true)
		, _HandleDirectionalNavigation(true)
		, _IsFocusable(true)
		, _OnItemToString_Debug()
		, _OnEnteredBadState()
		, _WrapHorizontalNavigation(true)
		, _EnableScroll(true)
	{
		this->_Clipping = EWidgetClipping::ClipToBounds;
	}

	SLATE_EVENT(FOnGenerateRow, OnGenerateTile)

		SLATE_EVENT(FOnWidgetToBeRemoved, OnTileReleased)

		SLATE_EVENT(FOnTableViewScrolled, OnTileViewScrolled)

		SLATE_EVENT(FOnItemScrolledIntoView, OnItemScrolledIntoView)

		SLATE_ARGUMENT(const TArray<ItemType>*, ListItemsSource)

		SLATE_ATTRIBUTE(float, ItemHeight)

		SLATE_ATTRIBUTE(float, ItemWidth)

		SLATE_ATTRIBUTE(EListItemAlignment, ItemAlignment)

		SLATE_EVENT(FOnContextMenuOpening, OnContextMenuOpening)

		SLATE_EVENT(FOnMouseButtonClick, OnMouseButtonClick)

		SLATE_EVENT(FOnMouseButtonDoubleClick, OnMouseButtonDoubleClick)

		SLATE_EVENT(FOnSelectionChanged, OnSelectionChanged)

		SLATE_EVENT(FIsSelectableOrNavigable, OnIsSelectableOrNavigable)

		SLATE_ATTRIBUTE(ESelectionMode::Type, SelectionMode)

		SLATE_ARGUMENT (bool, ClearSelectionOnClick)

		SLATE_ARGUMENT(TSharedPtr<SScrollBar>, ExternalScrollbar)

		SLATE_ARGUMENT(EOrientation, Orientation)

		SLATE_ARGUMENT(bool, EnableAnimatedScrolling)

		SLATE_ARGUMENT(TOptional<double>, FixedLineScrollOffset)

		SLATE_ATTRIBUTE(EVisibility, ScrollbarVisibility)

		SLATE_ARGUMENT(EFocusCause, ScrollbarDragFocusCause)

		SLATE_ARGUMENT(EAllowOverscroll, AllowOverscroll);

		SLATE_ARGUMENT(EConsumeMouseWheel, ConsumeMouseWheel);

		SLATE_ARGUMENT(float, WheelScrollMultiplier);

		SLATE_ARGUMENT(bool, HandleGamepadEvents);

		SLATE_ARGUMENT(bool, HandleDirectionalNavigation);

		SLATE_ATTRIBUTE(bool, IsFocusable)

		/** Assign this to get more diagnostics from the list view. */
		SLATE_EVENT(FOnItemToString_Debug, OnItemToString_Debug)

		SLATE_EVENT(FOnTableViewBadState, OnEnteredBadState);

		SLATE_ARGUMENT(bool, WrapHorizontalNavigation);

		SLATE_ARGUMENT(bool, EnableScroll);

	SLATE_END_ARGS()

/**
 * Construct this widget
 *
 * @param	InArgs	The declaration data for this widget
 */
	void Construct(const typename SSDKTileView<ItemType>::FArguments& InArgs)
	{
		this->Clipping = InArgs._Clipping;

		this->OnGenerateRow = InArgs._OnGenerateTile;
		this->OnRowReleased = InArgs._OnTileReleased;
		this->OnItemScrolledIntoView = InArgs._OnItemScrolledIntoView;

		this->ItemsSource = InArgs._ListItemsSource;
		this->OnContextMenuOpening = InArgs._OnContextMenuOpening;
		this->OnClick = InArgs._OnMouseButtonClick;
		this->OnDoubleClick = InArgs._OnMouseButtonDoubleClick;
		this->OnSelectionChanged = InArgs._OnSelectionChanged;
		this->OnIsSelectableOrNavigable = InArgs._OnIsSelectableOrNavigable;
		this->SelectionMode = InArgs._SelectionMode;

		this->bClearSelectionOnClick = InArgs._ClearSelectionOnClick;

		this->AllowOverscroll = InArgs._AllowOverscroll;
		this->ConsumeMouseWheel = InArgs._ConsumeMouseWheel;

		this->WheelScrollMultiplier = InArgs._WheelScrollMultiplier;

		this->bHandleGamepadEvents = InArgs._HandleGamepadEvents;
		this->bHandleDirectionalNavigation = InArgs._HandleDirectionalNavigation;
		this->IsFocusable = InArgs._IsFocusable;

		this->bEnableAnimatedScrolling = InArgs._EnableAnimatedScrolling;
		this->FixedLineScrollOffset = InArgs._FixedLineScrollOffset;

		this->OnItemToString_Debug = InArgs._OnItemToString_Debug.IsBound()
			? InArgs._OnItemToString_Debug
			: SListView< ItemType >::GetDefaultDebugDelegate();
		this->OnEnteredBadState = InArgs._OnEnteredBadState;

		this->bWrapHorizontalNavigation = InArgs._WrapHorizontalNavigation;

		this->bEnableScroll = InArgs._EnableScroll;

		// Check for any parameters that the coder forgot to specify.
		FString ErrorString;
		{
			if (!this->OnGenerateRow.IsBound())
			{
				ErrorString += TEXT("Please specify an OnGenerateTile. \n");
			}

			if (this->ItemsSource == nullptr)
			{
				ErrorString += TEXT("Please specify a ListItemsSource. \n");
			}
		}

		if (!ErrorString.IsEmpty())
		{
			// Let the coder know what they forgot
			this->ChildSlot
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(ErrorString))
				];
		}
		else
		{
			// Make the TableView
			this->ConstructChildren(InArgs._ItemWidth, InArgs._ItemHeight, InArgs._ItemAlignment, TSharedPtr<SHeaderRow>(), InArgs._ExternalScrollbar, InArgs._Orientation, InArgs._OnTileViewScrolled);
			if (this->ScrollBar.IsValid())
			{
				this->ScrollBar->SetDragFocusCause(InArgs._ScrollbarDragFocusCause);
				this->ScrollBar->SetUserVisibility(InArgs._ScrollbarVisibility);
			}
			this->AddMetadata(MakeShared<TTableViewMetadata<ItemType>>(this->SharedThis(this)));
		}
	}

	SSDKTileView(ETableViewMode::Type InListMode = ETableViewMode::Tile)
		: STileView<ItemType>(InListMode), bEnableScroll(false)
	{
	}


	/*virtual TSharedPtr<ITableRow> WidgetFromItem(const ItemType& InItem) const override
	{
		return WidgetGenerator.GetWidgetForItem(InItem);
	}*/

public:
	// Begin STableViewBase Interface
	virtual FReply OnTouchStarted(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent) override
	{
		if (bEnableScroll == false)
		{
			return FReply::Unhandled();
		}

		return STableViewBase::OnTouchStarted(MyGeometry, InTouchEvent);
	}

	virtual FReply OnTouchMoved(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent) override
	{
		if (bEnableScroll == false)
		{
			return FReply::Unhandled();
		}

		return STableViewBase::OnTouchMoved(MyGeometry, InTouchEvent);
	}

	virtual FReply OnTouchEnded(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent) override
	{
		if (bEnableScroll == false)
		{
			return FReply::Unhandled();
		}

		return STableViewBase::OnTouchEnded(MyGeometry, InTouchEvent);
	}

	virtual TSharedRef<class SWidget> GetScrollWidget() override
	{
		return STableViewBase::GetScrollWidget();
	}
	// End STableViewBase Interface

public:
	void SetEnableScroll(bool bEnable)
	{
		bEnableScroll = bEnable;

		// SWidget : Scroll Visibility 처리
		TSharedRef<class SWidget> ScrollWidget = GetScrollWidget();
		ScrollWidget.Get().SetVisibility(bEnable ? EVisibility::Visible : EVisibility::SelfHitTestInvisible);

		// OverScroll 활성화 처리
		this->AllowOverscroll = bEnable ? EAllowOverscroll::Yes : EAllowOverscroll::No;

		// SScrollBar : Drag Focus 처리
		if (this->ScrollBar.IsValid() == true)
		{
			this->ScrollBar.Get()->SetDragFocusCause(bEnable ? EFocusCause::Mouse : EFocusCause::Cleared);
			this->ScrollBar.Get()->SetVisibility(bEnable ? EVisibility::Visible : EVisibility::Collapsed);
		}
	}

	void SetScrollBarStyle(FScrollBarStyle* Style)
	{
		if (this->ScrollBar.IsValid() == true)
		{
			this->ScrollBar->SetStyle(Style);
		}
	}

	void SetScrollbarThickness(const FVector2D& NewScrollbarThickness)
	{
		if (this->ScrollBar.IsValid() == true)
		{
			this->ScrollBar->SetThickness(NewScrollbarThickness);
		}
	}

	void SetScrollbarPadding(const FMargin& NewScrollbarPadding)
	{
		if (this->ScrollBar.IsValid() == true)
		{
			this->ScrollBar->SetPadding(NewScrollbarPadding);
		}
	}

private:
	bool bEnableScroll;

};
