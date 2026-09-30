#include "MassLabPlayer.h"

#include "Camera/CameraComponent.h"

AMassLabPlayer::AMassLabPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SetRootComponent(Camera);
	Camera->bUsePawnControlRotation = false;
	Camera->SetRelativeRotation(FRotator(-90.0, 0.0, 0.0));
}

void AMassLabPlayer::BeginPlay()
{
	Super::BeginPlay();

	// The camera is the root, so the spawn transform can replace its constructor rotation.
	SetActorRotation(FRotator(-90.0, 0.0, 0.0));
	AddActorWorldOffset(FVector(0.0, 0.0, InitialHeightOffset));
}

void AMassLabPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// A camera-only pawn applies the input itself; it has no movement component or collision.
	const FVector MovementInput = ConsumeMovementInputVector().GetClampedToMaxSize(1.0);
	if (!MovementInput.IsNearlyZero())
	{
		AddActorWorldOffset(MovementInput * FMath::Max(0.0f, MovementSpeed) * DeltaTime);
	}
}
