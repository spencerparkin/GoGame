// Fill out your copyright notice in the Description page of Project Settings.

#include "GoGameState.h"
#include "GoGameBoard.h"
#include "GoGamePawn.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGameState, Log, All);

AGoGameState::AGoGameState()
{
	UE_LOG(LogGoGameState, Log, TEXT("Game state constructed!"));

	this->bReplicates = true;
	this->PrimaryActorTick.bCanEverTick = true;
	this->renderRefreshNeeded = false;
}

/*virtual*/ AGoGameState::~AGoGameState()
{
	UE_LOG(LogGoGameState, Log, TEXT("Game state destructed!"));

	this->Clear();
}

/*virtual*/ void AGoGameState::BeginPlay()
{
	Super::BeginPlay();

	if (this->GetLocalRole() != ROLE_Authority)
	{
		AGoGamePawn* gamePawn = Cast<AGoGamePawn>(UGameplayStatics::GetPlayerPawn(this->GetWorld(), 0));
		if (gamePawn)
			gamePawn->RequestSetup();
	}
}

void AGoGameState::Clear()
{
	while (this->matrixStack.Num() > 0)
	{
		GoGameMatrix* gameMatrix = this->PopMatrix();
		delete gameMatrix;
	}
}

void AGoGameState::ResetBoard(int boardSize)
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

	if (this->HasAuthority())
	{
		// Make sure all pawns are assigned a color.  Empty means spectator.
		// The color is replicated so this should propogate to the clients.
		// Better than doing this in the tick, we could maybe do this in a join or leave callback, but this works fine for now.
		// Note that spectators can get promoted to a color if a client with that color leaves.

		TArray<AGoGamePawn*> gamePawnArray;
		for (FConstPlayerControllerIterator iter = this->GetWorld()->GetPlayerControllerIterator(); iter; ++iter)
		{
			APlayerController* playerController = Cast<APlayerController>(iter->Get());
			if (playerController && playerController->NetConnection && playerController->NetConnection->GetConnectionState() == EConnectionState::USOCK_Open)
			{
				AGoGamePawn* gamePawn = Cast<AGoGamePawn>(playerController->GetPawn());
				if (gamePawn)
					gamePawnArray.Add(gamePawn);
			}
		}

		TSet<EGoGameCellState> usedColorSet;
		for(AGoGamePawn* gamePawn : gamePawnArray)
			if (gamePawn->myColor == EGoGameCellState::Black || gamePawn->myColor == EGoGameCellState::White)
				usedColorSet.Add(gamePawn->myColor);
		
		for (AGoGamePawn* gamePawn : gamePawnArray)
		{
			if (gamePawn->myColor != EGoGameCellState::Black && gamePawn->myColor != EGoGameCellState::White)
			{
				if (!usedColorSet.Contains(EGoGameCellState::Black))
				{
					gamePawn->myColor = EGoGameCellState::Black;
					usedColorSet.Add(gamePawn->myColor);
				}
				else if (!usedColorSet.Contains(EGoGameCellState::White))
				{
					gamePawn->myColor = EGoGameCellState::White;
					usedColorSet.Add(gamePawn->myColor);
				}
				else if (gamePawn->myColor != EGoGameCellState::Empty)
				{
					gamePawn->myColor = EGoGameCellState::Empty;
				}
			}
		}
	}
}

bool AGoGameState::AlterGameState(const GoGameMatrix::CellLocation& cellLocation, EGoGameCellState playerColor, bool* legalMove /*= nullptr*/)
{
	if (legalMove)
		*legalMove = false;

	bool altered = false;

	if (cellLocation.i == TNumericLimits<int>::Max() && cellLocation.j == TNumericLimits<int>::Max())	// We'll treat this as a pass.
	{
		GoGameMatrix* newGameMatrix = new GoGameMatrix(this->GetCurrentMatrix());
		if (!newGameMatrix->Pass(playerColor))
			delete newGameMatrix;
		else
		{
			if (legalMove)
			{
				delete newGameMatrix;
				*legalMove = true;
			}
			else
			{
				this->PushMatrix(newGameMatrix);
				altered = true;
			}
		}
	}
	else if (!this->GetCurrentMatrix()->IsInBounds(cellLocation))	// We'll treat any out-of-bounds location as an undo operation.
	{
		if(this->matrixStack.Num() > 1 && this->GetCurrentMatrix()->GetWhoseTurn() == playerColor)
		{
			if (legalMove)
				*legalMove = true;
			else
			{
				GoGameMatrix* oldGameMatrix = this->PopMatrix();
				if (oldGameMatrix)
				{
					delete oldGameMatrix;
					altered = true;
				}
			}
		}
	}
	else
	{
		GoGameMatrix* newGameMatrix = new GoGameMatrix(this->GetCurrentMatrix());

		if (!newGameMatrix->SetCellState(cellLocation, playerColor, this->GetForbiddenMatrix()))
			delete newGameMatrix;
		else
		{
			if (legalMove)
			{
				delete newGameMatrix;
				*legalMove = true;
			}
			else
			{
				this->PushMatrix(newGameMatrix);
				altered = true;
			}
		}
	}

	if (altered)
	{
		this->placementHistory.Add(cellLocation);

		// We don't broadcast here, because several board alterations
		// may be performed before we're ready to update the visuals.
		this->renderRefreshNeeded = true;
	}

	return altered;
}