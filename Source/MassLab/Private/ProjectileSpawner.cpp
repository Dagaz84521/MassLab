// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileSpawner.h"
#include "Projectile.h"
#include "Engine/World.h"
#include "ProjectileSpawnRequest.h"
#include "ProjectileSubsystem.h"
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
	if (!bAutoSpawn)
	{
		TimeSinceLastSpawn = 0.0;
		return;
	}
	if (SpawnInterval <= 0.0)
	{
		return;
	}

	TimeSinceLastSpawn += DeltaTime;
	if (TimeSinceLastSpawn >= SpawnInterval)
	{
		TimeSinceLastSpawn = FMath::Fmod(TimeSinceLastSpawn, SpawnInterval);
		SpawnProjectiles();
	}
}

void AProjectileSpawner::SpawnProjectiles()
{
	UWorld* World = GetWorld();
	if (!World || !ProjectileClass || SpawnNumEveryInterval <= 0)
	{
		return;
	}
	UProjectileSubsystem* ProjectileSubsystem = World->GetSubsystem<UProjectileSubsystem>();
	if (!ProjectileSubsystem)
	{
		return;
	}

	FProjectileSpawnConfig Config;
	Config.ActorClass = ProjectileClass;
	Config.Owner = this;

	const FVector SpawnLocation = GetActorLocation();
	const double SpawnDegreeIncrement = 360.0 / SpawnNumEveryInterval;
	TArray<FProjectileSpawnRequest> Requests;
	Requests.Reserve(SpawnNumEveryInterval);
	for (int32 i = 0; i < SpawnNumEveryInterval; ++i)
	{
		const FRotator SpawnRotation(0.0, SpawnDegreeIncrement * i, 0.0);
		FProjectileSpawnRequest& Request = Requests.AddDefaulted_GetRef();
		Request.Transform = FTransform(SpawnRotation, SpawnLocation);
		Request.InitialVelocity = SpawnRotation.Vector() * InitialSpeed;
		Request.MaxDistance = MaxDistance;
	}

	ProjectileSubsystem->SpawnBatch(Requests, Config);
}
