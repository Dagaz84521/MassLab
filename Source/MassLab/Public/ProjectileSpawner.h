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
	
	UFUNCTION(BlueprintCallable)
	void SpawnProjectiles();

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
	
	UPROPERTY(EditAnywhere)
	double MaxDistance = 1000.;
	
	UPROPERTY(EditAnywhere)
	bool bAutoSpawn = false;

private:
	void TickProjectileSpawning(float DeltaTime);

	double TimeSinceLastSpawn = 0.0;
	
public:
	UFUNCTION(BlueprintCallable)
	void SetSpawnInterval(double NewSpawnInterval) { SpawnInterval = NewSpawnInterval; }
	UFUNCTION(BlueprintCallable)
	void SetSpawnNumEveryInterval(int32 NewSpawnNumEveryInterval) { SpawnNumEveryInterval = NewSpawnNumEveryInterval; }
	UFUNCTION(BlueprintCallable)
	void SetMaxDistance(double NewMaxDistance) { MaxDistance = NewMaxDistance; }
	UFUNCTION(BlueprintCallable)
	void SetInitialSpeed(double NewInitialSpeed) { InitialSpeed = NewInitialSpeed; }
	UFUNCTION(BlueprintCallable)
	void SetAutoSpawn(bool bNewAutoSpawn) { bAutoSpawn = bNewAutoSpawn;}
	
	UFUNCTION(BlueprintPure)
	double GetSpawnInterval() { return SpawnInterval; }
	UFUNCTION(BlueprintPure)
	int32 GetSpawnNumEveryInterval() { return SpawnNumEveryInterval; }
	UFUNCTION(BlueprintPure)
	double GetMaxDistance() { return MaxDistance; }
	UFUNCTION(BlueprintPure)
	double GetInitialSpeed() { return InitialSpeed; }
	UFUNCTION(BlueprintPure)
	bool GetAutoSpawn() { return bAutoSpawn; }
};
