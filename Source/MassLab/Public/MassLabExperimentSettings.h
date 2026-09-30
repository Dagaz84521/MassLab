#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Templates/SubclassOf.h"
#include "MassLabExperimentSettings.generated.h"

class UProjectileBackend;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "MassLab Experiment"))
class MASSLAB_API UMassLabExperimentSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMassLabExperimentSettings();
	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, Category = "Projectile", meta = (AllowAbstract = "false"))
	TSubclassOf<UProjectileBackend> BackendClass;
};
