#include "GoGameAI.h"
#include "GoGameState.h"
#include "GoGameGroupMinimax.h"

GoGameAI::GoGameAI()
{
}

/*virtual*/ GoGameAI::~GoGameAI()
{
	this->Clear();
}

void GoGameAI::Clear()
{
	for (GoGameMatrix::ConnectedRegion* favoredGroup : this->favoredGroupArray)
		delete favoredGroup;

	for (GoGameMatrix::ConnectedRegion* opposingGroup : this->opposingGroupArray)
		delete opposingGroup;

	for (GoGameMatrix::ConnectedRegion* opposingImmortalGroup : this->opposingImmortalGroupArray)
		delete opposingImmortalGroup;

	this->favoredGroupArray.Reset();
	this->opposingGroupArray.Reset();
	this->favoredGroupInAtariArray.Reset();
	this->opposingImmortalGroupArray.Reset();
}

// Supposedly, we could try to carry over memory from turn to turn, but for now,
// each turn is taken by the computer as a complete analysis from scratch.  For example,
// we might keep track of which regions of the board are likely just lost to the opponent
// so that we don't waste moves placing there.  This isn't so easy to figure out, though,
// because domi space might be lost to the opponent and captured space might be big enough
// to contend for (i.e., make eye-spaces in).  It's an interesting problem.
GoGameMatrix::CellLocation GoGameAI::CalculateStonePlacement(AGoGameState* gameState)
{
	// TODO: A few thoughts.  First, if we can capture, we probably should.  A good go player
	//       will actually not waste a move capturing if they know they can put it off, because
	//       they can allocate their move to something else in the meantime.  But for this,
	//       it's probably best to capture if we can.  Second, the AI is really dumb about
	//       playing inside the opponent's territory.  This isn't always a bad thing if you
	//       know you can make eye-space, but if there is not enough room, then it is a fool's
	//       errand.  All of this knowledge is very hard to convey in code, so some kind of
	//       heuristic needs to be figured out.  Lastly, maybe I can push minimax to the limit
	//       with subregions of the board.  In any case, the evaluation function isn't very
	//       good, and needs to factor in a lot more information about the board.  Oh, and
	//       sometimes the AI will place a stone directly in atari.  Doh!

	this->Clear();

	float lookAheadDepth = 4;
	GoGameMatrix::CellLocation calculatedMove;
	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
	EGoGameCellState favoredPlayer = gameMatrix->GetWhoseTurn();
	EGoGameCellState opposingPlayer = (favoredPlayer == EGoGameCellState::Black) ? EGoGameCellState::White : EGoGameCellState::Black;

	// Collect all groups of the favored player, sorted by liberty count.
	gameMatrix->CollectAllRegionsOfType(favoredPlayer, this->favoredGroupArray);
	this->favoredGroupArray.Sort([](const GoGameMatrix::ConnectedRegion& groupA, const GoGameMatrix::ConnectedRegion& groupB) -> bool {
		return groupA.libertiesSet.Num() < groupB.libertiesSet.Num();
	});

	// Our first priority is to prevent any friendly stones from being captured.
	// Go find all our friendly groups in atari.
	for (int i = 0; i < this->favoredGroupArray.Num(); i++)
	{
		GoGameMatrix::ConnectedRegion* favoredGroup = this->favoredGroupArray[i];
		if (favoredGroup->libertiesSet.Num() == 1)
			this->favoredGroupInAtariArray.Add(favoredGroup);
		else
			break;	// Early out since favored group array is sorted by liberty count.
	}

	// Sort the atari groups by size in descending order.
	this->favoredGroupInAtariArray.Sort([](const GoGameMatrix::ConnectedRegion& groupA, const GoGameMatrix::ConnectedRegion& groupB) -> bool {
		return groupA.membersSet.Num() > groupB.membersSet.Num();
	});

	// Try each group in atari until we find a saving move.  Note that some groups are
	// worth sacrificing, but I don't know how to program that kind of logic.  A good player
	// can recognize a lost cause and there-by save any wasted moves and stones.
	// Also, here there isn't anything to prevent us from building a ladder.  Some ladders
	// are a fool's errand while others are okay if it will run into a friendly stone.
	// TODO: This AI is definitely *stupid* about ladders.  How do we fix this?
	//       Note that sometimes a good way to save a group in atari is to just put an opponent's group in atari.
	//       I could collect all atari saving moves into a list.  Then go collect all atari-generating moves into a list.  Then decide from there.
	//       If we can put an opposing player's group in atari where that group is not part of what is putting one of our groups in atari, then that could be a good move.
	//       This is getting messy in the programming.  I think that some hueristic combined with minimax is probably the best way to go.  Of course, we can't
	//       minimax the whole board, but maybe we can keep track of what cells on the board are still up for grabs and focus minimax where we think it would be most useful.
	//       Some sort of percentage per cell indicating the likelyhood that we can capture it for our own territory would be great.  Then find the best place to concentrate minimax.
	for (int i = 0; i < this->favoredGroupInAtariArray.Num(); i++)
	{
		GoGameMatrix::ConnectedRegion* favoredGroupInAtari = this->favoredGroupInAtariArray[i];
		GoGameMatrix::CellLocation targetGroupRep = *favoredGroupInAtari->membersSet.begin();
		GoGameGroupMinimax groupMinimax(lookAheadDepth, favoredPlayer);
		GoGameMatrix::CellLocation bestNextMove;
		if (groupMinimax.CalculateBestNextMove(gameState, targetGroupRep, bestNextMove))
		{
			// Is the found move going to actually save the group?
			GoGameMatrix* forbiddenMatrix = gameState->GetForbiddenMatrix();
			gameState->PushMatrix(new GoGameMatrix(gameState->GetCurrentMatrix()));
			if (gameState->GetCurrentMatrix()->SetCellState(bestNextMove, favoredPlayer, forbiddenMatrix))
			{
				GoGameMatrix::ConnectedRegion* group = gameState->GetCurrentMatrix()->SenseConnectedRegion(targetGroupRep);
				if (group->libertiesSet.Num() > 1)
					calculatedMove = bestNextMove;
				delete group;
			}
			delete gameState->PopMatrix();
			if (gameMatrix->IsInBounds(calculatedMove))
				break;
		}
	}

	// Did we decide on a saving move?
	if (!gameMatrix->IsInBounds(calculatedMove))
	{
		// No.  So let's go on the offensive.  We really should be trying to get
		// territory, but that's another bit of logic that isn't easy to code.
		// Collect all groups of the opposing player, sorted by liberty count.
		gameMatrix->CollectAllRegionsOfType(opposingPlayer, this->opposingGroupArray);
		this->opposingGroupArray.Sort([](const GoGameMatrix::ConnectedRegion& groupA, const GoGameMatrix::ConnectedRegion& groupB) -> bool {
			return groupA.libertiesSet.Num() < groupB.libertiesSet.Num();
		});

		// Go find all the stones of the opposing player that cannot be captured.
		TSet<GoGameMatrix::CellLocation> opposingImmortalStonesSet;
		this->FindAllImmortalGroupsOfColor(opposingPlayer, gameState, this->opposingImmortalGroupArray);
		for (int i = 0; i < opposingImmortalGroupArray.Num(); i++)
		{
			GoGameMatrix::ConnectedRegion* opposingImmortalGroup = opposingImmortalGroupArray[i];
			for (GoGameMatrix::CellLocation cellLocation : opposingImmortalGroup->membersSet)
				opposingImmortalStonesSet.Add(cellLocation);
		}

		// Consider attacking each of the opposing groups, most vulnerable to least.  Skip the immortal groups.
		for (int i = 0; i < this->opposingGroupArray.Num(); i++)
		{
			GoGameMatrix::ConnectedRegion* opposingGroup = opposingGroupArray[i];
			GoGameMatrix::CellLocation targetGroupRep = *opposingGroup->membersSet.begin();
			if (!opposingImmortalStonesSet.Contains(targetGroupRep))
			{
				GoGameGroupMinimax groupMinimax(lookAheadDepth, favoredPlayer);
				GoGameMatrix::CellLocation bestNextMove;
				if (groupMinimax.CalculateBestNextMove(gameState, targetGroupRep, bestNextMove))
				{
					calculatedMove = bestNextMove;
					if (gameMatrix->IsInBounds(calculatedMove))
						break;
				}
			}
		}

		// Did we decide on an attacking move?
		if (!gameMatrix->IsInBounds(calculatedMove))
		{
			// No.  Okay, well then, place a stone at random?  I don't know.  This is dumb.
			//...
		}
	}

	return calculatedMove;
}

