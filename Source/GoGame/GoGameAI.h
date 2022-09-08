#pragma once

#include "CoreMinimal.h"
#include "GoGameMatrix.h"

class AGoGameState;

class GoGameAI
{
public:
	GoGameAI(EGoGameCellState favoredColor);
	virtual ~GoGameAI();

	virtual void BeginThinking();
	virtual void StopThinking();
	virtual bool TickThinking();

	AGoGameState* gameState;
	GoGameMatrix::CellLocation stonePlacement;
	EGoGameCellState favoredPlayer;
};