#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "ProjectileBackend.h"
#include "DirectActorProjectileBackend.generated.h"

class AProjectile;

UCLASS(meta = (DisplayName = "Direct Actor Projectile Backend"))
class MASSLAB_API UDirectActorProjectileBackend : public UProjectileBackend
{
	GENERATED_BODY()

public:
	virtual void SpawnBatch(TConstArrayView<FProjectileSpawnRequest> Requests, const FProjectileSpawnConfig& Config) override;
	virtual void Reset() override;
	virtual int32 GetActiveProjectileCount() const override;

private:
	void ReleaseProjectile(AProjectile* Projectile);

	UFUNCTION()
	void HandleProjectileEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	UPROPERTY(Transient)
	TSet<TObjectPtr<AProjectile>> ActiveProjectiles;
};
