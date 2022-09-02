#include "GoGameMinimax.h"
#include "GoGameState.h"
#include "Math/UnrealMathUtility.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGameMinimax, Log, All);

GoGameMinimax::GoGameMinimax(int lookAheadDepth, EGoGameCellState favoredPlayer)
{
	this->lookAheadDepth = lookAheadDepth;
	this->favoredPlayer = favoredPlayer;
	this->totalEvaluations = 0;
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
	int evaluation = 0;
	this->Minimax(gameState, 1, evaluation, bestNextMove);
	UE_LOG(LogGoGameMinimax, Display, TEXT("Minimax evaluated %d board states."), this->totalEvaluations);
	if (!gameMatrix->IsInBounds(bestNextMove))
	{
		UE_LOG(LogGoGameMinimax, Warning, TEXT("Minimax did not find a valid move."), this->totalEvaluations);
		return false;
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

// TODO: Alpha-beta pruning?
void GoGameMinimax::Minimax(AGoGameState* gameState, int currentDepth, int& evaluation, GoGameMatrix::CellLocation& moveAssociatedWithEvaluation)
{
	this->totalEvaluations++;

	evaluation = 0;
	moveAssociatedWithEvaluation.i = -1;
	moveAssociatedWithEvaluation.j = -1;

	if (currentDepth >= this->lookAheadDepth)
	{
		GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
		
		BoardStatus currentStatus;
		currentStatus.AnalyzeBoard(gameMatrix);

		evaluation = this->baseStatus.EvaluateAgainst(currentStatus, this->favoredPlayer);
	}
	else
	{
		int minEvaluation = TNumericLimits<int>::Max();
		int maxEvaluation = -TNumericLimits<int>::Max();

		TArray<GoGameMatrix::CellLocation> minEvaluationCellArray;
		TArray<GoGameMatrix::CellLocation> maxEvaluationCellArray;

		GoGameMatrix* forbiddenMatrix = gameState->GetForbiddenMatrix();
		EGoGameCellState whoseTurn = gameState->GetCurrentMatrix()->GetWhoseTurn();

		// TODO: If more then one cell ties for the min or max, then maybe choose randomly from those?
		for (GoGameMatrix::CellLocation branchingCell : this->branchingCellSet)
		{
			gameState->PushMatrix(new GoGameMatrix(gameState->GetCurrentMatrix()));
	
			// If the board doesn't get altered, then the cell wasn't actually a playable location, and that's okay; just ignore it.
			bool altered = gameState->GetCurrentMatrix()->SetCellState(branchingCell, whoseTurn, forbiddenMatrix);
			if (altered)
			{
				int subEvaluation = 0;
				GoGameMatrix::CellLocation subEvaluationCell;
				this->Minimax(gameState, currentDepth + 1, subEvaluation, subEvaluationCell);
				
				if (subEvaluation <= minEvaluation)
				{
					if (subEvaluation < minEvaluation)
					{
						minEvaluation = subEvaluation;
						minEvaluationCellArray.Reset();
					}

					minEvaluationCellArray.Add(branchingCell);
				}

				if (subEvaluation >= maxEvaluation)
				{
					if (subEvaluation > maxEvaluation)
					{
						maxEvaluation = subEvaluation;
						maxEvaluationCellArray.Reset();
					}

					maxEvaluationCellArray.Add(branchingCell);
				}
			}

			delete gameState->PopMatrix();
		}

		if(whoseTurn == this->favoredPlayer)
		{
			evaluation = maxEvaluation;
			if (maxEvaluationCellArray.Num() > 0)
			{
				int i = FMath::RandRange(0, maxEvaluationCellArray.Num() - 1);
				moveAssociatedWithEvaluation = maxEvaluationCellArray[i];
			}
		}
		else
		{
			evaluation = minEvaluation;
			if (minEvaluationCellArray.Num() > 0)
			{
				int i = FMath::RandRange(0, minEvaluationCellArray.Num() - 1);
				moveAssociatedWithEvaluation = minEvaluationCellArray[i];
			}
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

	int territoryWeight = 10;
	evaluation += (futureStatus.blackTerritory - this->blackTerritory) * territoryWeight;
	evaluation -= (futureStatus.whiteTerritory - this->whiteTerritory) * territoryWeight;

	int captureWeight = 1000;
	evaluation += (futureStatus.blackCaptures - this->blackCaptures) * captureWeight;
	evaluation -= (futureStatus.whiteCaptures - this->whiteCaptures) * captureWeight;

	int immortalGroupWeight = 1;
	evaluation += (futureStatus.numBlackImmortalGroups - this->numBlackImmortalGroups) * immortalGroupWeight;
	evaluation -= (futureStatus.numWhiteImmortalGroups - this->numWhiteImmortalGroups) * immortalGroupWeight;

	int groupsInAtariWeight = 100;
	evaluation += (this->numBlackGroupsInAtari - futureStatus.numBlackGroupsInAtari) * groupsInAtariWeight;
	evaluation -= (this->numWhiteGroupsInAtari - futureStatus.numWhiteGroupsInAtari) * groupsInAtariWeight;

	int libertiesWeight = 1;
	evaluation += (futureStatus.totalBlackLiberties - this->totalBlackLiberties) * libertiesWeight;
	evaluation -= (futureStatus.totalWhiteLiberties - this->totalWhiteLiberties) * libertiesWeight;

	if (favoredPlayer == EGoGameCellState::White)
		evaluation *= -1;

	return evaluation;
}