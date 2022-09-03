#include "GoGameMinimax.h"
#include "GoGameState.h"
#include "Math/UnrealMathUtility.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGameMinimax, Log, All);

GoGameMinimax::GoGameMinimax(int lookAheadDepth, EGoGameCellState favoredPlayer)
{
	this->lookAheadDepth = lookAheadDepth;
	this->favoredPlayer = favoredPlayer;
	this->totalEvaluations = 0;
	this->totalEarlyOuts = 0;
	this->onlyVisitLiberties = true;
}

/*virtual*/ GoGameMinimax::~GoGameMinimax()
{
}

bool GoGameMinimax::CalculateBestNextMove(AGoGameState* gameState, GoGameMatrix::CellLocation& bestNextMove)
{
	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
	if (!gameMatrix)
		return false;

	if (gameMatrix->GetWhoseTurn() != this->favoredPlayer)
		return false;

	if (this->branchingCellSet.Num() == 0)
		return false;

	// We need to record the initial situation of the board so that we can compare it with each leaf of the game tree.
	this->baseStatus.AnalyzeBoard(gameMatrix);

	// Now go run minimax!
	this->totalEvaluations = 0;
	this->totalEarlyOuts = 0;
	int evaluation = 0;
	this->Minimax(gameState, 1, evaluation, &bestNextMove, nullptr);
	UE_LOG(LogGoGameMinimax, Display, TEXT("Minimax evaluated %d board states."), this->totalEvaluations);
	UE_LOG(LogGoGameMinimax, Display, TEXT("Was able to do %d early outs."), this->totalEarlyOuts);
	if (!gameMatrix->IsInBounds(bestNextMove))
	{
		UE_LOG(LogGoGameMinimax, Warning, TEXT("Minimax did not find a valid move."), this->totalEvaluations);
		bestNextMove.i = TNumericLimits<int>::Max();
		bestNextMove.j = TNumericLimits<int>::Max();
		return true;
	}
	
	// If we didn't find any favorable move, maybe we should pass?
	if (evaluation < 0)
	{
		// TODO: Hmmm...the AI seems to be passing too early in the game.  :/
		//       This bit of logic, by the way, is important.  I've seen the AI turn
		//       one of its immortal groups mortal, and I'm betting the move evaluation
		//       that it went with was negative.
		//bestNextMove.i = TNumericLimits<int>::Max();
		//bestNextMove.j = TNumericLimits<int>::Max();
	}

	return true;
}

