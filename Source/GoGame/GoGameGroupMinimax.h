#pragma once

#include "GoGameMatrix.h"

class AGoGameState;

// This class implements just one piece in what may be a larger AI system for a computer-backed go-player.
// The idea here is to focus on just a single group and determine what may be the best next stone placement
// for defending or irradicating that group.  We do that here using a variation of the mini-max algorithm.
// Of course, mini-max can't practically be applied to the whole of the game of go, because the branch-factor
// is way too big.  We may, however, be able to apply mini-max in the game of go by limiting its scope to
// just the liberties of a given group.
class GoGameGroupMinimax
{
public:
	GoGameGroupMinimax(int lookAheadDepth, EGoGameCellState favoredPlayer);
	virtual ~GoGameGroupMinimax();

	bool CalculateBestNextMove(AGoGameState* gameState, const GoGameMatrix::CellLocation& targetGroupRep, GoGameMatrix::CellLocation& bestNextMove);

private:
	
	void Minimax(AGoGameState* gameState, const GoGameMatrix::CellLocation& targetGroupRep, int currentDepth, float& evaluation, GoGameMatrix::CellLocation& moveAssociatedWithEvaluation);

	void ExpandEmptyCellSet(GoGameMatrix* gameMatrix, TSet<GoGameMatrix::CellLocation>& emptyCellSet, int iterations);

	int lookAheadDepth;
	int totalEvaluations;
	EGoGameCellState favoredPlayer, opposingPlayer;
	EGoGameCellState groupColor;
};