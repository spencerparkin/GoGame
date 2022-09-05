#pragma once

#include "GoGameMatrix.h"

class AGoGameState;

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