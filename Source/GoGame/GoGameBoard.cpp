#include "GoGameBoard.h"
#include "GoGameMode.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "GoGameBoardPiece.h"
#include "Kismet/GameplayStatics.h"

AGoGameBoard::AGoGameBoard()
{
}

/*virtual*/ AGoGameBoard::~AGoGameBoard()
{
}

BEGIN_FUNCTION_BUILD_OPTIMIZATION

/*virtual*/ void AGoGameBoard::BeginPlay()
{
	Super::BeginPlay();

	FVector boardCenter = this->GetActorLocation();

	GoGameMatrix* gameMatrix = GetCurrentMatrix();
	if (!gameMatrix)
		return;
		
	UClass* boardPieceClass = this->gameBoardPieceClass.LoadSynchronous();
	if (!boardPieceClass)
		return;

	int size = gameMatrix->GetMatrixSize();
	if (size <= 0)
		return;

	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			FVector delta(float(i) - float(size - 1) / 2.0f, float(j) - float(size - 1) / 2.0f, 0.0f);
			float scale = 93.0f / float(size);
			FVector boardPieceLocation = boardCenter + delta * scale;
			boardPieceLocation.Z = 4.0f;

			FRotator boardPieceRotation(0.0f, 0.0f, 0.0f);
			
			FVector boardPieceScale(1.0f, 1.0f, 1.0f);
			boardPieceScale *= 19.0f / float(size);

			FActorSpawnParameters spawnParams;
			spawnParams.Owner = this;

			FTransform transform;
			transform.SetIdentity();
			transform.SetLocation(boardPieceLocation);
			transform.SetRotation(boardPieceRotation.Quaternion());

			AGoGameBoardPiece* boardPiece = this->GetWorld()->SpawnActor<AGoGameBoardPiece>(boardPieceClass, transform, spawnParams);

			boardPiece->cellLocation.i = i;
			boardPiece->cellLocation.j = j;

			boardPiece->SetActorScale3D(boardPieceScale);

			// TODO: We might want to setup an attachment here for each piece so that the pieces move with the board.
			//       For now, the board never moves, so there isn't much point to doing that.
		}
	}

	this->UpdateMaterial();
	this->UpdateRender();
}

GoGameMatrix* AGoGameBoard::GetCurrentMatrix()
{
	AGoGameMode* gameMode = Cast<AGoGameMode>(UGameplayStatics::GetGameMode(this->GetWorld()));
	if (!gameMode)
		return nullptr;

	AGoGameState* gameState = Cast<AGoGameState>(gameMode->GameState);
	if (!gameState)
		return nullptr;

	return gameState->GetCurrentMatrix();
}

int AGoGameBoard::GetMatrixSize()
{
	GoGameMatrix* gameMatrix = this->GetCurrentMatrix();
	if (!gameMatrix)
		return 0;

	return gameMatrix->GetMatrixSize();
}

void AGoGameBoard::UpdateRender()
{
	TArray<AActor*> actorArray;
	UGameplayStatics::GetAllActorsOfClass(this->GetWorld(), AGoGameBoardPiece::StaticClass(), actorArray);

	for (int i = 0; i < actorArray.Num(); i++)
	{
		AGoGameBoardPiece* boardPiece = Cast<AGoGameBoardPiece>(actorArray[i]);
		if (boardPiece)
			boardPiece->UpdateRender();
	}
}

END_FUNCTION_BUILD_OPTIMIZATION