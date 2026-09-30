// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"

#include "Components/StaticMeshComponent.h"
#include "ProjectileSpawnRequest.h"

// Sets default values
AProjectile::AProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	ProjectileMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMeshComponent"));
	SetRootComponent(ProjectileMeshComponent);
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (!bHasSpawnRequest)
	{
		InitialLocation = GetActorLocation();
	}
}

void AProjectile::InitializeProjectile(const FProjectileSpawnRequest& Request, FProjectileReleaseDelegate ReleaseDelegate)
{
	InitialLocation = Request.Transform.GetLocation();
	InitialVelocity = Request.InitialVelocity;
	MaxDistance = Request.MaxDistance;
	OnReleaseRequested = MoveTemp(ReleaseDelegate);
	bHasSpawnRequest = true;

	// Deferred spawning applies the velocity after component initialization.
	if (IsActorInitialized())
	{
		ApplyInitialVelocity();
	}
}

void AProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (bHasSpawnRequest)
	{
		ApplyInitialVelocity();
	}
}

void AProjectile::ApplyInitialVelocity()
{
	ProjectileMovementComponent->Velocity = InitialVelocity;
	ProjectileMovementComponent->UpdateComponentVelocity();
}

void AProjectile::RequestRelease()
{
	if (OnReleaseRequested.IsBound())
	{
		OnReleaseRequested.Execute(this);
	}
	else
	{
		// Projectiles placed directly in a level can still run independently.
		Destroy();
	}
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (FVector::Dist(GetActorLocation(), InitialLocation) > MaxDistance)
	{
		RequestRelease();
	}
}

