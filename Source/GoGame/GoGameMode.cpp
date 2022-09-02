// Copyright Epic Games, Inc. All Rights Reserved.

#include "GoGameMode.h"
#include "GoGamePlayerController.h"
#include "GoGamePawn.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "GoGameModule.h"
#include "GoGameOptions.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

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
		int boardSize = 19;
		FString boardSizeStr = UGameplayStatics::ParseOption(this->OptionsString, "BoardSize");
		if (boardSizeStr.Len() > 0)
		{
			boardSize = ::atoi(TCHAR_TO_ANSI(*boardSizeStr));
			if (boardSize < 2)
				boardSize = 2;
			if (boardSize > 19)
				boardSize = 19;
		}

		UE_LOG(LogGoGameMode, Log, TEXT("Creating initial game state!  (Board size: %d x %d)"), boardSize);
		gameState->ResetBoard(boardSize);
	}
}

/*virtual*/ void AGoGameMode::PostLogin(APlayerController* playerController)
{
	Super::PostLogin(playerController);

	// Can't setup client here, because their game-state object isn't necessarily replicated yet.
}