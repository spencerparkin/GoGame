#include "GoGameBoardPiece.h"
#include "GoGameMode.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "Kismet/GameplayStatics.h"

AGoGameBoardPiece::AGoGameBoardPiece()
{
}

/*virtual*/ AGoGameBoardPiece::~AGoGameBoardPiece()
{
}

BEGIN_FUNCTION_BUILD_OPTIMIZATION

// A game piece exists at every location on the board.
// For places where a piece has yet to be placed, the piece just doesn't render.
void AGoGameBoardPiece::HandleClick(UPrimitiveComponent* ClickedComp, FKey ButtonClicked)
{
	AGoGameMode* gameMode = Cast<AGoGameMode>(UGameplayStatics::GetGameMode(this->GetWorld()));
	if (!gameMode)
		return;

	AGoGameState* gameState = Cast<AGoGameState>(gameMode->GameState);
	if (!gameState)
		return;

	GoGameMatrix* newGameMatrix = new GoGameMatrix(gameState->GetCurrentMatrix());

	if (!newGameMatrix->SetCellState(this->cellLocation, newGameMatrix->GetWhoseTurn()))
		delete newGameMatrix;
	else
		gameState->PushMatrix(newGameMatrix);

	// TODO: Signal all the game pieces to update their color.  Should try to do this with events, I think.
}

END_FUNCTION_BUILD_OPTIMIZATION