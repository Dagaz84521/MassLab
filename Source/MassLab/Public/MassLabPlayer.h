#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MassLabPlayer.generated.h"

class UCameraComponent;

UCLASS()
class MASSLAB_API AMassLabPlayer : public APawn
{
	GENERATED_BODY()

public:
	AMassLabPlayer();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	/** Movement speed in Unreal units (centimeters per second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0"))
	float MovementSpeed = 1500.0f;

	/** Height added above the spawn point when play starts, in centimeters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (Units = "cm"))
	float InitialHeightOffset = 2000.0f;
};
