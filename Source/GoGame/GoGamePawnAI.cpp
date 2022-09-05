#include "GoGamePawnAI.h"
#include "GoGameMCTS.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGamePawnAI, Log, All);

AGoGamePawnAI::AGoGamePawnAI()
{
	this->gameMCTS = nullptr;
	this->thinkTimeStart = 0.0f;
	this->thinkTimeMax = 30.0f;
	this->stonePlacementSubmitted = false;
}

/*virtual*/ AGoGamePawnAI::~AGoGamePawnAI()
{
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
					if (!this->gameMCTS)
					{
						this->gameMCTS = new GoGameMCTS();
						this->gameMCTS->gameMatrix = new GoGameMatrix(gameMatrix);
						this->gameMCTS->favoredPlayer = this->myColor;
						this->thinkTimeStart = this->GetWorld()->GetTimeSeconds();
					}

					this->gameMCTS->PerformSingleIteration();

					float currentTime = this->GetWorld()->GetTimeSeconds();
					float elapsedTime = currentTime - this->thinkTimeStart;
					if (elapsedTime >= this->thinkTimeMax)
					{
						UE_LOG(LogGoGamePawnAI, Display, TEXT("MCTS out of time.  Did %d iterations."), this->gameMCTS->totalIterationCount);

						GoGameMatrix::CellLocation stonePlacement;
						if (!this->gameMCTS->GetEstimatedBestMove(stonePlacement))
						{
							stonePlacement.i = TNumericLimits<int>::Max();
							stonePlacement.j = TNumericLimits<int>::Max();
						}

						this->TryAlterGameState(stonePlacement.i, stonePlacement.j);
						this->stonePlacementSubmitted = true;
					}
				}
			}
			else
			{
				// I had an idea to carry over the MCTS tree from one turn to the next, but just do this for now.
				if (this->gameMCTS)
				{
					delete this->gameMCTS;
					this->gameMCTS = nullptr;
				}

				this->stonePlacementSubmitted = false;
			}
		}
	}
}