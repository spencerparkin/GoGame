// Fill out your copyright notice in the Description page of Project Settings.

#include "GoGamePawn.h"
#include "GoGameMatrix.h"
#include "GoGameMode.h"
#include "GoGameState.h"
#include "GoGameBoard.h"
#include "GoGamePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGamePawn, Log, All);

AGoGamePawn::AGoGamePawn()
{
	this->myColor = EGoGameCellState::Empty;
	this->bReplicates = true;
	this->PrimaryActorTick.bCanEverTick = true;
	this->gameBoard = nullptr;
}

/*virtual*/ AGoGamePawn::~AGoGamePawn()
{
}

void AGoGamePawn::BeginPlay()
{
	Super::BeginPlay();

	if (!this->gameBoard)
		this->gameBoard = Cast<AGoGameBoard>(UGameplayStatics::GetActorOfClass(this->GetWorld(), AGoGameBoard::StaticClass()));
}

BEGIN_FUNCTION_BUILD_OPTIMIZATION

void AGoGamePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

/*virtual*/ void AGoGamePawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGoGamePawn, myColor);
}

void AGoGamePawn::OnRep_MyColorChanged()
{
	UE_LOG(LogGoGamePawn, Log, TEXT("My color rep-notify fired!  (myColor=%d)"), int(this->myColor));

	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (gameState)
		gameState->renderRefreshNeeded = true;
}

// Client called, server run.
void AGoGamePawn::RequestSetup_Implementation()
{
	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (gameState)
	{
		UE_LOG(LogGoGamePawn, Log, TEXT("Replicating game state for client!"));
		this->ResetBoard(gameState->GetCurrentMatrix()->GetMatrixSize());

		// Replay the whole game history for the client.
		UE_LOG(LogGoGamePawn, Log, TEXT("Sending %d stone placements..."), gameState->placementHistory.Num());
		for (int i = 0; i < gameState->placementHistory.Num(); i++)
		{
			const GoGameMatrix::CellLocation& cellLocation = gameState->placementHistory[i];
			this->AlterGameState_OwningClient(cellLocation.i, cellLocation.j);
		}
	}
}

// Server called, client run.
void AGoGamePawn::ResetBoard_Implementation(int boardSize)
{
	UE_LOG(LogGoGamePawn, Log, TEXT("ResetBoard RPC called!"));

	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (gameState)
		gameState->ResetBoard(boardSize);
}

// Server called, client run (all clients).
void AGoGamePawn::AlterGameState_AllClients_Implementation(int i, int j)
{
	UE_LOG(LogGoGamePawn, Log, TEXT("AlterGameState_AllClients RPC called!"));

	this->AlterGameState_Shared(i, j);
}

// Server called, client run (client owning pawn).
void AGoGamePawn::AlterGameState_OwningClient_Implementation(int i, int j)
{
	UE_LOG(LogGoGamePawn, Log, TEXT("AlterGameState_OwningClient RPC called!"));

	this->AlterGameState_Shared(i, j);
}

void AGoGamePawn::AlterGameState_Shared(int i, int j)
{
	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (gameState)
	{
		GoGameMatrix::CellLocation cellLocation(i, j);

		UE_LOG(LogGoGamePawn, Log, TEXT("Client placing stone at location (%d, %d) as per the server's request."), cellLocation.i, cellLocation.j);

		bool altered = gameState->AlterGameState(cellLocation, gameState->GetCurrentMatrix()->GetWhoseTurn());
		if (!altered)
		{
			UE_LOG(LogGoGamePawn, Error, TEXT("Board alteration failed!  This shouldn't happen as the server vets all moves before relaying them to the client."));
		}
	}
}

// Client called, server run.
void AGoGamePawn::TryAlterGameState_Implementation(int i, int j)
{
	UE_LOG(LogGoGamePawn, Log, TEXT("TryAlterGameState RPC called!"));

	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	AGoGamePlayerController* playerController = Cast<AGoGamePlayerController>(this->Owner);
	if (gameState && playerController)
	{
		AGoGamePawn* gamePawn = Cast<AGoGamePawn>(playerController->GetPawn());
		if (gamePawn)
		{
			GoGameMatrix::CellLocation cellLocation;
			cellLocation.i = i;
			cellLocation.j = j;

			// Can the requested move be made?
			bool legalMove = false;
			bool altered = gameState->AlterGameState(cellLocation, gamePawn->myColor, &legalMove);
			check(!altered);
			if (legalMove)
			{
				// Yes.  Now go tell all the clients to apply the move.
				// Note that this will also execute locally to change the servers game state too.
				this->AlterGameState_AllClients(cellLocation.i, cellLocation.j);
			}
		}
	}
}

END_FUNCTION_BUILD_OPTIMIZATION