#include "GoGameIdiotAI.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "Kismet/GameplayStatics.h"

// This is so idiotic, I'm ashamed I programmed any of it.
// It is just too hard to program all of your intuition and experience
// in the game of Go.  I'm not a very good Go player at all, but I
// know a few things, and a few tricks, and I can crush the following
// logic every time.  Go, in my opinion, is played on both a micro and
// macro scale, and if you're not thinking about both as you play,
// you won't do very well.  You have to think ahead about shape,
// and anticipate what kind of shapes your opponent is trying to make.
// Also, sometimes the best way to save a group is to threaten your
// opponent in an entirely different part of the board.  I can also
// see moves that will box an opponent in and make it impossible for
// their groups to survive, or now the best liberty to remove to
// ensure an opponent's group's destruction in some cases, and then
// won't bother to kill it, because I know that's a wasted move, and
// so on.  I never thought I could create an AI that would always win,
// but I thought maybe I could make one that gave me a hard time winning.
// The computer has some obvious advantages; namely, speed and memory.
// I don't have those, and yet, I crush this stupid logic easily.
// There is so much of go that is experience and intuition.  I have
// yet to figure out some sort of computer algorithm or stratagy
// that does reasonably well.  Again, I don't care if the AI still
// loses occationally, but right now, everything I've tried always
// loses by a land-slide.  I think mini-max has given me the best
// results, but it can still be very dumb, and it takes way too long
// to compute.

GoGameIdiotAI::GoGameIdiotAI()
{
	this->favoredPlayer = EGoGameCellState::Empty;
	this->phaseNumber = 0;
	this->phaseTickCount = 0;
}

/*virtual*/ GoGameIdiotAI::~GoGameIdiotAI()
{
}

bool GoGameIdiotAI::ScoreAndSelectBestPlacement(AGoGameState* gameState, TFunctionRef<float(GoGameMatrix* gameMatrix, const GoGameMatrix::CellLocation& cellLocation)> scoreFunction, GoGameMatrix::CellLocation& stonePlacement)
{
	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
	GoGameMatrix* forbiddenMatrix = gameState->GetForbiddenMatrix();
	TSet<GoGameMatrix::CellLocation> cellLocationSet;
	gameMatrix->GenerateAllPossiblePlacements(cellLocationSet, forbiddenMatrix);

	TArray<Candidate> candidateArray;
	for (GoGameMatrix::CellLocation cellLocation : cellLocationSet)
	{
		Candidate candidate;
		candidate.cellLocation = cellLocation;
		candidate.score = scoreFunction(gameMatrix, cellLocation);
		candidateArray.Add(candidate);
	}

	if (candidateArray.Num() == 0)
		return false;

	candidateArray.Sort([](const Candidate& candidateA, const Candidate& candidateB) -> bool {
		return candidateA.score > candidateB.score;
	});

	if (candidateArray[0].score <= 0.0f)
		return false;

	stonePlacement = candidateArray[0].cellLocation;
	return true;
}

