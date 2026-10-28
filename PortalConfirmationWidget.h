#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PortalConfirmationWidget.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPortalConfirmationAction);

UCLASS()
class ARENASHOOTER_API UPortalConfirmationWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable, Category = "Portal")
	FPortalConfirmationAction OnTravelConfirmed;

	UPROPERTY(BlueprintAssignable, Category = "Portal")
	FPortalConfirmationAction OnTravelCancelled;

	UFUNCTION(BlueprintCallable, Category = "Portal")
	void ConfirmTravel();

	UFUNCTION(BlueprintCallable, Category = "Portal")
	void CancelTravel();

private:
	bool bDecisionMade = false;
};
