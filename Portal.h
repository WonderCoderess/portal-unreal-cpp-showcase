#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Portal.generated.h"
class UNiagaraComponent;


class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;
class USoundBase;
class UArrowComponent; //Arrival direction indicator for the portal
class UPrimitiveComponent;
class USoundBase; 

UCLASS()
class ARENASHOOTER_API APortal : public AActor
{
	GENERATED_BODY()
	
public:	
	APortal();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION()
	void OnTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void TryPlacePlayerAtArrival();

	FTimerHandle ArrivalTimerHandle;

	// Start the travel process, including delay and level transition.
	void CompleteTravel();

	FTimerHandle TravelTimerHandle;

	bool bTravelInProgress = false;

public:	

	// Portal components
	UPROPERTY(VisibleAnywhere, Category = "Portal|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Portal|Components")
	TObjectPtr<UStaticMeshComponent> Platform;

	UPROPERTY(VisibleAnywhere, Category = "Portal|Components")
	TObjectPtr<UStaticMeshComponent> Rings;

	UPROPERTY(VisibleAnywhere, Category = "Portal|Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(VisibleAnywhere, Category = "Portal|Components")
	TObjectPtr<UArrowComponent> ArrivalPoint;

	UPROPERTY(VisibleAnywhere, Category = "Portal|Components")
	TObjectPtr<UNiagaraComponent> ActivationEffect;

	// Box half-size: 200 means 400 cm across that axis.
	UPROPERTY(EditAnywhere, Category = "Portal|Settings", meta = (ClampMin = "0.0", Units = "cm"))
	FVector DetectionSize = FVector(200.f, 200.f, 200.f);

	// Unique within this level.
	UPROPERTY(EditInstanceOnly, Category = "Portal|Travel")
	FName PortalId;

	UPROPERTY(EditInstanceOnly, Category = "Portal|Travel")
	TSoftObjectPtr<UWorld> DestinationLevel; // TSoftObjectPtr allows for lazy loading of the level asset.

	UPROPERTY(EditInstanceOnly, Category = "Portal|Travel")
	FName DestinationPortalId;

	UPROPERTY(EditAnywhere, Category = "Portal|Effects",
		meta = (ClampMin = "0.1", Units = "s"))
	float TravelDelay = 1.f;

	UPROPERTY(EditAnywhere, Category = "Portal|Effects")
	TObjectPtr<USoundBase> ActivationSound = nullptr;

};
