#include "GoGamePawnAI.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "GoGameIdiotAI.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGamePawnAI, Log, All);

AGoGamePawnAI::AGoGamePawnAI()
{
	this->stonePlacementSubmitted = false;
	this->gameIdiotAI = nullptr;
}

/*virtual*/ AGoGamePawnAI::~AGoGamePawnAI()
{
	delete this->gameIdiotAI;
}

/*virtual*/ void AGoGamePawnAI::BeginPlay()
{
	Super::BeginPlay();
}

/*virtual*/ void AGoGamePawnAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (gameState)
	{
		GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
		if (gameMatrix)
		{
			if (gameMatrix->GetWhoseTurn() == this->myColor)
			{
				if (!this->stonePlacementSubmitted)
				{
					if (!this->gameIdiotAI)
						this->gameIdiotAI = new GoGameIdiotAI(this->myColor);

					GoGameMatrix::CellLocation cellLocation;
					if (!this->gameIdiotAI->CalculateStonePlacement(gameState, cellLocation) || !gameMatrix->IsInBounds(cellLocation))
					{
						UE_LOG(LogGoGamePawnAI, Error, TEXT("AI pawn doesn't know what to do!  Going to pass..."));
						cellLocation.i = TNumericLimits<int>::Max();
						cellLocation.j = TNumericLimits<int>::Max();
					}

					UE_LOG(LogGoGamePawnAI, Error, TEXT("AI pawn decides to move at (%d, %d)."), cellLocation.i, cellLocation.j);
					this->TryAlterGameState(cellLocation.i, cellLocation.j);
					this->stonePlacementSubmitted = true;
				}
			}
			else
			{
				this->stonePlacementSubmitted = false;
			}
		}
	}
}