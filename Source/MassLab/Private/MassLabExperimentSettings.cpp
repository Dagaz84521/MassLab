#include "MassLabExperimentSettings.h"

#include "DirectActorProjectileBackend.h"

UMassLabExperimentSettings::UMassLabExperimentSettings()
{
	BackendClass = UDirectActorProjectileBackend::StaticClass();
}

FName UMassLabExperimentSettings::GetCategoryName() const
{
	return TEXT("Game");
}
