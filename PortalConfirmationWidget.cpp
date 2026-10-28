#include "PortalConfirmationWidget.h"

void UPortalConfirmationWidget::ConfirmTravel()
{
	if (bDecisionMade)
	{
		return;
	}

	bDecisionMade = true;
	OnTravelConfirmed.Broadcast();
}

void UPortalConfirmationWidget::CancelTravel()
{
	if (bDecisionMade)
	{
		return;
	}

	bDecisionMade = true;
	OnTravelCancelled.Broadcast();
}