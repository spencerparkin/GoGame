#include "GoGamePlayerController.h"

AGoGamePlayerController::AGoGamePlayerController()
{
	this->bShowMouseCursor = true;
	this->bEnableClickEvents = true;
	this->bEnableTouchEvents = true;
	this->DefaultMouseCursor = EMouseCursor::Crosshairs;
}

/*virtual*/ AGoGamePlayerController::~AGoGamePlayerController()
{
}