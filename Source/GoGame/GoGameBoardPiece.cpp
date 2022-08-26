#include "GoGameBoardPiece.h"
#include "GoGameBoard.h"
#include "GoGameMode.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "GoGamePawn.h"
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
}

// A game piece exists at every location on the board.
// For places where a piece has yet to be placed, the piece just doesn't render.
void AGoGameBoardPiece::HandleClick(UPrimitiveComponent* ClickedComp, FKey ButtonClicked)
{
	// TODO: I don't know, but in the midst of players joining the game, UE is unpossessing and repossessing the player controller and we end up here having no pawn at all.  WTF is going on?
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