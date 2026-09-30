#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class AActor;
class AProjectile;

// Per-projectile data shared by all spawning backends.
struct FProjectileSpawnRequest
{
	FTransform Transform = FTransform::Identity;
	FVector InitialVelocity = FVector::ZeroVector;
	double MaxDistance = 1000.0;
};

// Optional Actor configuration for the current Blueprint-based projectile type.
struct FProjectileSpawnConfig
{
	TSubclassOf<AProjectile> ActorClass;
	TWeakObjectPtr<AActor> Owner;
};
