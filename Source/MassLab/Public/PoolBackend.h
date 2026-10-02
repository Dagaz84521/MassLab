#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "ProjectileBackend.h"
#include "PoolBackend.generated.h"

class AProjectile;

// A reflected wrapper lets each projectile class own a GC-visible free list.
USTRUCT()
struct FProjectilePool
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<TObjectPtr<AProjectile>> InactiveProjectiles;
};

UCLASS(meta = (DisplayName = "Pooled Actor Projectile Backend"))
class MASSLAB_API UPoolBackend : public UProjectileBackend
{
	GENERATED_BODY()

public:
	virtual void SpawnBatch(TConstArrayView<FProjectileSpawnRequest> Requests, const FProjectileSpawnConfig& Config) override;
	virtual void Prewarm(int32 Count, const FProjectileSpawnConfig& Config) override;
	virtual void Reset() override;
	virtual void Shutdown() override;
	virtual int32 GetActiveProjectileCount() const override;
	int32 GetInactiveProjectileCount() const;

private:
	AProjectile* CreateProjectile(const FProjectileSpawnConfig& Config);
	AProjectile* AcquireProjectile(const FProjectileSpawnConfig& Config);
	void ReleaseProjectile(AProjectile* Projectile);

	UFUNCTION()
	void HandleProjectileEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	UPROPERTY(Transient)
	TMap<TSubclassOf<AProjectile>, FProjectilePool> Pools;

	UPROPERTY(Transient)
	TSet<TObjectPtr<AProjectile>> ActiveProjectiles;
};
