// Copyright Epic Games, Inc. All Rights Reserved.

#include "GoGameMode.h"
#include "GoGamePlayerController.h"
#include "GoGamePawn.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"

AGoGameMode::AGoGameMode()
{
	this->PlayerControllerClass = AGoGamePlayerController::StaticClass();
	//this->DefaultPawnClass = AGoGamePawn::StaticClass();
	this->GameStateClass = AGoGameState::StaticClass();
}

/*virtual*/ AGoGameMode::~AGoGameMode()
{
}

/*virtual*/ void AGoGameMode::InitGameState()
{
	Super::InitGameState();

	GoGameMatrix* goGameMatrix = new GoGameMatrix();
	goGameMatrix->SetMatrixSize(10);

	AGoGameState* goGameState = Cast<AGoGameState>(this->GameState);
	goGameState->PushMatrix(goGameMatrix);
}