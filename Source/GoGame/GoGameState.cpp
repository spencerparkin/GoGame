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

	if (this->GetLocalRole() == ENetRole::ROLE_Authority && IsRunningDedicatedServer())
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

		TSet<int> usedColorSet;
		for(AGoGamePawn* gamePawn : gamePawnArray)
			if (gamePawn->myColor == int(EGoGameCellState::Black) || gamePawn->myColor == int(EGoGameCellState::White))
				usedColorSet.Add(gamePawn->myColor);
		
		for (AGoGamePawn* gamePawn : gamePawnArray)
		{
			if (gamePawn->myColor != int(EGoGameCellState::Black) && gamePawn->myColor != int(EGoGameCellState::White))
			{
				if (!usedColorSet.Contains(int(EGoGameCellState::Black)))
				{
					gamePawn->myColor = int(EGoGameCellState::Black);
					usedColorSet.Add(gamePawn->myColor);
				}
				else if (!usedColorSet.Contains(int(EGoGameCellState::White)))
				{
					gamePawn->myColor = int(EGoGameCellState::White);
					usedColorSet.Add(gamePawn->myColor);
				}
				else if (gamePawn->myColor != int(EGoGameCellState::Empty))
				{
					gamePawn->myColor = int(EGoGameCellState::Empty);
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

	// We'll treat any out-of-bounds location as an undo operation.
	if (!this->GetCurrentMatrix()->IsInBounds(cellLocation))
	{
		if (legalMove)
			*legalMove = (this->matrixStack.Num() > 0) && (playerColor == EGoGameCellState::Black_or_White || this->GetCurrentMatrix()->GetWhoseTurn() == playerColor);
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
	else
	{
		GoGameMatrix* newGameMatrix = new GoGameMatrix(this->GetCurrentMatrix());

		EGoGameCellState cellState = EGoGameCellState::Empty;
		if (playerColor == EGoGameCellState::Black_or_White)
			cellState = newGameMatrix->GetWhoseTurn();
		else
			cellState = (EGoGameCellState)playerColor;

		if (!newGameMatrix->SetCellState(cellLocation, cellState, this->GetForbiddenMatrix()))
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

		// We don't broadcast here, because several board alternations
		// may be performed before we're ready to update the visuals.
		this->renderRefreshNeeded = true;
	}

	return altered;
}