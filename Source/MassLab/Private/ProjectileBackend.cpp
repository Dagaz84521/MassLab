#include "ProjectileBackend.h"

#include "Engine/World.h"

void UProjectileBackend::Initialize(UWorld& InWorld)
{
	SimulationWorld = &InWorld;
}

void UProjectileBackend::Shutdown()
{
	Reset();
	SimulationWorld.Reset();
}

UWorld* UProjectileBackend::GetWorld() const
{
	return SimulationWorld.Get();
}
