#include "GoGameBoardPiece.h"
#include "GoGameBoard.h"
#include "GoGameMode.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "GoGamePawn.h"
#include "GoGameModule.h"
#include "GoGameOptions.h"
#include "GoGamePlayerController.h"
#include "Kismet/GameplayStatics.h"

AGoGameBoardPiece::AGoGameBoardPiece()
{
	this->highlighted = false;
	this->cellLocation.i = -1;
	this->cellLocation.j = -1;
}

/*virtual*/ AGoGameBoardPiece::~AGoGameBoardPiece()
{
}

BEGIN_FUNCTION_BUILD_OPTIMIZATION

/*virtual*/ void AGoGameBoardPiece::BeginPlay()
{
	Super::BeginPlay();		// This calls the BP begin-play which binds a delegate to call our HandleClick method.

	AGoGameBoard* gameBoard = Cast<AGoGameBoard>(this->Owner);
	if (gameBoard)
		gameBoard->OnBoardAppearanceChanged.AddDynamic(this, &AGoGameBoardPiece::UpdateAppearance);
}

void AGoGameBoardPiece::UpdateAppearance()
{
	this->UpdateRender();

	AGoGameBoard* gameBoard = Cast<AGoGameBoard>(this->Owner);
	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (gameBoard && gameBoard->gamePointer && gameState)
	{
		if (gameState->placementHistory.Num() > 0)
		{
			bool showPointer = false;
			GoGameModule* gameModule = (GoGameModule*)FModuleManager::Get().GetModule("GoGame");
			if (gameModule)
				showPointer = gameModule->gameOptions->showPointerToMostRecentlyPlacedStone;

			gameBoard->gamePointer->SetActorHiddenInGame(!showPointer);

			const GoGameMatrix::CellLocation& lastCellLocation = gameState->placementHistory[gameState->placementHistory.Num() - 1];
			if (this->cellLocation == lastCellLocation)
			{
				if (gameBoard->gamePointer->GetAttachParentActor() != this)
				{
					FDetachmentTransformRules detachRules(EDetachmentRule::KeepRelative, EDetachmentRule::KeepRelative, EDetachmentRule::KeepRelative, false);
					gameBoard->gamePointer->DetachFromActor(detachRules);
				}

				if (showPointer)
				{
					// TODO: Is this interfering with line trace for the click event for placing stones?  I think it is.  How to fix?
					FAttachmentTransformRules attachRules(EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, false);
					gameBoard->gamePointer->AttachToActor(this, attachRules);
				}
			}
		}
	}
}

// A game piece exists at every location on the board.
// For places where a piece has yet to be placed, the piece just doesn't render.
void AGoGameBoardPiece::HandleClick(UPrimitiveComponent* ClickedComp, FKey ButtonClicked)
{
	// TODO: Need to move this functionality to the pawn, because the pointer interferes with the line trace.
	//       Rather, we should handle a click in the pawn and do our own line-trace with an ignore-actor set with the pointer object.
	AGoGamePlayerController* playerController = Cast<AGoGamePlayerController>(UGameplayStatics::GetPlayerController(this->GetWorld(), 0));
	if (playerController)
	{
		AGoGamePawn* gamePawn = Cast<AGoGamePawn>(playerController->GetPawn());
		if(gamePawn)
			gamePawn->TryAlterGameState(this->cellLocation.i, this->cellLocation.j);
	}
}

EGoGameCellState AGoGameBoardPiece::GetPieceColor()
{
	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (!gameState)
		return EGoGameCellState::Empty;

	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
	if (!gameMatrix)
		return EGoGameCellState::Empty;

	EGoGameCellState cellState;
	if (!gameMatrix->GetCellState(this->cellLocation, cellState))
		return EGoGameCellState::Empty;

	return cellState;
}

END_FUNCTION_BUILD_OPTIMIZATION