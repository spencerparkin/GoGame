#pragma once

#include "GoGameMatrix.h"

class AGoGameState;

// I'm not under any delusion that I can make a good Go-game AI, but I can try to do something just for run.
class GoGameAI
{
public:
	GoGameAI();
	virtual ~GoGameAI();

	GoGameMatrix::CellLocation CalculateStonePlacement(AGoGameState* gameState);
};