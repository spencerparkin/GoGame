// Copyright Epic Games, Inc. All Rights Reserved.

#include "GoGameMode.h"
#include "GoGamePlayerController.h"
#include "GoGamePawn.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "GoGameModule.h"
#include "GoGameOptions.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGameMode, Log, All);

AGoGameMode::AGoGameMode()
{
	this->PlayerControllerClass = AGoGamePlayerController::StaticClass();
	this->DefaultPawnClass = AGoGamePawn::StaticClass();
	this->GameStateClass = AGoGameState::StaticClass();
}

/*virtual*/ AGoGameMode::~AGoGameMode()
{
}

/*virtual*/ void AGoGameMode::InitGameState()
{
	Super::InitGameState();

	AGoGameState* gameState = Cast<AGoGameState>(this->GameState);
	if (gameState)
	{
		UE_LOG(LogGoGameMode, Log, TEXT("Creating initial game state for server!"));
		gameState->ResetBoard_Internal(19);
	}
}

/*virtual*/ void AGoGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (this->GetLocalRole() == ENetRole::ROLE_Authority)
	{
		APlayerState* playerState = NewPlayer->GetPlayerState<APlayerState>();
		int playerID = playerState->GetPlayerId();
		UE_LOG(LogGoGameMode, Log, TEXT("Player with ID %d just joined."), playerID);

		AGoGameState* gameState = Cast<AGoGameState>(this->GameState);
		if (gameState)
		{
			// TODO: Why doesn't this work?  The RPC is just getting executed locally.  :(
			UE_LOG(LogGoGameMode, Log, TEXT("Creating initial game state for client!"));
			gameState->ResetBoard(19, playerID);

			// Replay the whole game history for the specific client.
			for (int i = 0; i < gameState->placementHistory.Num(); i++)
			{
				const GoGameMatrix::CellLocation& cellLocation = gameState->placementHistory[i];
				gameState->AlterGameState(cellLocation.i, cellLocation.j, playerID);
			}
		}
	}
}