// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Projectile.generated.h"

class AProjectile;
class UStaticMeshComponent;
struct FProjectileSpawnRequest;

DECLARE_DELEGATE_OneParam(FProjectileReleaseDelegate, AProjectile*);

UCLASS()
class MASSLAB_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectile();
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// The backend positions the Actor; this initializes its launch and release state.
	void InitializeProjectile(const FProjectileSpawnRequest& Request, FProjectileReleaseDelegate ReleaseDelegate);
	double GetMaxDistance() const { return MaxDistance; }
	
	FORCEINLINE void SetInitialLocation(const FVector& Location) { InitialLocation = Location; }
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ProjectileMeshComponent;
	
	UPROPERTY(EditAnywhere)
	double MaxDistance = 1000.;
	
	UPROPERTY()
	FVector InitialLocation;

private:
	void ApplyInitialVelocity();
	void RequestRelease();

	FProjectileReleaseDelegate OnReleaseRequested;
	FVector InitialVelocity = FVector::ZeroVector;
	bool bHasSpawnRequest = false;
};