// Note that alpha-beta pruning is implemented here as far as I understand the optimization.
// TODO: Could we possibly speed this up even more using memoization?  We could create a map
//       from matrix key to evaluation.  The matrix key would be an encoding of the entire
//       board into a string.  It's worth a try.  Whether it works depends on how often the
//       same board state is encountered again and again during the traversal process.  Note
//       that the memoization could and should be preserved across calls to minimax (obviously).
void GoGameMinimax::Minimax(AGoGameState* gameState, int currentDepth, int& finalEvaluation, GoGameMatrix::CellLocation* moveAssociatedWithEvaluation, int* supCurrentEval)
{
	this->totalEvaluations++;

	finalEvaluation = 0;

	if (moveAssociatedWithEvaluation)
	{
		moveAssociatedWithEvaluation->i = -1;
		moveAssociatedWithEvaluation->j = -1;
	}

	if (currentDepth >= this->lookAheadDepth)
	{
		GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
		
		BoardStatus currentStatus;
		currentStatus.AnalyzeBoard(gameMatrix);

		finalEvaluation = this->baseStatus.EvaluateAgainst(currentStatus, this->favoredPlayer);
	}
	else
	{
		GoGameMatrix* forbiddenMatrix = gameState->GetForbiddenMatrix();
		EGoGameCellState whoseTurn = gameState->GetCurrentMatrix()->GetWhoseTurn();
		FindType findType = (whoseTurn == this->favoredPlayer) ? FIND_MAX : FIND_MIN;

		int currentEval = 0;
		switch (findType)
		{
			case FIND_MIN:
			{
				currentEval = TNumericLimits<int>::Max();
				break;
			}
			case FIND_MAX:
			{
				currentEval = -TNumericLimits<int>::Max();
				break;
			}
		}

		TArray<GoGameMatrix::CellLocation> evaluationCellArray;

		// TODO: If more then one cell ties for the min or max, then maybe choose randomly from those?
		for (GoGameMatrix::CellLocation branchingCell : this->branchingCellSet)
		{
			if (this->onlyVisitLiberties && !gameState->GetCurrentMatrix()->IsLiberty(branchingCell))
				continue;

			gameState->PushMatrix(new GoGameMatrix(gameState->GetCurrentMatrix()));
	
			bool earlyOut = false;

			// If the board doesn't get altered, then the cell wasn't actually a playable location, and that's okay; just ignore it.
			bool altered = gameState->GetCurrentMatrix()->SetCellState(branchingCell, whoseTurn, forbiddenMatrix);
			if (altered)
			{
				int subEvaluation = 0;
				GoGameMatrix::CellLocation subEvaluationCell;
				this->Minimax(gameState, currentDepth + 1, subEvaluation, nullptr, &currentEval);
				
				switch (findType)
				{
					case FIND_MIN:
					{
						if (subEvaluation <= currentEval)
						{
							if (subEvaluation < currentEval)
							{
								currentEval = subEvaluation;
								evaluationCellArray.Reset();

								// If our current evaluation is less than the parent's current evaluation (and ours is only going to get smaller),
								// then we can early out, because the parent is trying to find the maximum evalation of its children.
								if (supCurrentEval && currentEval < *supCurrentEval)
									earlyOut = true;
							}

							if (moveAssociatedWithEvaluation)
								evaluationCellArray.Add(branchingCell);
						}
						break;
					}
					case FIND_MAX:
					{
						if (subEvaluation >= currentEval)
						{
							if (subEvaluation > currentEval)
							{
								currentEval = subEvaluation;
								evaluationCellArray.Reset();

								// If our current evaluation is greater than the parent's current evaluation (and ours is only going to get bigger),
								// then we can early out, because the parent is trying to find the minimum of its children.
								if (supCurrentEval && currentEval > *supCurrentEval)
									earlyOut = true;
							}

							if (moveAssociatedWithEvaluation)
								evaluationCellArray.Add(branchingCell);
						}

						break;
					}
				}
			}

			delete gameState->PopMatrix();

			if (earlyOut)
			{
				this->totalEarlyOuts++;
				break;
			}
		}

		finalEvaluation = currentEval;

		if (moveAssociatedWithEvaluation && evaluationCellArray.Num() > 0)
		{
			int i = FMath::RandRange(0, evaluationCellArray.Num() - 1);
			*moveAssociatedWithEvaluation = evaluationCellArray[i];
		}
	}
}

GoGameMinimax::BoardStatus::BoardStatus()
{
	this->blackTerritory = 0;
	this->whiteTerritory = 0;
	this->blackCaptures = 0;
	this->whiteCaptures = 0;
	this->numBlackImmortalGroups = 0;
	this->numWhiteImmortalGroups = 0;
	this->numBlackGroupsInAtari = 0;
	this->numWhiteGroupsInAtari = 0;
	this->totalBlackLiberties = 0;
	this->totalWhiteLiberties = 0;
}

/*virtual*/ GoGameMinimax::BoardStatus::~BoardStatus()
{
}

