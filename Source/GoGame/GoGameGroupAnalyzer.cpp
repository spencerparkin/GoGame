#include "GoGameGroupAnalyzer.h"
#include "GoGameState.h"

GoGameGroupAnalyzer::GoGameGroupAnalyzer(int lookAheadDepth, EGoGameCellState favoredPlayer)
{
	this->lookAheadDepth = lookAheadDepth;
	this->favoredPlayer = favoredPlayer;
	this->opposingPlayer = (EGoGameCellState::Black == favoredPlayer) ? EGoGameCellState::White : EGoGameCellState::Black;
	this->groupColor = EGoGameCellState::Empty;
}

/*virtual*/ GoGameGroupAnalyzer::~GoGameGroupAnalyzer()
{
}

bool GoGameGroupAnalyzer::CalculateBestNextMove(AGoGameState* gameState, const GoGameMatrix::CellLocation& targetGroupRep, GoGameMatrix::CellLocation& bestNextMove)
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

	float evaluation = 0.0f;
	this->Minimax(gameState, targetGroupRep, 1, evaluation, bestNextMove);
	return true;
}

// TODO: Alpha-beta pruning?
void GoGameGroupAnalyzer::Minimax(AGoGameState* gameState, const GoGameMatrix::CellLocation& targetGroupRep, int currentDepth, float& evaluation, GoGameMatrix::CellLocation& moveAssociatedWithEvaluation)
{
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
		if (this->favoredPlayer == this->groupColor)
			evaluation = float(group->libertiesSet.Num());
		else
			evaluation = -float(group->libertiesSet.Num());
	}
	else
	{
		float minEvaluation = TNumericLimits<float>::Max();
		float maxEvaluation = TNumericLimits<float>::Min();

		GoGameMatrix::CellLocation minEvaluationCell, maxEvaluationCell;
		minEvaluationCell.i = -1;
		minEvaluationCell.j = -1;
		maxEvaluationCell.i = -1;
		maxEvaluationCell.j = -1;

		// TODO: If more then one cell ties for the min or max, then maybe choose randomly from those?
		for (GoGameMatrix::CellLocation libertyCell : group->libertiesSet)
		{
			gameState->PushMatrix(gameState->GetCurrentMatrix());
	
			bool altered = gameState->GetCurrentMatrix()->SetCellState(libertyCell, whoseTurn, gameState->GetForbiddenMatrix());
			if (altered)
			{
				float subEvaluation = 0.0f;
				GoGameMatrix::CellLocation subEvaluationCell;
				this->Minimax(gameState, targetGroupRep, currentDepth + 1, subEvaluation, subEvaluationCell);
				
				if (subEvaluation < minEvaluation)
				{
					minEvaluation = subEvaluation;
					minEvaluationCell = libertyCell;
				}
				if (subEvaluation > maxEvaluation)
				{
					maxEvaluation = subEvaluation;
					maxEvaluationCell = libertyCell;
				}
			}

			gameState->PopMatrix();
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