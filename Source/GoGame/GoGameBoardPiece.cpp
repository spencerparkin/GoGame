#include "GoGameBoardPiece.h"
#include "GoGameBoard.h"
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

	AGoGameBoard* gameBoard = Cast<AGoGameBoard>(this->Owner);
	if (gameBoard)
		gameBoard->UpdateRender();
}

EGoGameCellState AGoGameBoardPiece::GetPieceColor()
{
	AGoGameMode* gameMode = Cast<AGoGameMode>(UGameplayStatics::GetGameMode(this->GetWorld()));
	if (!gameMode)
		return EGoGameCellState::Empty;

	AGoGameState* gameState = Cast<AGoGameState>(gameMode->GameState);
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