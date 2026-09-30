#if WITH_DEV_AUTOMATION_TESTS

#include "DirectActorProjectileBackend.h"
#include "MassLabExperimentSettings.h"
#include "Projectile.h"
#include "ProjectileSubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectileBackendLifecycleTest, "MassLab.Projectile.BackendLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FProjectileBackendLifecycleTest::RunTest(const FString& Parameters)
{
	UMassLabExperimentSettings* Settings = GetMutableDefault<UMassLabExperimentSettings>();
	TGuardValue<TSubclassOf<UProjectileBackend>> BackendSetting(Settings->BackendClass, UDirectActorProjectileBackend::StaticClass());

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	};
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();

	UProjectileSubsystem* Subsystem = World->GetSubsystem<UProjectileSubsystem>();
	if (!TestNotNull(TEXT("Game world creates the projectile subsystem"), Subsystem))
	{
		return false;
	}

	FProjectileSpawnConfig Config;
	Config.ActorClass = AProjectile::StaticClass();
	Config.Owner = World->SpawnActor<AActor>();
	TArray<FProjectileSpawnRequest> Requests;
	FProjectileSpawnRequest& FirstRequest = Requests.AddDefaulted_GetRef();
	FirstRequest.Transform = FTransform(FRotator(0.0, 90.0, 0.0), FVector(1000.0, 0.0, 100.0));
	FirstRequest.InitialVelocity = FVector(0.0, 100.0, 0.0);
	FirstRequest.MaxDistance = 10.0;
	FProjectileSpawnRequest& SecondRequest = Requests.AddDefaulted_GetRef();
	SecondRequest.Transform.SetLocation(FVector(2000.0, 0.0, 100.0));
	SecondRequest.InitialVelocity = FVector(-50.0, 0.0, 0.0);
	SecondRequest.MaxDistance = 1000.0;
	Subsystem->SpawnBatch(Requests, Config);
	TestEqual(TEXT("Batch creates both projectiles"), Subsystem->GetActiveProjectileCount(), 2);

	AProjectile* First = nullptr;
	AProjectile* Second = nullptr;
	for (TActorIterator<AProjectile> It(World); It; ++It)
	{
		if (It->GetActorLocation().Equals(Requests[0].Transform.GetLocation()))
		{
			First = *It;
		}
		else if (It->GetActorLocation().Equals(Requests[1].Transform.GetLocation()))
		{
			Second = *It;
		}
	}
	if (!TestNotNull(TEXT("First projectile"), First) || !TestNotNull(TEXT("Second projectile"), Second))
	{
		return false;
	}
	TestTrue(TEXT("Deferred initialization preserves world-space velocity"), First->GetVelocity().Equals(Requests[0].InitialVelocity));
	TestTrue(TEXT("Second projectile has its own velocity"), Second->GetVelocity().Equals(Requests[1].InitialVelocity));
	TestEqual(TEXT("Spawn owner is preserved"), First->GetOwner(), Config.Owner.Get());
	First->Tick(0.0f);
	TestEqual(TEXT("Distance is measured from the spawn position"), Subsystem->GetActiveProjectileCount(), 2);
	First->SetActorLocation(Requests[0].Transform.GetLocation() + FVector(0.0, 11.0, 0.0));
	First->Tick(0.0f);
	TestTrue(TEXT("Expired projectile is destroyed by the backend"), First->IsActorBeingDestroyed());
	TestEqual(TEXT("Expired projectile leaves the active set"), Subsystem->GetActiveProjectileCount(), 1);
	Second->Destroy();
	TestEqual(TEXT("External destruction also updates the active set"), Subsystem->GetActiveProjectileCount(), 0);

	UClass* BlueprintClass = LoadClass<AProjectile>(nullptr, TEXT("/Game/BP_Projectile.BP_Projectile_C"));
	if (!TestNotNull(TEXT("Existing projectile Blueprint"), BlueprintClass))
	{
		return false;
	}
	Config.ActorClass = BlueprintClass;
	Requests.SetNum(1);
	Requests[0].Transform.SetLocation(FVector(3000.0, 0.0, 100.0));
	Requests[0].MaxDistance = Config.ActorClass.GetDefaultObject()->GetMaxDistance();
	Subsystem->SpawnBatch(Requests, Config);
	TestEqual(TEXT("Blueprint projectile is tracked"), Subsystem->GetActiveProjectileCount(), 1);
	for (TActorIterator<AProjectile> It(World); It; ++It)
	{
		TestEqual(TEXT("Selected Blueprint class is instantiated"), It->GetClass(), BlueprintClass);
		TestTrue(TEXT("Blueprint startup preserves requested launch velocity"), It->GetVelocity().Equals(Requests[0].InitialVelocity));
		const UStaticMeshComponent* Mesh = It->FindComponentByClass<UStaticMeshComponent>();
		const UStaticMeshComponent* DefaultMesh = Config.ActorClass.GetDefaultObject()->FindComponentByClass<UStaticMeshComponent>();
		if (TestNotNull(TEXT("Blueprint mesh component"), Mesh) && TestNotNull(TEXT("Blueprint default mesh component"), DefaultMesh))
		{
			TestEqual(TEXT("Blueprint mesh selection is retained"), Mesh->GetStaticMesh(), DefaultMesh->GetStaticMesh());
			TestTrue(TEXT("Blueprint root scale is retained"), Mesh->GetComponentScale().Equals(DefaultMesh->GetRelativeScale3D()));
		}
	}
	Subsystem->ResetProjectiles();
	TestEqual(TEXT("Reset clears every projectile"), Subsystem->GetActiveProjectileCount(), 0);
	Subsystem->SpawnBatch(Requests, Config);
	TestEqual(TEXT("Backend remains usable after reset"), Subsystem->GetActiveProjectileCount(), 1);
	Subsystem->Deinitialize();
	TestEqual(TEXT("Subsystem shutdown clears the backend"), Subsystem->GetActiveProjectileCount(), 0);

	return true;
}

#endif
