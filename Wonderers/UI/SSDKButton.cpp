
#include "UI/SSDKButton.h"

#include "Engine/SDKGameTask.h"

//#include "Framework/Application/SlateApplication.h"

static FName SButtonTypeName("SSDKButton");

void SSDKButton::Construct(const FArguments& InArgs)
{
	bIsPressed = false;

	// Text overrides button content. If nothing is specified, put an null widget in the button.
	// Null content makes the button enter a special mode where it will ask to be as big as the image used for its border.
	struct
	{
		TSharedRef<SWidget> operator()(const FArguments& InOpArgs) const
		{
			if ((InOpArgs._Content.Widget == SNullWidget::NullWidget) && (InOpArgs._Text.IsBound() || !InOpArgs._Text.Get().IsEmpty()))
			{
				return SNew(STextBlock)
					.Visibility(EVisibility::HitTestInvisible)
					.Text(InOpArgs._Text)
					.TextStyle(InOpArgs._TextStyle)
					.TextShapingMethod(InOpArgs._TextShapingMethod)
					.TextFlowDirection(InOpArgs._TextFlowDirection);
			}
			else
			{
				return InOpArgs._Content.Widget;
			}
		}
	} DetermineContent;

	SBorder::Construct(SBorder::FArguments()
		.ContentScale(InArgs._ContentScale)
		.DesiredSizeScale(InArgs._DesiredSizeScale)
		.BorderBackgroundColor(InArgs._ButtonColorAndOpacity)
		.ForegroundColor(InArgs._ForegroundColor)
		.BorderImage(this, &SButton::GetBorder)
		.HAlign(InArgs._HAlign)
		.VAlign(InArgs._VAlign)
		.Padding(TAttribute<FMargin>(this, &SSDKButton::GetCombinedPadding))
		.ShowEffectWhenDisabled(TAttribute<bool>(this, &SSDKButton::GetShowDisabledEffect))
		[
			DetermineContent(InArgs)
		]
	);

	// Only do this if we're exactly an SSDKButton
	if (GetType() == SButtonTypeName)
	{
		SetCanTick(false);
	}

	ContentPadding = InArgs._ContentPadding;

	SetButtonStyle(InArgs._ButtonStyle);

	bIsFocusable = InArgs._IsFocusable;

	OnClicked = InArgs._OnClicked;
	OnPressed = InArgs._OnPressed;
	OnReleased = InArgs._OnReleased;
	OnHovered = InArgs._OnHovered;
	OnUnhovered = InArgs._OnUnhovered;

	ClickMethod = InArgs._ClickMethod;
	TouchMethod = InArgs._TouchMethod;
	PressMethod = InArgs._PressMethod;

	HoveredSound = InArgs._HoveredSoundOverride.Get(Style->HoveredSlateSound);
	PressedSound = InArgs._PressedSoundOverride.Get(Style->PressedSlateSound);
}

FMargin SSDKButton::GetCombinedPadding() const
{
	return (IsPressed())
		? ContentPadding.Get() + PressedBorderPadding
		: ContentPadding.Get() + BorderPadding;
}

bool SSDKButton::GetShowDisabledEffect() const
{
	return DisabledImage->DrawAs == ESlateBrushDrawType::NoDrawType;
}

int32 SSDKButton::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	bool bEnabled = ShouldBeEnabled(bParentEnabled);
	bool bShowDisabledEffect = GetShowDisabledEffect();

	const FSlateBrush* BrushResource = !bShowDisabledEffect && !bEnabled ? DisabledImage : GetBorder();

	ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;

	if (BrushResource && BrushResource->DrawAs != ESlateBrushDrawType::NoDrawType)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			BrushResource,
			DrawEffects,
			BrushResource->GetTint(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint() * BorderBackgroundColor.Get().GetColor(InWidgetStyle) * ColorAndOpacity.Get()
		);
	}

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bEnabled);
}

bool SSDKButton::SupportsKeyboardFocus() const
{
	return false;
}

FReply SSDKButton::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	/*if (IsPressed() && IsPreciseTapOrClick(MouseEvent) && FSlateApplication::Get().HasTraveledFarEnoughToTriggerDrag(MouseEvent, PressedScreenSpacePosition))
	{
		Release();
	}*/

	return FReply::Unhandled();
}

FReply SSDKButton::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (OnRightMouseButtonDown.IsBound())
	{
		if (IsEnabled() && (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton))
		{
			FReply Reply = FReply::Unhandled();
			Press();
			PressedScreenSpacePosition = MouseEvent.GetScreenSpacePosition();
			EButtonClickMethod::Type InputClickMethod = GetClickMethodFromInputType(MouseEvent);
			if (InputClickMethod == EButtonClickMethod::MouseDown)
			{
				OnRightMouseButtonDown.Broadcast();
				Reply = FReply::Handled();
				ensure(Reply.IsEventHandled() == true);
			}
			else if (InputClickMethod == EButtonClickMethod::PreciseClick)
			{
				Reply = FReply::Handled();
			}
			else
			{
				Reply = FReply::Handled().CaptureMouse(AsShared());
			}
			Invalidate(EInvalidateWidget::Layout);
			return Reply;
		}
	}
	return SButton::OnMouseButtonDown(MyGeometry, MouseEvent);
}
