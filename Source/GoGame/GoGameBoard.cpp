#include "GoGameBoard.h"
#include "GoGameMode.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "GoGameBoardPiece.h"
#include "Kismet/GameplayStatics.h"

AGoGameBoard::AGoGameBoard()
{
	this->recreatePieces = true;
}

/*virtual*/ AGoGameBoard::~AGoGameBoard()
{
}

BEGIN_FUNCTION_BUILD_OPTIMIZATION

/*virtual*/ void AGoGameBoard::BeginPlay()
{
	Super::BeginPlay();

	if (!IsRunningDedicatedServer())
		this->OnBoardAppearanceChanged.AddDynamic(this, &AGoGameBoard::UpdateAppearance);

	this->recreatePieces = true;
}

void AGoGameBoard::GatherPieces(const TSet<GoGameMatrix::CellLocation>& locationSet, TArray<AGoGameBoardPiece*>& boardPieceArray)
{
	TArray<AActor*> actorArray;
	UGameplayStatics::GetAllActorsOfClass(this->GetWorld(), AGoGameBoardPiece::StaticClass(), actorArray);
	for (int i = 0; i < actorArray.Num(); i++)
	{
		AGoGameBoardPiece* boardPiece = Cast<AGoGameBoardPiece>(actorArray[i]);
		if (locationSet.Contains(boardPiece->cellLocation))
			boardPieceArray.Add(boardPiece);
	}
}

GoGameMatrix* AGoGameBoard::GetCurrentMatrix()
{
	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
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

void AGoGameBoard::UpdateAppearance()
{
	if (this->recreatePieces)
	{
		this->recreatePieces = false;

		// TODO: Detach and destroy all currently attached pieces before we proceed.

		FVector boardCenter = this->GetActorLocation();
		GoGameMatrix* gameMatrix = GetCurrentMatrix();
		UClass* boardPieceClass = this->gameBoardPieceClass.LoadSynchronous();
		int size = gameMatrix->GetMatrixSize();
		if (size > 1 && boardPieceClass && gameMatrix)
		{
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

					FAttachmentTransformRules rules(EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, false);
					boardPiece->GetRootComponent()->AttachToComponent(this->GetRootComponent(), rules);

					boardPiece->UpdateRender();
				}
			}
		}
	}

	this->UpdateMaterial();
}

END_FUNCTION_BUILD_OPTIMIZATION