bool GoGameIdiotAI::CalculateStonePlacement(AGoGameState* gameState, GoGameMatrix::CellLocation& stonePlacement)
{
	if (this->favoredPlayer == EGoGameCellState::Empty)
		return false;

	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
	if (!gameMatrix)
		return false;

	if (!gameMatrix || gameMatrix->GetWhoseTurn() != this->favoredPlayer)
		return false;

	EGoGameCellState opponentPlayer = (this->favoredPlayer == EGoGameCellState::Black) ? EGoGameCellState::White : EGoGameCellState::Black;

	switch (this->phaseNumber)
	{
		case 0:
		{
			bool foundBest = this->ScoreAndSelectBestPlacement(gameState, [](GoGameMatrix* givenGameMatrix, const GoGameMatrix::CellLocation& cellLocation) -> float {
				float taxicabDistance = givenGameMatrix->TaxicabDistanceToNearestOccupiedCell(cellLocation);
				float distanceToEdge = givenGameMatrix->ShortestDistanceToBoardEdge(cellLocation);
				float distanceToCenter = givenGameMatrix->ShortestDistanceToBoardCenter(cellLocation);
				float score = taxicabDistance * distanceToEdge * distanceToCenter;
				return score;
			}, stonePlacement);

			if (!foundBest)
				return false;

			if (++this->phaseTickCount >= 5)
			{
				this->phaseNumber++;
				this->phaseTickCount = 0;
			}

			break;
		}
		case 1:
		{
			GoGameMatrix* forbiddenMatrix = gameState->GetForbiddenMatrix();
			TSet<GoGameMatrix::CellLocation> validMovesSet;
			gameMatrix->GenerateAllPossiblePlacements(validMovesSet, forbiddenMatrix);

			// Go find opponent groups in atari.
			bool captureMoveMade = false;
			TArray<GoGameMatrix::ConnectedRegion*> opponentGroupArray;
			gameMatrix->CollectAllRegionsOfType(opponentPlayer, opponentGroupArray);
			opponentGroupArray.Sort([](GoGameMatrix::ConnectedRegion& groupA, GoGameMatrix::ConnectedRegion& groupB) -> bool {
				return groupA.membersSet.Num() > groupB.membersSet.Num();
			});

			// Examine them largest to smallest.  Capture them now if necessary.
			for (int i = 0; i < opponentGroupArray.Num() && !captureMoveMade; i++)
			{
				GoGameMatrix::ConnectedRegion* opponentGroup = opponentGroupArray[i];
				if(opponentGroup->libertiesSet.Num() == 1)
				{
					bool needToCaptureNow = true;
					GoGameMatrix* trialMatrix = new GoGameMatrix(gameMatrix);
					trialMatrix->SetCellState(*opponentGroup->libertiesSet.begin(), opponentPlayer, nullptr, true);
					GoGameMatrix::ConnectedRegion* opponentGroupAttemptedSave = trialMatrix->SenseConnectedRegion(*opponentGroup->membersSet.begin());
					if (opponentGroupAttemptedSave->libertiesSet.Num() == 1)
						needToCaptureNow = false;
					delete opponentGroupAttemptedSave;
					delete trialMatrix;
					if (needToCaptureNow)
					{
						stonePlacement = *opponentGroup->libertiesSet.begin();
						if (validMovesSet.Contains(stonePlacement))
							captureMoveMade = true;
					}
				}
			}

			for (GoGameMatrix::ConnectedRegion* opponentGroup : opponentGroupArray)
				delete opponentGroup;

			if (captureMoveMade)
				return true;

			// Look for a move that puts one or more opponent groups in atari.  Pick the one, if any, that maximizes opponent's atari state.
			int opponentGroupsInAtari = gameMatrix->CountGroupsInAtariForColor(opponentPlayer);
			int largestAtariIncrease = 0;
			for (GoGameMatrix::CellLocation cellLocation : validMovesSet)
			{
				GoGameMatrix* trialMatrix = new GoGameMatrix(gameMatrix);
				trialMatrix->SetCellState(cellLocation, this->favoredPlayer, forbiddenMatrix);
				int atariIncrease = trialMatrix->CountGroupsInAtariForColor(opponentPlayer) - opponentGroupsInAtari;
				if (atariIncrease > largestAtariIncrease)
				{
					largestAtariIncrease = atariIncrease;
					stonePlacement = cellLocation;
				}
				delete trialMatrix;
			}

			if (largestAtariIncrease > 0)
				return true;

			TSet<GoGameMatrix::CellLocation> favoredTerritorySet;
			gameMatrix->FindAllTerritoryOfColor(this->favoredPlayer, favoredTerritorySet);

			// Don't let any kitty-corner "connection" become unconnected.
			bool foundKittyCornerDanger = false;
			for (int i = 0; i < gameMatrix->GetMatrixSize() - 1 && !foundKittyCornerDanger; i++)
			{
				for (int j = 0; j < gameMatrix->GetMatrixSize() - 1 && !foundKittyCornerDanger; j++)
				{
					EGoGameCellState cellState[2][2];
					gameMatrix->GetCellState(GoGameMatrix::CellLocation(i, j), cellState[0][0]);
					gameMatrix->GetCellState(GoGameMatrix::CellLocation(i + 1, j), cellState[1][0]);
					gameMatrix->GetCellState(GoGameMatrix::CellLocation(i, j + 1), cellState[0][1]);
					gameMatrix->GetCellState(GoGameMatrix::CellLocation(i + 1, j + 1), cellState[1][1]);

					if (cellState[0][0] == this->favoredPlayer && cellState[1][1] == this->favoredPlayer)
					{
						if (cellState[1][0] == opponentPlayer)
						{
							stonePlacement.i = i;
							stonePlacement.j = j + 1;
							if (validMovesSet.Contains(stonePlacement) && !favoredTerritorySet.Contains(stonePlacement))
								foundKittyCornerDanger = true;
						}
						else if (cellState[0][1] == opponentPlayer)
						{
							stonePlacement.i = i + 1;
							stonePlacement.j = j;
							if (validMovesSet.Contains(stonePlacement) && !favoredTerritorySet.Contains(stonePlacement))
								foundKittyCornerDanger = true;
						}
					}
					else if (cellState[1][0] == this->favoredPlayer && cellState[0][1] == this->favoredPlayer)
					{
						if (cellState[0][0] == opponentPlayer)
						{
							stonePlacement.i = i + 1;
							stonePlacement.j = j + 1;
							if (validMovesSet.Contains(stonePlacement) && !favoredTerritorySet.Contains(stonePlacement))
								foundKittyCornerDanger = true;
						}
						else if (cellState[1][1] == opponentPlayer)
						{
							stonePlacement.i = i;
							stonePlacement.j = j;
							if (validMovesSet.Contains(stonePlacement) && !favoredTerritorySet.Contains(stonePlacement))
								foundKittyCornerDanger = true;
						}
					}
				}
			}

			if (foundKittyCornerDanger)
				return true;

			TArray<GoGameMatrix::ConnectedRegion*> groupArray;
			gameMatrix->CollectAllRegionsOfType(this->favoredPlayer, groupArray);
			if (groupArray.Num() == 0)
				return false;

			GoGameMatrix::ConnectedRegion* selectedGroup = groupArray[FMath::RandRange(0, groupArray.Num() - 1)];

			// Can we merge the selected group with another group?
			GoGameMatrix::CellLocation groupRep = *selectedGroup->membersSet.begin();
			bool foundBest = this->ScoreAndSelectBestPlacement(gameState, [&selectedGroup, &groupRep, this](GoGameMatrix* givenGameMatrix, const GoGameMatrix::CellLocation& cellLocation) -> float {
				float score = 0.0f;
				GoGameMatrix* trialMatrix = new GoGameMatrix(givenGameMatrix);
				bool success = trialMatrix->SetCellState(cellLocation, this->favoredPlayer, nullptr);
				check(success);
				GoGameMatrix::ConnectedRegion* group = trialMatrix->SenseConnectedRegion(groupRep);
				if (group->membersSet.Num() > selectedGroup->membersSet.Num() + 1 && group->membersSet.Num() > 1 && selectedGroup->membersSet.Num() > 1)
					score += group->membersSet.Num() - selectedGroup->membersSet.Num();
				delete trialMatrix;
				return score;
			}, stonePlacement);

			if (!foundBest)
			{
				// Can we grow the selected group?
				foundBest = this->ScoreAndSelectBestPlacement(gameState, [&selectedGroup, &favoredTerritorySet](GoGameMatrix* givenGameMatrix, const GoGameMatrix::CellLocation& cellLocation) -> float {
					
					float liberties = 0.0f;
					float connections = 0.0f;
					for (int i = 0; i < 4; i++)
					{
						GoGameMatrix::CellLocation adjLocation = cellLocation.GetAdjcentLocation(i);
						if (givenGameMatrix->IsInBounds(adjLocation))
						{
							EGoGameCellState cellState;
							givenGameMatrix->GetCellState(adjLocation, cellState);
							if (cellState == EGoGameCellState::Empty)
								liberties += 1.0f;
							else if (selectedGroup->membersSet.Contains(adjLocation))
								connections += 1.0f;
						}
					}

					float kittyConnections = 0.0f;
					for (int i = 0; i < 4; i++)
					{
						GoGameMatrix::CellLocation kittyLocation = cellLocation.GetKittyCornerLocation(i);
						if (givenGameMatrix->IsInBounds(kittyLocation))
						{
							if (selectedGroup->membersSet.Contains(kittyLocation))
								kittyConnections += 1.0f;
						}
					}

					float score = 0.0f;
					if (connections == 0.0f && kittyConnections > 0.0f && liberties == 4.0f)
						score = liberties * kittyConnections * 2.0f;	// Favor kitty connections over actual connections.
					else if (connections > 0.0f && liberties > 0.0f)
						score = liberties * connections;

					if (favoredTerritorySet.Contains(cellLocation))
						score = 0.0f;

					return score;
				}, stonePlacement);

				if (!foundBest)
				{
					this->phaseNumber = 0;
					this->phaseTickCount = 0;
					return this->CalculateStonePlacement(gameState, stonePlacement);
				}
			}

			this->phaseTickCount++;
			break;
		}
	}

	return true;
}