#include "GoGamePawnAI.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "GoGameIdiotAI.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGamePawnAI, Log, All);

AGoGamePawnAI::AGoGamePawnAI()
{
	this->state = State::STANDBY;
	this->gameAI = nullptr;
	this->turnBeginTime = 0.0f;
	this->turnMinTime = 2.0f;
	this->turnMaxTime = 10.0f;
}

/*virtual*/ AGoGamePawnAI::~AGoGamePawnAI()
{
	delete this->gameAI;
}

/*virtual*/ void AGoGamePawnAI::BeginPlay()
{
	Super::BeginPlay();
}

/*virtual*/ void AGoGamePawnAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (!gameState)
		return;
	
	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
	if (!gameMatrix)
		return;

	switch (this->state)
	{
		case State::STANDBY:
		{
			// Wait here until it's our turn.
			if(gameMatrix->GetWhoseTurn() == this->myColor)
				this->state = State::BEGIN_TURN;
			
			break;
		}
		case State::BEGIN_TURN:
		{
			UE_LOG(LogGoGamePawnAI, Error, TEXT("AI turn begins!"));

			if (!this->gameAI)
			{
				// TODO: We might expirament with other GoGameAI derivatives.  For now, just always use the idiot.
				this->gameAI = new GoGameIdiotAI(this->myColor);
				this->gameAI->gameState = gameState;
			}

			// This is where an AI might kick-off its threads.
			this->gameAI->BeginThinking();
			this->turnBeginTime = this->GetWorld()->GetTimeSeconds();
			this->state = State::TICK_TURN;
			break;
		}
		case State::TICK_TURN:
		{
			// This is where an AI might iterate a game tree (e.g., the MCTS algorithm) or where
			// the AI might monitor its threads or something like that.
			bool isFinished = this->gameAI->TickThinking();

			// The AI's turn expires when its done and the minimum thinking time is reached, or
			// when the maximum thinking time has been exceeded.  The minimum is important so that
			// the human player sees some sort of time pass after their turn is taken and before
			// the computer takes its turn.  A good AI would probably need all the time it can get.
			float currentTime = this->GetWorld()->GetTimeSeconds();
			float turnElapsedTime = currentTime - this->turnBeginTime;
			if((isFinished && turnElapsedTime >= this->turnMinTime) || turnElapsedTime >= this->turnMaxTime)
				this->state = State::END_TURN;

			break;
		}
		case State::END_TURN:
		{
			UE_LOG(LogGoGamePawnAI, Error, TEXT("AI turn ends!"));

			// This is where an AI might join its threads.
			this->gameAI->StopThinking();

			GoGameMatrix::CellLocation cellLocation = this->gameAI->stonePlacement;
			UE_LOG(LogGoGamePawnAI, Error, TEXT("AI pawn decides to move at (%d, %d)."), cellLocation.i, cellLocation.j);

			this->TryAlterGameState(cellLocation.i, cellLocation.j);

			this->state = State::WAIT_FOR_TURN_FLIP;
			break;
		}
		case State::WAIT_FOR_TURN_FLIP:
		{
			// Note that if the move calculated by the AI is bad, we'll be waiting here indefinitely.
			if (gameMatrix->GetWhoseTurn() != this->myColor)
				this->state = State::STANDBY;

			break;
		}
		default:
		{
			this->state = State::STANDBY;
			break;
		}
	}
}