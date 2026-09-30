#include "MassLabPlayerController.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"

AMassLabPlayerController::AMassLabPlayerController()
{
	bShowMouseCursor = true;
}

void AMassLabPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
}

void AMassLabPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!ensureMsgf(EnhancedInput, TEXT("MassLabPlayerController requires an EnhancedInputComponent.")))
	{
		return;
	}

	if (!MovementMappingContext)
	{
		CreateInputMappings();
	}

	EnhancedInput->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this, &AMassLabPlayerController::MoveForward);
	EnhancedInput->BindAction(MoveRightAction, ETriggerEvent::Triggered, this, &AMassLabPlayerController::MoveRight);
	EnhancedInput->BindAction(MoveVerticalAction, ETriggerEvent::Triggered, this, &AMassLabPlayerController::MoveVertical);

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->AddMappingContext(MovementMappingContext, 0);
		}
	}
}

void AMassLabPlayerController::CreateInputMappings()
{
	MovementMappingContext = NewObject<UInputMappingContext>(this);

	auto CreateAxisAction = [this]()
	{
		UInputAction* Action = NewObject<UInputAction>(this);
		Action->ValueType = EInputActionValueType::Axis1D;
		// Opposite keys cancel instead of choosing whichever mapping was processed last.
		Action->AccumulationBehavior = EInputActionAccumulationBehavior::Cumulative;
		return Action;
	};
	MoveForwardAction = CreateAxisAction();
	MoveRightAction = CreateAxisAction();
	MoveVerticalAction = CreateAxisAction();

	auto MapNegativeKey = [this](UInputAction* Action, FKey Key)
	{
		FEnhancedActionKeyMapping& Mapping = MovementMappingContext->MapKey(Action, Key);
		Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(MovementMappingContext));
	};

	MovementMappingContext->MapKey(MoveForwardAction, EKeys::W);
	MapNegativeKey(MoveForwardAction, EKeys::S);
	MovementMappingContext->MapKey(MoveRightAction, EKeys::D);
	MapNegativeKey(MoveRightAction, EKeys::A);
	MovementMappingContext->MapKey(MoveVerticalAction, EKeys::SpaceBar);
	MapNegativeKey(MoveVerticalAction, EKeys::LeftShift);
	MapNegativeKey(MoveVerticalAction, EKeys::RightShift);
}

void AMassLabPlayerController::MoveForward(const FInputActionValue& Value)
{
	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->AddMovementInput(FVector::ForwardVector, Value.Get<float>());
	}
}

void AMassLabPlayerController::MoveRight(const FInputActionValue& Value)
{
	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->AddMovementInput(FVector::RightVector, Value.Get<float>());
	}
}

void AMassLabPlayerController::MoveVertical(const FInputActionValue& Value)
{
	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->AddMovementInput(FVector::UpVector, Value.Get<float>());
	}
}

void AMassLabPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (MovementMappingContext)
			{
				InputSubsystem->RemoveMappingContext(MovementMappingContext);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}
