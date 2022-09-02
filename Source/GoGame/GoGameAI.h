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

	static void FindAllImmortalGroupsOfColor(EGoGameCellState color, AGoGameState* gameState, TArray<GoGameMatrix::ConnectedRegion*>& immortalGroupArray);

	void Clear();

	TArray<GoGameMatrix::ConnectedRegion*> favoredGroupArray;
	TArray<GoGameMatrix::ConnectedRegion*> opposingGroupArray;
	TArray<GoGameMatrix::ConnectedRegion*> favoredGroupInAtariArray;
	TArray<GoGameMatrix::ConnectedRegion*> opposingImmortalGroupArray;
};