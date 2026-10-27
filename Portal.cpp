#include "Portal.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "ArenaGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "NiagaraComponent.h"

// Define a macro for debugging portal behavior. Set to 1 to enable debug logs, or 0 to disable them.
#define PORTAL_DEBUG 0

APortal::APortal()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Create the platformm
	Platform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Platform"));
	Platform->SetupAttachment(SceneRoot);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlatformMesh(TEXT("/Game/Portals/SciFiPortal/SM_Portal_Platform.SM_Portal_Platform"));

	if (PlatformMesh.Succeeded())
	{
		Platform->SetStaticMesh(PlatformMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlatformMaterial(TEXT("/Game/Portals/SciFiPortal/M_Portal_Platform.M_Portal_Platform"));

	if (PlatformMaterial.Succeeded())
	{
		Platform->SetMaterial(0, PlatformMaterial.Object);
	}

	// Create the rings.
	Rings = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rings"));
	Rings->SetupAttachment(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> RingsMesh(TEXT("/Game/Portals/SciFiPortal/SM_Portal_Rings.SM_Portal_Rings"));

	if (RingsMesh.Succeeded())
	{
		Rings->SetStaticMesh(RingsMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RingsMaterial(TEXT("/Game/Portals/SciFiPortal/M_Portal_Rings.M_Portal_Rings"));

	if (RingsMaterial.Succeeded())
	{
		Rings->SetMaterial(0, RingsMaterial.Object);
	}

	// Create the player detection area.
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(SceneRoot);
	TriggerBox->SetBoxExtent(DetectionSize);
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);

	// Create the arrival position and facing direction.
	ArrivalPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrivalPoint"));
	ArrivalPoint->SetupAttachment(SceneRoot);

	// Create the portal activation Niagara effect.
	ActivationEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ActivationEffect"));
	ActivationEffect->SetupAttachment(SceneRoot);
	ActivationEffect->SetAutoActivate(false);

}

void APortal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	TriggerBox->SetBoxExtent(DetectionSize);
}

void APortal::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APortal::OnTriggerBeginOverlap);

	UArenaGameInstance* GameInstance =
		Cast<UArenaGameInstance>(GetGameInstance());

#if PORTAL_DEBUG
		UE_LOG(LogTemp, Warning,
			TEXT("Portal start | Map=%s | PortalId=%s | Pending=%s | GameInstance=%s"),
			*GetWorld()->GetMapName(),
			*PortalId.ToString(),
			GameInstance
			? *GameInstance->PendingArrivalPortalId.ToString()
			: TEXT("BRAK"),
			*GetNameSafe(GetGameInstance()));
#endif

	if (GameInstance &&
		!PortalId.IsNone() &&
		GameInstance->PendingArrivalPortalId == PortalId)
	{
		GetWorldTimerManager().SetTimer(
			ArrivalTimerHandle,
			this,
			&APortal::TryPlacePlayerAtArrival,
			0.1f,
			true);
	}

}

void APortal::OnTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);

	if (!PlayerPawn || OtherActor != PlayerPawn)
	{
		return;
	}

	if (bTravelInProgress)
	{
		return;
	}

	UArenaGameInstance* GameInstance =
		Cast<UArenaGameInstance>(GetGameInstance());

	if (!GameInstance)
	{
		return;
	}

	// Ignore overlaps while placing the player after level travel.
	if (!GameInstance->PendingArrivalPortalId.IsNone())
	{
		return;
	}

#if PORTAL_DEBUG
	UE_LOG(LogTemp, Warning, TEXT("Player entered portal: %s"),
		*GetName());
#endif

	if (DestinationLevel.IsNull() || DestinationPortalId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Portal destination is not configured."));
		return;
	}

	bTravelInProgress = true;

	ActivationEffect->Activate(true);

#if PORTAL_DEBUG
	UE_LOG(LogTemp, Warning,
		TEXT("ACTIVATING PORTAL EFFECT: %s"), *GetName());
#endif

	if (ActivationSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ActivationSound,
			TriggerBox->GetComponentLocation());
	}

	GetWorldTimerManager().SetTimer(
		TravelTimerHandle,
		this,
		&APortal::CompleteTravel,
		TravelDelay,
		false);
}

void APortal::TryPlacePlayerAtArrival()
{
	UArenaGameInstance* GameInstance =
		Cast<UArenaGameInstance>(GetGameInstance());

	if (!GameInstance ||
		GameInstance->PendingArrivalPortalId != PortalId)
	{
		GetWorldTimerManager().ClearTimer(ArrivalTimerHandle);
		return;
	}

	APlayerController* PlayerController =
		UGameplayStatics::GetPlayerController(this, 0);

	if (!PlayerController || !PlayerController->GetPawn())
	{
		return;
	}

	// The GameMode already selected the spawn position.
	// Match the view to the character's actual spawn rotation.
	PlayerController->SetControlRotation(
		PlayerController->GetPawn()->GetActorRotation());

	GameInstance->PendingArrivalPortalId = NAME_None;

	GetWorldTimerManager().ClearTimer(ArrivalTimerHandle);

#if PORTAL_DEBUG
	UE_LOG(LogTemp, Warning,
		TEXT("Portal arrival completed: %s"),
		*PortalId.ToString());
#endif
}

void APortal::CompleteTravel()
{
	UArenaGameInstance* GameInstance =
		Cast<UArenaGameInstance>(GetGameInstance());
	
#if PORTAL_DEBUG
	UE_LOG(LogTemp, Warning,
		TEXT("Completing travel through portal: %s"), *GetName());
#endif

	if (!GameInstance)
	{
		bTravelInProgress = false;
		ActivationEffect->Deactivate();
		return;
	}

	GameInstance->PendingArrivalPortalId = DestinationPortalId;

#if PORTAL_DEBUG
	UE_LOG(LogTemp, Warning,
		TEXT("Opening destination level: %s"), *DestinationLevel.GetAssetName());
#endif

	UGameplayStatics::OpenLevelBySoftObjectPtr(
		this, DestinationLevel);
}