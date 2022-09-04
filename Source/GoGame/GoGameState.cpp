// Fill out your copyright notice in the Description page of Project Settings.

#include "GoGameState.h"
#include "GoGameBoard.h"
#include "GoGamePawnHuman.h"
#include "GoGamePlayerController.h"
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

BEGIN_FUNCTION_BUILD_OPTIMIZATION

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

END_FUNCTION_BUILD_OPTIMIZATION