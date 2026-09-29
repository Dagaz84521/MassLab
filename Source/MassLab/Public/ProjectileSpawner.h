// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/SubclassOf.h"
#include "ProjectileSpawner.generated.h"

class AProjectile;

UCLASS()
class MASSLAB_API AProjectileSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectileSpawner();

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION()
	void SpawnAProjectile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	TSubclassOf<AProjectile> ProjectileClass;
	
	UPROPERTY(EditAnywhere)
	double SpawnInterval = 2.;
	
	UPROPERTY(EditAnywhere)
	int32 SpawnNumEveryInterval = 1;
	
	UPROPERTY(EditAnywhere)
	double InitialSpeed = 100.;

private:
	void TickProjectileSpawning(float DeltaTime);

	double TimeSinceLastSpawn = 0.0;
};
