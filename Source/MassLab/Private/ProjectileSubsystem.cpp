#include "ProjectileSubsystem.h"

#include "Engine/World.h"
#include "MassLabExperimentSettings.h"
#include "ProjectileBackend.h"
#include "Projectile.h"

DEFINE_LOG_CATEGORY_STATIC(LogProjectileSubsystem, Log, All);

void UProjectileSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (ActiveBackend)
	{
		return;
	}

	UClass* BackendClass = GetDefault<UMassLabExperimentSettings>()->BackendClass.Get();
	if (!BackendClass || BackendClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(LogProjectileSubsystem, Error, TEXT("Select a concrete Projectile Backend Class in Project Settings > Game > MassLab Experiment."));
		return;
	}

	ActiveBackend = NewObject<UProjectileBackend>(this, BackendClass);
	ActiveBackend->Initialize(InWorld);
	UE_LOG(LogProjectileSubsystem, Log, TEXT("Projectile backend: %s"), *BackendClass->GetName());
}

void UProjectileSubsystem::SpawnBatch(TConstArrayView<FProjectileSpawnRequest> Requests, const FProjectileSpawnConfig& Config)
{
	if (Requests.IsEmpty())
	{
		return;
	}
	if (ensureMsgf(ActiveBackend, TEXT("The projectile subsystem must begin play before spawning projectiles.")))
	{
		ActiveBackend->SpawnBatch(Requests, Config);
	}
}

void UProjectileSubsystem::ResetProjectiles()
{
	if (ActiveBackend)
	{
		ActiveBackend->Reset();
	}
}

void UProjectileSubsystem::PrewarmProjectiles(TSubclassOf<AProjectile> ProjectileClass, int32 Count, AActor* Owner)
{
	if (ActiveBackend && ProjectileClass && Count > 0)
	{
		FProjectileSpawnConfig Config;
		Config.ActorClass = ProjectileClass;
		Config.Owner = Owner;
		ActiveBackend->Prewarm(Count, Config);
	}
}

int32 UProjectileSubsystem::GetActiveProjectileCount() const
{
	return ActiveBackend ? ActiveBackend->GetActiveProjectileCount() : 0;
}

void UProjectileSubsystem::Deinitialize()
{
	if (ActiveBackend)
	{
		ActiveBackend->Shutdown();
		ActiveBackend = nullptr;
	}
	Super::Deinitialize();
}

bool UProjectileSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
