#pragma once

#include "GoGameMatrix.h"

class AGoGameState;

// TODO: Derive from base AI class and override asynchronous interface so that
//       the user can see a bit of time pass before the AI places a stone.
class GoGameIdiotAI
{
public:
	GoGameIdiotAI();
	virtual ~GoGameIdiotAI();

	bool CalculateStonePlacement(AGoGameState* gameState, GoGameMatrix::CellLocation& stonePlacement);

	EGoGameCellState favoredPlayer;

	struct Candidate
	{
		GoGameMatrix::CellLocation cellLocation;
		float score;
	};

	bool ScoreAndSelectBestPlacement(AGoGameState* gameState, TFunctionRef<float(GoGameMatrix* gameMatrix, const GoGameMatrix::CellLocation& cellLocation)> scoreFunction, GoGameMatrix::CellLocation& stonePlacement);

	int phaseNumber;
	int phaseTickCount;
};