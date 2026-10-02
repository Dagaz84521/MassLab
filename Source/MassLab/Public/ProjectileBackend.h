#pragma once

#include "CoreMinimal.h"
#include "ProjectileSpawnRequest.h"
#include "UObject/Object.h"
#include "ProjectileBackend.generated.h"

class UWorld;

UCLASS(Abstract)
class MASSLAB_API UProjectileBackend : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(UWorld& InWorld);
	virtual void Shutdown();
	virtual UWorld* GetWorld() const override;

	virtual void SpawnBatch(TConstArrayView<FProjectileSpawnRequest> Requests, const FProjectileSpawnConfig& Config)
		PURE_VIRTUAL(UProjectileBackend::SpawnBatch, );
	// Backends that do not reuse Actors have no preparation work.
	virtual void Prewarm(int32 Count, const FProjectileSpawnConfig& Config) {}
	virtual void Reset() PURE_VIRTUAL(UProjectileBackend::Reset, );
	virtual int32 GetActiveProjectileCount() const PURE_VIRTUAL(UProjectileBackend::GetActiveProjectileCount, return 0;);

private:
	TWeakObjectPtr<UWorld> SimulationWorld;
};
