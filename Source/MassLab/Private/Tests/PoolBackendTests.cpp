#if WITH_DEV_AUTOMATION_TESTS

#include "PoolBackend.h"
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
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectilePoolLifecycleTest, "MassLab.Projectile.PoolLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FProjectilePoolLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	TStrongObjectPtr<UPoolBackend> Backend(NewObject<UPoolBackend>());
	Backend->Initialize(*World);
	ON_SCOPE_EXIT
	{
		Backend->Shutdown();
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	};
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();

	FProjectileSpawnConfig Config;
	Config.ActorClass = AProjectile::StaticClass();
	Config.Owner = World->SpawnActor<AActor>();
	Backend->Prewarm(2, Config);
	Backend->Prewarm(2, Config);
	TestEqual(TEXT("Prewarm creates free capacity without duplicating it"), Backend->GetInactiveProjectileCount(), 2);
	TestEqual(TEXT("Prewarmed actors are not active"), Backend->GetActiveProjectileCount(), 0);
	for (TActorIterator<AProjectile> It(World); It; ++It)
	{
		TestTrue(TEXT("Idle actors are hidden"), It->IsHidden());
		TestFalse(TEXT("Idle actor tick is disabled"), It->IsActorTickEnabled());
		TestFalse(TEXT("Idle collision is disabled"), It->GetActorEnableCollision());
		TestFalse(TEXT("Idle movement tick is disabled"), It->FindComponentByClass<UProjectileMovementComponent>()->IsComponentTickEnabled());
	}
	CollectGarbage(RF_NoFlags);
	TestEqual(TEXT("GC retains the reflected free list"), Backend->GetInactiveProjectileCount(), 2);

	TArray<FProjectileSpawnRequest> Requests;
	Requests.SetNum(2);
	Requests[0].Transform = FTransform(FRotator(0.0, 90.0, 0.0), FVector(1000.0, 0.0, 100.0), FVector(2.0, 3.0, 4.0));
	Requests[0].InitialVelocity = FVector(0.0, 100.0, 0.0);
	Requests[0].MaxDistance = 10.0;
	Requests[1].Transform.SetLocation(FVector(2000.0, 0.0, 100.0));
	Requests[1].InitialVelocity = FVector(-50.0, 0.0, 0.0);
	Backend->SpawnBatch(Requests, Config);
	TestEqual(TEXT("Acquire activates both existing actors"), Backend->GetActiveProjectileCount(), 2);
	TestEqual(TEXT("Acquire consumes the free list"), Backend->GetInactiveProjectileCount(), 0);

	AProjectile* First = nullptr;
	AProjectile* Second = nullptr;
	int32 ActorCount = 0;
	for (TActorIterator<AProjectile> It(World); It; ++It)
	{
		++ActorCount;
		if (It->GetActorLocation().Equals(Requests[0].Transform.GetLocation()))
		{
			First = *It;
		}
		else if (It->GetActorLocation().Equals(Requests[1].Transform.GetLocation()))
		{
			Second = *It;
		}
	}
	TestEqual(TEXT("Warm spawning creates no new actors"), ActorCount, 2);
	if (!TestNotNull(TEXT("First projectile"), First) || !TestNotNull(TEXT("Second projectile"), Second))
	{
		return false;
	}
	TestTrue(TEXT("Launch velocity is world-space despite spawn rotation"), First->GetVelocity().Equals(Requests[0].InitialVelocity));
	TestTrue(TEXT("Requested scale is applied"), First->GetActorScale3D().Equals(Requests[0].Transform.GetScale3D()));
	First->SetLifeSpan(0.1f);
	UProjectileMovementComponent* Movement = First->FindComponentByClass<UProjectileMovementComponent>();
	Movement->StopSimulating(FHitResult());
	First->SetActorLocation(Requests[0].Transform.GetLocation() + FVector(0.0, 11.0, 0.0));
	First->Tick(0.0f);
	TestFalse(TEXT("Expiry keeps the actor alive"), First->IsActorBeingDestroyed());
	TestEqual(TEXT("Expiry returns one actor"), Backend->GetInactiveProjectileCount(), 1);
	TestEqual(TEXT("Expiry updates the active count"), Backend->GetActiveProjectileCount(), 1);
	TestEqual(TEXT("Old lifespan is cancelled"), First->GetLifeSpan(), 0.0f);
	First->Tick(0.0f);
	TestEqual(TEXT("Repeated inactive tick does not duplicate a release"), Backend->GetInactiveProjectileCount(), 1);

	Config.Owner = World->SpawnActor<AActor>();
	Requests.SetNum(1);
	Requests[0].Transform = FTransform(FRotator::ZeroRotator, FVector(5000.0, 0.0, 100.0), FVector(3.0, 2.0, 1.0));
	Requests[0].InitialVelocity = FVector(80.0, 0.0, 0.0);
	Requests[0].MaxDistance = 100.0;
	Backend->SpawnBatch(Requests, Config);
	TestTrue(TEXT("The expired actor is reused at the new position"), First->GetActorLocation().Equals(Requests[0].Transform.GetLocation()));
	TestEqual(TEXT("Reuse updates owner"), First->GetOwner(), Config.Owner.Get());
	TestTrue(TEXT("Reuse applies scale without accumulating the old scale"), First->GetActorScale3D().Equals(Requests[0].Transform.GetScale3D()));
	TestEqual(TEXT("Reuse resets distance limit"), First->GetMaxDistance(), Requests[0].MaxDistance);
	TestEqual(TEXT("Stopped movement reacquires its root"), Movement->UpdatedComponent.Get(), First->GetRootComponent());
	TestTrue(TEXT("Reuse restarts movement tick"), Movement->IsComponentTickEnabled());
	Movement->TickComponent(0.1f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Reused actor actually moves"), First->GetActorLocation().Equals(Requests[0].Transform.GetLocation() + FVector(8.0, 0.0, 0.0), 0.001));
	First->Tick(0.0f);
	TestEqual(TEXT("Distance is measured from the new launch point"), Backend->GetActiveProjectileCount(), 2);

	Backend->Reset();
	Backend->Reset();
	TestEqual(TEXT("Reset recycles every active actor"), Backend->GetActiveProjectileCount(), 0);
	TestEqual(TEXT("Repeated reset retains exactly two idle actors"), Backend->GetInactiveProjectileCount(), 2);
	First->Destroy();
	TestEqual(TEXT("External destruction removes an idle actor"), Backend->GetInactiveProjectileCount(), 1);
	Backend->SpawnBatch(Requests, Config);
	TestFalse(TEXT("Surviving actor is activated"), Second->IsHidden());
	Second->Destroy();
	TestEqual(TEXT("External destruction removes an active actor"), Backend->GetActiveProjectileCount(), 0);
	TestEqual(TEXT("Destroyed actors are not retained"), Backend->GetInactiveProjectileCount(), 0);

	UClass* BlueprintClass = LoadClass<AProjectile>(nullptr, TEXT("/Game/BP_Projectile.BP_Projectile_C"));
	if (!TestNotNull(TEXT("Projectile Blueprint"), BlueprintClass))
	{
		return false;
	}
	Backend->Prewarm(1, Config);
	FProjectileSpawnConfig BlueprintConfig = Config;
	BlueprintConfig.ActorClass = BlueprintClass;
	// Compare against a fresh spawn, including Blueprint construction-script scale.
	TStrongObjectPtr<UDirectActorProjectileBackend> ReferenceBackend(NewObject<UDirectActorProjectileBackend>());
	ReferenceBackend->Initialize(*World);
	ReferenceBackend->SpawnBatch(Requests, BlueprintConfig);
	FVector ExpectedBlueprintScale = FVector::OneVector;
	for (TActorIterator<AProjectile> It(World); It; ++It)
	{
		if (It->GetClass() == BlueprintClass)
		{
			ExpectedBlueprintScale = It->GetActorScale3D();
		}
	}
	ReferenceBackend->Shutdown();
	Backend->Prewarm(1, BlueprintConfig);
	for (int32 Round = 0; Round < 3; ++Round)
	{
		Backend->SpawnBatch(Requests, Config);
		Backend->SpawnBatch(Requests, BlueprintConfig);
		int32 NativeCount = 0;
		int32 BlueprintCount = 0;
		for (TActorIterator<AProjectile> It(World); It; ++It)
		{
			if (It->GetClass() == BlueprintClass)
			{
				++BlueprintCount;
				TestTrue(FString::Printf(TEXT("Blueprint scale matches fresh spawning (actual %s, expected %s)"),
					*It->GetActorScale3D().ToString(), *ExpectedBlueprintScale.ToString()), It->GetActorScale3D().Equals(ExpectedBlueprintScale));
				TestTrue(TEXT("Blueprint reuse resets velocity"), It->GetVelocity().Equals(Requests[0].InitialVelocity));
			}
			else
			{
				++NativeCount;
			}
		}
		TestEqual(TEXT("Native actors use their own class pool"), NativeCount, 1);
		TestEqual(TEXT("Blueprint actors use their own class pool"), BlueprintCount, 1);
		Backend->Reset();
	}
	Backend->SpawnBatch(Requests, Config);
	TestEqual(TEXT("Shutdown setup contains an active actor"), Backend->GetActiveProjectileCount(), 1);
	TestEqual(TEXT("Shutdown setup contains an idle actor"), Backend->GetInactiveProjectileCount(), 1);
	Backend->Shutdown();
	TestEqual(TEXT("Shutdown clears active actors"), Backend->GetActiveProjectileCount(), 0);
	TestEqual(TEXT("Shutdown clears idle actors"), Backend->GetInactiveProjectileCount(), 0);
	TestNull(TEXT("Shutdown releases the world reference"), Backend->GetWorld());
	int32 RemainingCount = 0;
	for (TActorIterator<AProjectile> It(World); It; ++It)
	{
		++RemainingCount;
	}
	TestEqual(TEXT("Shutdown destroys actors from both states"), RemainingCount, 0);
	Backend->SpawnBatch(Requests, Config);
	TestEqual(TEXT("A shut down backend cannot spawn"), Backend->GetActiveProjectileCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectilePoolSubsystemTest, "MassLab.Projectile.PoolSubsystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FProjectilePoolSubsystemTest::RunTest(const FString& Parameters)
{
	UMassLabExperimentSettings* Settings = GetMutableDefault<UMassLabExperimentSettings>();
	TGuardValue<TSubclassOf<UProjectileBackend>> BackendSetting(Settings->BackendClass, UPoolBackend::StaticClass());
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
	if (!TestNotNull(TEXT("Projectile subsystem"), Subsystem))
	{
		return false;
	}
	Subsystem->PrewarmProjectiles(AProjectile::StaticClass(), 3);
	TestEqual(TEXT("Subsystem prewarming creates no active projectiles"), Subsystem->GetActiveProjectileCount(), 0);
	FProjectileSpawnConfig Config;
	Config.ActorClass = AProjectile::StaticClass();
	TArray<FProjectileSpawnRequest> Requests;
	Requests.SetNum(4);
	Subsystem->SpawnBatch(Requests, Config);
	TestEqual(TEXT("Pool grows when the prewarmed capacity is exhausted"), Subsystem->GetActiveProjectileCount(), 4);
	Subsystem->ResetProjectiles();
	TestEqual(TEXT("Subsystem reset returns actors to the pool"), Subsystem->GetActiveProjectileCount(), 0);
	int32 ActorCount = 0;
	for (TActorIterator<AProjectile> It(World); It; ++It)
	{
		++ActorCount;
		TestTrue(TEXT("Reset actors remain alive and inactive"), It->IsHidden());
	}
	TestEqual(TEXT("Reset retains the pool's four actors"), ActorCount, 4);
	Subsystem->SpawnBatch(Requests, Config);
	TestEqual(TEXT("Subsystem can spawn again after reset"), Subsystem->GetActiveProjectileCount(), 4);
	Subsystem->Deinitialize();
	TestEqual(TEXT("Subsystem shutdown clears the selected pool backend"), Subsystem->GetActiveProjectileCount(), 0);
	return true;
}

#endif