/*static*/ void GoGameAI::FindAllImmortalGroupsOfColor(EGoGameCellState color, AGoGameState* gameState, TArray<GoGameMatrix::ConnectedRegion*>& immortalGroupArray)
{
	gameState->PushMatrix(new GoGameMatrix(gameState->GetCurrentMatrix()));
	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
	EGoGameCellState otherColor = (color == EGoGameCellState::Black) ? EGoGameCellState::White : EGoGameCellState::Black;
	bool altered = false;
	
	// Let the other color stones be placed as if the given color player passes indefinitely until no more stones can be placed.
	// Once we're done, all remaining groups of the given color should be those that are immortal.
	do
	{
		altered = false;
		for (int i = 0; i < gameMatrix->GetMatrixSize() && !altered; i++)
		{
			for (int j = 0; j < gameMatrix->GetMatrixSize() && !altered; j++)
			{
				GoGameMatrix::CellLocation cellLocation(i, j);
				EGoGameCellState cellState;
				gameMatrix->GetCellState(cellLocation, cellState);
				if (cellState == EGoGameCellState::Empty)
					altered = gameMatrix->SetCellState(cellLocation, otherColor, nullptr, true);
			}
		}
	} while (altered);

	gameMatrix->CollectAllRegionsOfType(color, immortalGroupArray);
	delete gameState->PopMatrix();
}