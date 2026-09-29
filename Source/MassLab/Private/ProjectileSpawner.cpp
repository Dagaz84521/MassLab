// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileSpawner.h"
#include "Projectile.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
// Sets default values
AProjectileSpawner::AProjectileSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	ProjectileClass = AProjectile::StaticClass();

}

// Called when the game starts or when spawned
void AProjectileSpawner::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AProjectileSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TickProjectileSpawning(DeltaTime);
}

void AProjectileSpawner::TickProjectileSpawning(float DeltaTime)
{
	if (SpawnInterval <= 0.0)
	{
		return;
	}

	TimeSinceLastSpawn += DeltaTime;
	if (TimeSinceLastSpawn >= SpawnInterval)
	{
		TimeSinceLastSpawn = FMath::Fmod(TimeSinceLastSpawn, SpawnInterval);
		SpawnAProjectile();
	}
}

void AProjectileSpawner::SpawnAProjectile()
{
	UWorld* World = GetWorld();
	if (!World || !ProjectileClass)
	{
		return;
	}

	const FRotator SpawnRotation(0.0, FMath::FRandRange(0.0, 360.0), 0.0);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	AProjectile* Projectile = World->SpawnActor<AProjectile>(ProjectileClass.Get(), GetActorLocation(), SpawnRotation, SpawnParams);
	if (!Projectile)
	{
		return;
	}

	if (UProjectileMovementComponent* Movement = Projectile->FindComponentByClass<UProjectileMovementComponent>())
	{
		Movement->SetVelocityInLocalSpace(FVector(InitialSpeed, 0.0, 0.0));
	}
}
