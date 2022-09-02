#pragma once

#include "GoGameMatrix.h"

class AGoGameState;

class GoGameMinimax
{
public:
	GoGameMinimax(int lookAheadDepth, EGoGameCellState favoredPlayer);
	virtual ~GoGameMinimax();

	bool CalculateBestNextMove(AGoGameState* gameState, GoGameMatrix::CellLocation& bestNextMove);

	// These are the locations on the board where we will consider placing stones as we
	// traverse the game tree.  For small Go boards, we could populate this set with all
	// currently empty cells.  For larger Go boards, doing so would be impractical, as the
	// running time of our algorithm would become enormous.  Alternatively in such cases,
	// we could do an analysis of the board to determine where to focus minimax.  For example,
	// we could identify opponent territory as being of too small an area to play in to make
	// eye-spaces, so we could rule out those portions of the board.  We might also try to
	// favor portions of the board where it seems that more is at stake in terms of captures
	// and territory.
	TSet<GoGameMatrix::CellLocation> branchingCellSet;

private:
	
	void Minimax(AGoGameState* gameState, int currentDepth, int& evaluation, GoGameMatrix::CellLocation& moveAssociatedWithEvaluation);

	void ExpandEmptyCellSet(GoGameMatrix* gameMatrix, TSet<GoGameMatrix::CellLocation>& emptyCellSet, int iterations);

	int lookAheadDepth;
	int totalEvaluations;
	EGoGameCellState favoredPlayer;

	class BoardStatus
	{
	public:
		BoardStatus();
		virtual ~BoardStatus();

		void AnalyzeBoard(GoGameMatrix* gameMatrix);
		int EvaluateAgainst(const BoardStatus& futureStatus, EGoGameCellState favoredPlayer) const;

		int blackTerritory;
		int whiteTerritory;
		int blackCaptures;
		int whiteCaptures;
		int numBlackImmortalGroups;
		int numWhiteImmortalGroups;
		int numBlackGroupsInAtari;
		int numWhiteGroupsInAtari;
		int totalBlackLiberties;
		int totalWhiteLiberties;
	};

	BoardStatus baseStatus;
};