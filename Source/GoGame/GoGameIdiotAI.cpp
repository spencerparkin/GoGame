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
// to compute.  The hardest thing, I think, to program in the computer
// is the notion of trying to make eye-space.

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

			// Look for a move that puts one or more opponent groups in atari.  Pick the one, if any, that maximizes the opponent's atari state.
			// It is nice if we can do a double-atari move.  This guarentees us a capture.
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

			// Go find favored groups in atari.  Note that one line of defense is putting an opponent's group in atari, and we already tried that.
			bool savingMoveMade = false;
			TArray<GoGameMatrix::ConnectedRegion*> favoredGroupArray;
			gameMatrix->CollectAllRegionsOfType(this->favoredPlayer, favoredGroupArray);
			favoredGroupArray.Sort([](GoGameMatrix::ConnectedRegion& groupA, GoGameMatrix::ConnectedRegion& groupB) -> bool {
				return groupA.membersSet.Num() > groupB.membersSet.Num();
			});

			// Examine them largest to smallest.  Can we increase the liberties?  Note, we could easily get stuck in a latter here.
			// Latters are okay if they'll run into a friendly stone, but not the edge of the board.
			for (int i = 0; i < favoredGroupArray.Num() && !savingMoveMade; i++)
			{
				GoGameMatrix::ConnectedRegion* favoredGroup = favoredGroupArray[i];
				if (favoredGroup->libertiesSet.Num() == 1)
				{
					GoGameMatrix* trialMatrix = new GoGameMatrix(gameMatrix);
					if (trialMatrix->SetCellState(*favoredGroup->libertiesSet.begin(), this->favoredPlayer, nullptr))
					{
						GoGameMatrix::ConnectedRegion* favoredGroupAttemptedSave = trialMatrix->SenseConnectedRegion(*favoredGroup->membersSet.begin());
						if (favoredGroupAttemptedSave->libertiesSet.Num() > 1)
						{
							stonePlacement = *favoredGroup->libertiesSet.begin();
							savingMoveMade = true;
						}
					}
					delete trialMatrix;
				}
			}

			for (GoGameMatrix::ConnectedRegion* favoredGroup : favoredGroupArray)
				delete favoredGroup;

			if (savingMoveMade)
				return true;

			// Look for a move the opponent can make that puts one or more favored groups in atari.
			// If one is found, does it help us to make that move instead?
			int favoredGroupsInAtari = gameMatrix->CountGroupsInAtariForColor(this->favoredPlayer);
			for (GoGameMatrix::CellLocation cellLocation : validMovesSet)
			{
				GoGameMatrix* trialMatrix = new GoGameMatrix(gameMatrix);
				trialMatrix->SetCellState(cellLocation, opponentPlayer, nullptr, true);
				int atariIncrease = trialMatrix->CountGroupsInAtariForColor(this->favoredPlayer) - favoredGroupsInAtari;
				delete trialMatrix;
				if (atariIncrease > 0)
				{
					trialMatrix = new GoGameMatrix(gameMatrix);
					if (trialMatrix->SetCellState(cellLocation, this->favoredPlayer, forbiddenMatrix))
					{
						GoGameMatrix::ConnectedRegion* group = trialMatrix->SenseConnectedRegion(cellLocation);
						if (group->libertiesSet.Num() >= 3)
						{
							stonePlacement = cellLocation;
							savingMoveMade = true;
						}
						delete group;
					}
					delete trialMatrix;
				}
				if (savingMoveMade)
					break;
			}

			if (savingMoveMade)
				return true;

			TSet<GoGameMatrix::CellLocation> favoredTerritorySet;
			gameMatrix->FindAllTerritoryOfColor(this->favoredPlayer, favoredTerritorySet);

			TSet<GoGameMatrix::CellLocation> opponentTerritorySet;
			gameMatrix->FindAllTerritoryOfColor(opponentPlayer, opponentTerritorySet);

			TArray<GoGameMatrix::CellLocation> nonTerritorialMovesArray;
			for (GoGameMatrix::CellLocation cellLocation : validMovesSet)
				if (!favoredTerritorySet.Contains(cellLocation) && !opponentTerritorySet.Contains(cellLocation))
					nonTerritorialMovesArray.Add(cellLocation);

			if (nonTerritorialMovesArray.Num() == 0)
				return false;

			int i = FMath::RandRange(0, nonTerritorialMovesArray.Num() - 1);
			stonePlacement = nonTerritorialMovesArray[i];

			this->phaseTickCount++;
			break;
		}
	}

	return true;
}