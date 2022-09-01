#include "GoGameGroupMinimax.h"
#include "GoGameState.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGameMinimax, Log, All);

GoGameGroupMinimax::GoGameGroupMinimax(int lookAheadDepth, EGoGameCellState favoredPlayer)
{
	this->lookAheadDepth = lookAheadDepth;
	this->favoredPlayer = favoredPlayer;
	this->opposingPlayer = (EGoGameCellState::Black == favoredPlayer) ? EGoGameCellState::White : EGoGameCellState::Black;
	this->groupColor = EGoGameCellState::Empty;
	this->totalEvaluations = 0;
}

/*virtual*/ GoGameGroupMinimax::~GoGameGroupMinimax()
{
}

bool GoGameGroupMinimax::CalculateBestNextMove(AGoGameState* gameState, const GoGameMatrix::CellLocation& targetGroupRep, GoGameMatrix::CellLocation& bestNextMove)
{
	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
	if (!gameMatrix)
		return false;

	if (!gameMatrix->GetCellState(targetGroupRep, this->groupColor))
		return false;

	if (this->groupColor != EGoGameCellState::Black && this->groupColor != EGoGameCellState::White)
		return false;
	
	if (gameMatrix->GetWhoseTurn() != this->favoredPlayer)
		return false;

	this->totalEvaluations = 0;
	float evaluation = 0.0f;
	this->Minimax(gameState, targetGroupRep, 1, evaluation, bestNextMove);
	UE_LOG(LogGoGameMinimax, Display, TEXT("Minimax evaluated %d board states."), this->totalEvaluations);
	if (bestNextMove.i < 0 || bestNextMove.j < 0)
	{
		// This is not necessarily an error.  The group in question may be immortal or have mutual life.
		UE_LOG(LogGoGameMinimax, Warning, TEXT("Minimax did not find a valid move."), this->totalEvaluations);
		return false;
	}

	return true;
}

// TODO: Alpha-beta pruning?
void GoGameGroupMinimax::Minimax(AGoGameState* gameState, const GoGameMatrix::CellLocation& targetGroupRep, int currentDepth, float& evaluation, GoGameMatrix::CellLocation& moveAssociatedWithEvaluation)
{
	this->totalEvaluations++;

	evaluation = 0.0f;
	moveAssociatedWithEvaluation.i = -1;
	moveAssociatedWithEvaluation.j = -1;
	
	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
	GoGameMatrix::ConnectedRegion* group = gameMatrix->SenseConnectedRegion(targetGroupRep);
	EGoGameCellState whoseTurn = gameState->GetCurrentMatrix()->GetWhoseTurn();

	if (group->type == GoGameMatrix::ConnectedRegion::TERRITORY)	// The group was captured!
	{
		if (this->favoredPlayer == this->groupColor)
			evaluation = TNumericLimits<float>::Min();
		else
			evaluation = TNumericLimits<float>::Max();
	}
	else if (currentDepth >= this->lookAheadDepth)
	{
		// TODO: This metric might not be the best.  Maybe we should also count liberties
		//       of other groups, even thoughs for both players.  A better metric could
		//       result in a smarter minimax algorithm.
		if (this->favoredPlayer == this->groupColor)
			evaluation = float(group->libertiesSet.Num());
		else
			evaluation = -float(group->libertiesSet.Num());
	}
	else
	{
		float minEvaluation = TNumericLimits<float>::Max();
		float maxEvaluation = -TNumericLimits<float>::Max();

		GoGameMatrix::CellLocation minEvaluationCell, maxEvaluationCell;
		GoGameMatrix* forbiddenMatrix = gameState->GetForbiddenMatrix();

		// The larger the space we search, the smarter our minimax should be, but
		// this also increases our branch factor, making it take exponentially longer.
		TSet<GoGameMatrix::CellLocation> emptyCellSet = group->libertiesSet;
		this->ExpandEmptyCellSet(gameMatrix, emptyCellSet, 1);

		// TODO: If more then one cell ties for the min or max, then maybe choose randomly from those?
		for (GoGameMatrix::CellLocation emptyCell : emptyCellSet)
		{
			gameState->PushMatrix(new GoGameMatrix(gameState->GetCurrentMatrix()));
	
			// If the board doesn't get altered, then the cell wasn't actually playable, but that's okay.
			bool altered = gameState->GetCurrentMatrix()->SetCellState(emptyCell, whoseTurn, forbiddenMatrix);
			if (altered)
			{
				float subEvaluation = 0.0f;
				GoGameMatrix::CellLocation subEvaluationCell;
				this->Minimax(gameState, targetGroupRep, currentDepth + 1, subEvaluation, subEvaluationCell);
				
				if (subEvaluation < minEvaluation)
				{
					minEvaluation = subEvaluation;
					minEvaluationCell = emptyCell;
				}

				if (subEvaluation > maxEvaluation)
				{
					maxEvaluation = subEvaluation;
					maxEvaluationCell = emptyCell;
				}
			}

			delete gameState->PopMatrix();
		}

		if(whoseTurn == this->favoredPlayer)
		{
			evaluation = maxEvaluation;
			moveAssociatedWithEvaluation = maxEvaluationCell;
		}
		else
		{
			evaluation = minEvaluation;
			moveAssociatedWithEvaluation = minEvaluationCell;
		}
	}

	delete group;
}

void GoGameGroupMinimax::ExpandEmptyCellSet(GoGameMatrix* gameMatrix, TSet<GoGameMatrix::CellLocation>& emptyCellSet, int iterations)
{
	for (int i = 0; i < iterations; i++)
	{
		TArray<GoGameMatrix::CellLocation> cellArray;
		for (GoGameMatrix::CellLocation cellLocation : emptyCellSet)
			cellArray.Add(cellLocation);

		for (int j = 0; j < cellArray.Num(); j++)
		{
			const GoGameMatrix::CellLocation& cellLocation = cellArray[j];
			TArray<GoGameMatrix::CellLocation> adjacentLocationArray;
			adjacentLocationArray.Add(GoGameMatrix::CellLocation(cellLocation.i - 1, cellLocation.j));
			adjacentLocationArray.Add(GoGameMatrix::CellLocation(cellLocation.i + 1, cellLocation.j));
			adjacentLocationArray.Add(GoGameMatrix::CellLocation(cellLocation.i, cellLocation.j - 1));
			adjacentLocationArray.Add(GoGameMatrix::CellLocation(cellLocation.i, cellLocation.j + 1));

			for (int k = 0; k < adjacentLocationArray.Num(); k++)
			{
				const GoGameMatrix::CellLocation& adjacentLocation = adjacentLocationArray[k];
				EGoGameCellState cellState;
				if (gameMatrix->GetCellState(adjacentLocation, cellState) && cellState == EGoGameCellState::Empty && !emptyCellSet.Contains(adjacentLocation))
					emptyCellSet.Add(adjacentLocation);
			}
		}
	}
}