#pragma once

#include "CoreMinimal.h"
#include "ProjectileSpawnRequest.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectileSubsystem.generated.h"

class UProjectileBackend;

UCLASS()
class MASSLAB_API UProjectileSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void SpawnBatch(TConstArrayView<FProjectileSpawnRequest> Requests, const FProjectileSpawnConfig& Config);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void PrewarmProjectiles(TSubclassOf<AProjectile> ProjectileClass, int32 Count, AActor* Owner = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void ResetProjectiles();

	UFUNCTION(BlueprintPure, Category = "Projectile")
	int32 GetActiveProjectileCount() const;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UProjectileBackend> ActiveBackend;
};
