#include "GoGameBoardPiece.h"
#include "GoGameBoard.h"
#include "GoGameMode.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "Kismet/GameplayStatics.h"

AGoGameBoardPiece::AGoGameBoardPiece()
{
	this->highlighted = false;
	this->cellLocation.i = -1;
	this->cellLocation.j = -1;
	this->bReplicates = true;
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
	// TODO: We should not do this directly.  Rather, we should make an RPC to the server asking
	//       it to do the move, then if the server says it's okay, it will tell all the clients
	//       to make the move on our end.  This is how we can keep the game-state synchronized
	//       across all clients and the server.

	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (!gameState)
		return;

	// TODO: Fix crash.  There is no current matrix on the client; only on the server, because
	//       the game mode initialized the game state on the server, but not the client.  So I
	//       think that there has to be an RPC called by the server on the client to tell it
	//       to initialize the state.  The server could store the sequence of all cell locations
	//       thus far made (typically the empty set) and then send that to the client.  I don't know.
	//       Consider a spectator coming to see the game.  How would their state get synchronized?
	//       In everything, we might only need two RPCs: one of the client asking the server to
	//       make a move, and one of the server telling the client to make a move.
	GoGameMatrix* newGameMatrix = new GoGameMatrix(gameState->GetCurrentMatrix());

	if (!newGameMatrix->SetCellState(this->cellLocation, newGameMatrix->GetWhoseTurn(), gameState->GetForbiddenMatrix()))
		delete newGameMatrix;
	else
		gameState->PushMatrix(newGameMatrix);

	AGoGameBoard* gameBoard = Cast<AGoGameBoard>(this->Owner);
	if (gameBoard)
		gameBoard->OnBoardAppearanceChanged.Broadcast();
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