void GoGameMinimax::BoardStatus::AnalyzeBoard(GoGameMatrix* gameMatrix)
{
	int scoreDelta = 0;
	(void)gameMatrix->CalculateCurrentWinner(scoreDelta, this->blackTerritory, this->whiteTerritory);
	this->blackCaptures = gameMatrix->GetBlackCaptureCount();
	this->whiteCaptures = gameMatrix->GetWhiteCaptureCount();

	TArray<GoGameMatrix::ConnectedRegion*> blackImmortalGroupsArray;
	gameMatrix->FindAllImmortalGroupsOfColor(EGoGameCellState::Black, blackImmortalGroupsArray);
	this->numBlackImmortalGroups = blackImmortalGroupsArray.Num();
	for (auto group : blackImmortalGroupsArray)
		delete group;

	TArray<GoGameMatrix::ConnectedRegion*> whiteImmortalGroupsArray;
	gameMatrix->FindAllImmortalGroupsOfColor(EGoGameCellState::White, whiteImmortalGroupsArray);
	this->numWhiteImmortalGroups = whiteImmortalGroupsArray.Num();
	for (auto group : whiteImmortalGroupsArray)
		delete group;

	TArray<GoGameMatrix::ConnectedRegion*> blackGroupsArray;
	gameMatrix->CollectAllRegionsOfType(EGoGameCellState::Black, blackGroupsArray);
	this->numBlackGroupsInAtari = 0;
	this->totalBlackLiberties = 0;
	for (int i = 0; i < blackGroupsArray.Num(); i++)
	{
		GoGameMatrix::ConnectedRegion* blackGroup = blackGroupsArray[i];
		if (blackGroup->libertiesSet.Num() == 1)
			this->numBlackGroupsInAtari++;
		this->totalBlackLiberties += blackGroup->libertiesSet.Num();
		delete blackGroup;
	}

	TArray<GoGameMatrix::ConnectedRegion*> whiteGroupsArray;
	gameMatrix->CollectAllRegionsOfType(EGoGameCellState::White, whiteGroupsArray);
	this->numWhiteGroupsInAtari = 0;
	for (int i = 0; i < whiteGroupsArray.Num(); i++)
	{
		GoGameMatrix::ConnectedRegion* whiteGroup = whiteGroupsArray[i];
		if (whiteGroup->libertiesSet.Num() == 1)
			this->numWhiteGroupsInAtari++;
		this->totalWhiteLiberties += whiteGroup->libertiesSet.Num();
		delete whiteGroup;
	}
}

int GoGameMinimax::BoardStatus::EvaluateAgainst(const BoardStatus& futureStatus, EGoGameCellState favoredPlayer) const
{
	int evaluation = 0;

	// TODO: I'm really not sure what a good evaluation function would be here.  Maybe do some research online.
	switch(favoredPlayer)
	{
		case EGoGameCellState::Black:
		{
			evaluation += (futureStatus.blackTerritory - this->blackTerritory) * 100;
			evaluation -= (futureStatus.whiteTerritory - this->whiteTerritory) * 50;

			evaluation += (futureStatus.blackCaptures - this->blackCaptures) * 50000;
			evaluation -= (futureStatus.whiteCaptures - this->whiteCaptures) * 100000;

			evaluation += (futureStatus.numBlackImmortalGroups - this->numBlackImmortalGroups) * 10;
			evaluation -= (futureStatus.numWhiteImmortalGroups - this->numWhiteImmortalGroups) * 5;

			evaluation -= (futureStatus.numBlackGroupsInAtari - this->numBlackGroupsInAtari) * 1000;
			evaluation += (futureStatus.numWhiteGroupsInAtari - this->numWhiteGroupsInAtari) * 500;

			evaluation += (futureStatus.totalBlackLiberties - this->totalBlackLiberties) * 10;
			evaluation -= (futureStatus.totalWhiteLiberties - this->totalWhiteLiberties) * 5;

			break;
		}
		case EGoGameCellState::White:
		{
			evaluation += (futureStatus.whiteTerritory - this->whiteTerritory) * 100;
			evaluation -= (futureStatus.blackTerritory - this->blackTerritory) * 50;

			evaluation += (futureStatus.whiteCaptures - this->whiteCaptures) * 50000;
			evaluation -= (futureStatus.blackCaptures - this->blackCaptures) * 100000;

			evaluation += (futureStatus.numWhiteImmortalGroups - this->numWhiteImmortalGroups) * 10;
			evaluation -= (futureStatus.numBlackImmortalGroups - this->numBlackImmortalGroups) * 5;

			evaluation -= (futureStatus.numWhiteGroupsInAtari - this->numWhiteGroupsInAtari) * 1000;
			evaluation += (futureStatus.numBlackGroupsInAtari - this->numBlackGroupsInAtari) * 500;

			evaluation += (futureStatus.totalWhiteLiberties - this->totalWhiteLiberties) * 10;
			evaluation -= (futureStatus.totalBlackLiberties - this->totalBlackLiberties) * 5;

			break;
		}
	}

	return evaluation;
}