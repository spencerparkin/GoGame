// Copyright Epic Games, Inc. All Rights Reserved.

#include "GoGameMode.h"
#include "GoGamePlayerController.h"
#include "GoGamePawn.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "GoGameModule.h"
#include "GoGameOptions.h"

AGoGameMode::AGoGameMode()
{
	this->PlayerControllerClass = AGoGamePlayerController::StaticClass();
	this->DefaultPawnClass = AGoGamePawn::StaticClass();
	this->GameStateClass = AGoGameState::StaticClass();

	this->gameOptions = nullptr;
}

/*virtual*/ AGoGameMode::~AGoGameMode()
{
}

/*virtual*/ void AGoGameMode::InitGameState()
{
	Super::InitGameState();

	GoGameModule* gameModule = (GoGameModule*)FModuleManager::Get().GetModule("GoGame");
	this->gameOptions = gameModule->gameOptions;

	GoGameMatrix* gameMatrix = new GoGameMatrix();
	gameMatrix->SetMatrixSize(this->gameOptions->boardDimension);

	AGoGameState* gameState = Cast<AGoGameState>(this->GameState);
	gameState->PushMatrix(gameMatrix);
}