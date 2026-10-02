#include "PoolBackend.h"

#include "Engine/World.h"
#include "Projectile.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

void UPoolBackend::SpawnBatch(TConstArrayView<FProjectileSpawnRequest> Requests, const FProjectileSpawnConfig& Config)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PoolBackend_SpawnBatch);
	if (!GetWorld() || !Config.ActorClass)
	{
		return;
	}

	for (const FProjectileSpawnRequest& Request : Requests)
	{
		AProjectile* Projectile = AcquireProjectile(Config);
		if (!Projectile)
		{
			continue;
		}

		// Track before activation: enabling collision can invoke gameplay callbacks.
		ActiveProjectiles.Add(Projectile);
		Projectile->SetOwner(Config.Owner.Get());
		Projectile->InitializeProjectile(Request, FProjectileReleaseDelegate::CreateUObject(this, &UPoolBackend::ReleaseProjectile));
		Projectile->ActivateFromPool(Request.Transform);
	}
}

AProjectile* UPoolBackend::CreateProjectile(const FProjectileSpawnConfig& Config)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PoolBackend_CreateProjectile);
	UWorld* World = GetWorld();
	if (!World || !Config.ActorClass || Config.ActorClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return nullptr;
	}

	// Create once at unit spawn scale so reuse can preserve the Blueprint root scale.
	AProjectile* Projectile = World->SpawnActorDeferred<AProjectile>(Config.ActorClass.Get(), FTransform::Identity,
		Config.Owner.Get(), nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return nullptr;
	}

	ActiveProjectiles.Add(Projectile);
	Projectile->OnEndPlay.AddDynamic(this, &UPoolBackend::HandleProjectileEndPlay);
	Projectile->InitializeProjectile(FProjectileSpawnRequest(), FProjectileReleaseDelegate::CreateUObject(this, &UPoolBackend::ReleaseProjectile));
	Projectile->FinishSpawning(FTransform::Identity, false, nullptr, ESpawnActorScaleMethod::MultiplyWithRoot);

	if (!IsValid(Projectile) || Projectile->IsActorBeingDestroyed() || !ActiveProjectiles.Contains(Projectile))
	{
		ActiveProjectiles.Remove(Projectile);
		return nullptr;
	}

	ActiveProjectiles.Remove(Projectile);
	Projectile->DeactivateForPool();
	Projectile->SetOwner(nullptr);
	return IsValid(Projectile) && !Projectile->IsActorBeingDestroyed() ? Projectile : nullptr;
}

AProjectile* UPoolBackend::AcquireProjectile(const FProjectileSpawnConfig& Config)
{
	if (FProjectilePool* Pool = Pools.Find(Config.ActorClass))
	{
		while (!Pool->InactiveProjectiles.IsEmpty())
		{
			// Keep array capacity: frequent reuse should not allocate container storage.
			AProjectile* Projectile = Pool->InactiveProjectiles.Pop(EAllowShrinking::No);
			if (IsValid(Projectile) && !Projectile->IsActorBeingDestroyed())
			{
				return Projectile;
			}
		}
	}
	return CreateProjectile(Config);
}

void UPoolBackend::Prewarm(int32 Count, const FProjectileSpawnConfig& Config)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PoolBackend_Prewarm);
	if (!GetWorld() || !Config.ActorClass || Count <= 0)
	{
		return;
	}

	// Count is the desired free capacity, so repeated prewarming is idempotent.
	const FProjectilePool* Pool = Pools.Find(Config.ActorClass);
	const int32 ExistingCount = Pool ? Pool->InactiveProjectiles.Num() : 0;
	Pools.FindOrAdd(Config.ActorClass).InactiveProjectiles.Reserve(FMath::Max(Count, ExistingCount));
	for (int32 Index = ExistingCount; Index < Count; ++Index)
	{
		AProjectile* Projectile = CreateProjectile(Config);
		if (!Projectile)
		{
			break;
		}
		Pools.FindOrAdd(Config.ActorClass).InactiveProjectiles.Add(Projectile);
	}
}

void UPoolBackend::ReleaseProjectile(AProjectile* Projectile)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PoolBackend_ReleaseProjectile);
	// Removing first makes repeated/reentrant release requests harmless.
	if (!ActiveProjectiles.Remove(Projectile) || !IsValid(Projectile) || Projectile->IsActorBeingDestroyed())
	{
		return;
	}
	Projectile->DeactivateForPool();
	Projectile->SetOwner(nullptr);
	if (IsValid(Projectile) && !Projectile->IsActorBeingDestroyed())
	{
		Pools.FindOrAdd(Projectile->GetClass()).InactiveProjectiles.Add(Projectile);
	}
}

void UPoolBackend::HandleProjectileEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	AProjectile* Projectile = Cast<AProjectile>(Actor);
	ActiveProjectiles.Remove(Projectile);
	if (FProjectilePool* Pool = Pools.Find(Actor->GetClass()))
	{
		Pool->InactiveProjectiles.RemoveSwap(Projectile, EAllowShrinking::No);
	}
}

void UPoolBackend::Reset()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PoolBackend_Reset);
	const TArray<TObjectPtr<AProjectile>> ProjectilesToRelease = ActiveProjectiles.Array();
	for (AProjectile* Projectile : ProjectilesToRelease)
	{
		ReleaseProjectile(Projectile);
	}
}

void UPoolBackend::Shutdown()
{
	// Recycle active actors, then close spawning before any destruction callbacks.
	Super::Shutdown();
	// Detach callbacks before destroying: EndPlay would otherwise mutate the pools.
	TArray<TObjectPtr<AProjectile>> ProjectilesToDestroy = ActiveProjectiles.Array();
	for (const auto& Entry : Pools)
	{
		ProjectilesToDestroy.Append(Entry.Value.InactiveProjectiles);
	}
	ActiveProjectiles.Empty();
	Pools.Empty();
	for (AProjectile* Projectile : ProjectilesToDestroy)
	{
		if (IsValid(Projectile) && !Projectile->IsActorBeingDestroyed())
		{
			Projectile->OnEndPlay.RemoveDynamic(this, &UPoolBackend::HandleProjectileEndPlay);
			Projectile->DeactivateForPool();
			Projectile->Destroy();
		}
	}
}

int32 UPoolBackend::GetActiveProjectileCount() const
{
	return ActiveProjectiles.Num();
}

int32 UPoolBackend::GetInactiveProjectileCount() const
{
	int32 Count = 0;
	for (const auto& Entry : Pools)
	{
		Count += Entry.Value.InactiveProjectiles.Num();
	}
	return Count;
}

