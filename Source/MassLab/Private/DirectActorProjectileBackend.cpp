#include "DirectActorProjectileBackend.h"

#include "Engine/World.h"
#include "Projectile.h"

void UDirectActorProjectileBackend::SpawnBatch(TConstArrayView<FProjectileSpawnRequest> Requests, const FProjectileSpawnConfig& Config)
{
	UWorld* World = GetWorld();
	if (!World || !Config.ActorClass)
	{
		return;
	}

	for (const FProjectileSpawnRequest& Request : Requests)
	{
		AProjectile* Projectile = World->SpawnActorDeferred<AProjectile>(Config.ActorClass.Get(), Request.Transform, Config.Owner.Get());
		if (!Projectile)
		{
			continue;
		}

		ActiveProjectiles.Add(Projectile);
		Projectile->OnEndPlay.AddDynamic(this, &UDirectActorProjectileBackend::HandleProjectileEndPlay);
		Projectile->InitializeProjectile(Request, FProjectileReleaseDelegate::CreateUObject(this, &UDirectActorProjectileBackend::ReleaseProjectile));
		Projectile->FinishSpawning(Request.Transform, false, nullptr, ESpawnActorScaleMethod::MultiplyWithRoot);

		// Construction scripts or BeginPlay can destroy the Actor immediately.
		if (!IsValid(Projectile) || Projectile->IsActorBeingDestroyed())
		{
			ActiveProjectiles.Remove(Projectile);
		}
	}
}

void UDirectActorProjectileBackend::ReleaseProjectile(AProjectile* Projectile)
{
	if (IsValid(Projectile) && ActiveProjectiles.Contains(Projectile))
	{
		Projectile->Destroy();
	}
}

void UDirectActorProjectileBackend::HandleProjectileEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	ActiveProjectiles.Remove(Cast<AProjectile>(Actor));
}

void UDirectActorProjectileBackend::Reset()
{
	// Destroying an Actor invokes EndPlay and removes it from ActiveProjectiles.
	const TArray<TObjectPtr<AProjectile>> ProjectilesToDestroy = ActiveProjectiles.Array();
	for (AProjectile* Projectile : ProjectilesToDestroy)
	{
		if (IsValid(Projectile))
		{
			Projectile->Destroy();
		}
	}
	ActiveProjectiles.Empty();
}

int32 UDirectActorProjectileBackend::GetActiveProjectileCount() const
{
	return ActiveProjectiles.Num();
}
