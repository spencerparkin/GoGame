// Fill out your copyright notice in the Description page of Project Settings.

#include "GoGameState.h"
#include "GoGameBoard.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGameState, Log, All);

AGoGameState::AGoGameState()
{
	this->bReplicates = true;
	this->PrimaryActorTick.bCanEverTick = true;
	this->renderRefreshNeeded = false;
}

/*virtual*/ AGoGameState::~AGoGameState()
{
	this->Clear();
}

void AGoGameState::Clear()
{
	while (this->matrixStack.Num() > 0)
	{
		GoGameMatrix* gameMatrix = this->PopMatrix();
		delete gameMatrix;
	}
}

void AGoGameState::ResetBoard_Implementation(int boardSize, int playerID)
{
	if (this->CanPerformRPC(playerID))
		this->ResetBoard_Internal(boardSize);
}

void AGoGameState::ResetBoard_Internal(int boardSize)
{
	this->Clear();

	GoGameMatrix* gameMatrix = new GoGameMatrix();
	gameMatrix->SetMatrixSize(boardSize);

	this->PushMatrix(gameMatrix);

	this->renderRefreshNeeded = true;
}

GoGameMatrix* AGoGameState::GetCurrentMatrix()
{
	if (this->matrixStack.Num() > 0)
		return this->matrixStack[this->matrixStack.Num() - 1];
	
	return nullptr;
}

GoGameMatrix* AGoGameState::GetForbiddenMatrix()
{
	if (this->matrixStack.Num() > 1)
		return this->matrixStack[this->matrixStack.Num() - 2];

	return nullptr;
}

void AGoGameState::PushMatrix(GoGameMatrix* gameMatrix)
{
	this->matrixStack.Push(gameMatrix);
}

GoGameMatrix* AGoGameState::PopMatrix()
{
	if (this->matrixStack.Num() == 0)
		return nullptr;

	GoGameMatrix* gameMatrix = this->matrixStack[this->matrixStack.Num() - 1];
	this->matrixStack.Pop();
	return gameMatrix;
}

/*virtual*/ void AGoGameState::Tick(float DeltaTime)
{
	static bool hack = false;
	if (hack)
	{
		this->ResetBoard_Internal(19);
	}

	if (this->renderRefreshNeeded && this->GetCurrentMatrix())
	{
		this->renderRefreshNeeded = false;

		// Signal the visuals to reflect the current state of the game.
		TArray<AActor*> actorArray;
		UGameplayStatics::GetAllActorsOfClass(this->GetWorld(), AGoGameBoard::StaticClass(), actorArray);
		if (actorArray.Num() == 1)
		{
			AGoGameBoard* gameBoard = Cast<AGoGameBoard>(actorArray[0]);
			if (gameBoard)
				gameBoard->OnBoardAppearanceChanged.Broadcast();
		}
	}
}

// This is called by the server and executed on the client.
void AGoGameState::AlterGameState_Implementation(int i, int j, int playerID)
{
	if (this->CanPerformRPC(playerID))
	{
		GoGameMatrix::CellLocation cellLocation;
		cellLocation.i = i;
		cellLocation.j = j;

		UE_LOG(LogGoGameState, Log, TEXT("Client placing stone at location (%d, %d) as per the server's request."), cellLocation.i, cellLocation.j);

		bool altered = this->AlterGameState_Internal(cellLocation);
		if (!altered)
		{
			UE_LOG(LogGoGameState, Error, TEXT("Board alteration failed!  This shouldn't happen as the server vets all moves before relaying them to the client."));
		}
	}
}

bool AGoGameState::AlterGameState_Internal(const GoGameMatrix::CellLocation& cellLocation)
{
	bool altered = false;

	// We'll treat any out-of-bounds location as an undo operation.
	if (!this->GetCurrentMatrix()->IsInBounds(cellLocation))
	{
		GoGameMatrix* oldGameMatrix = this->PopMatrix();
		if (oldGameMatrix)
		{
			delete oldGameMatrix;
			altered = true;
		}
	}
	else
	{
		GoGameMatrix* newGameMatrix = new GoGameMatrix(this->GetCurrentMatrix());

		if (!newGameMatrix->SetCellState(cellLocation, newGameMatrix->GetWhoseTurn(), this->GetForbiddenMatrix()))
			delete newGameMatrix;
		else
		{
			this->PushMatrix(newGameMatrix);
			altered = true;
		}
	}

	if (altered)
	{
		this->placementHistory.Add(cellLocation);

		// We don't broadcast here, because several board alternations
		// may be performed before we're ready to update the visuals.
		this->renderRefreshNeeded = true;
	}

	return altered;
}

bool AGoGameState::CanPerformRPC(int playerID)
{
	if (playerID == INT32_MAX)
		return true;

	APlayerController* playerController = UGameplayStatics::GetPlayerController(this->GetWorld(), 0);
	if (!playerController)
		return false;

	APlayerState* playerState = playerController->GetPlayerState<APlayerState>();
	if (!playerState)
		return false;

	return playerState->GetPlayerId() == playerID;
}

// This is being executed on the server at the request of the client.
void AGoGameState::TryAlterGameState_Implementation(int i, int j)
{
	GoGameMatrix::CellLocation cellLocation;
	cellLocation.i = i;
	cellLocation.j = j;

	// Could the requested move be made?
	if (this->AlterGameState_Internal(cellLocation))
	{
		// Yes.  Now go tell the clients to do the same.
		// TODO: This *just* calls the clients, right?  Or will this try to execute locally too?
		this->AlterGameState(cellLocation.i, cellLocation.j, INT32_MAX);
	}
}