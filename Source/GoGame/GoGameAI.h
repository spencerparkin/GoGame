#pragma once

#include "GoGameMatrix.h"

class AGoGameState;

// I'm not under any delusion that I can make a good Go-game AI, but I can try to do something just for run.
// TODO: I'm going to wipe this out and take a very different approach.  I need to develop a highly accurate
//       heat map of the board.  Cold spots are where it's not worth playing, and hot spots are where it is
//       worth playing.  Minimax should then be concentrated in as large an area as possible (while still having
//       a practical running time) over the hottest area of the board.  This, combined with a really good
//       evaluation function, and a search depth as far as we can practically go, might make for a reasonably
//       good Go player.  For me, reasonably good is just enough to give me trouble, which is a low bar, but
//       might be achievable.
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