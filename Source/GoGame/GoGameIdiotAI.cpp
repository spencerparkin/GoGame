#include "GoGameIdiotAI.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "Kismet/GameplayStatics.h"

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
				float score = taxicabDistance * distanceToEdge;
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

			// TODO: If there is a move we can make to kill an opponent's group, do it.  That's a no-brainer.

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
							if (validMovesSet.Contains(stonePlacement))
								foundKittyCornerDanger = true;
						}
						else if (cellState[0][1] == opponentPlayer)
						{
							stonePlacement.i = i + 1;
							stonePlacement.j = j;
							if (validMovesSet.Contains(stonePlacement))
								foundKittyCornerDanger = true;
						}
					}
					else if (cellState[1][0] == this->favoredPlayer && cellState[0][1] == this->favoredPlayer)
					{
						if (cellState[0][0] == opponentPlayer)
						{
							stonePlacement.i = i + 1;
							stonePlacement.j = j + 1;
							if (validMovesSet.Contains(stonePlacement))
								foundKittyCornerDanger = true;
						}
						else if (cellState[1][1] == opponentPlayer)
						{
							stonePlacement.i = i;
							stonePlacement.j = j;
							if (validMovesSet.Contains(stonePlacement))
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

			GoGameMatrix::ConnectedRegion* mostImportantGroup = nullptr;
			float maxGroupImportance = 0.0f;
			for (int i = 0; i < groupArray.Num(); i++)
			{
				GoGameMatrix::ConnectedRegion* group = groupArray[i];
				float groupImportance = float(group->membersSet.Num()) / float(group->libertiesSet.Num());
				if (groupImportance > maxGroupImportance)
				{
					maxGroupImportance = groupImportance;
					mostImportantGroup = group;
				}
			}

			if (!mostImportantGroup)
				return false;

			bool foundBest = this->ScoreAndSelectBestPlacement(gameState, [&mostImportantGroup](GoGameMatrix* givenGameMatrix, const GoGameMatrix::CellLocation& cellLocation) -> float {
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
						else if (mostImportantGroup->membersSet.Contains(adjLocation))
							connections += 1.0f;
					}
				}
				
				float kittyConnections = 0.0f;
				for (int i = 0; i < 4; i++)
				{
					GoGameMatrix::CellLocation kittyLocation = cellLocation.GetKittyCornerLocation(i);
					if (givenGameMatrix->IsInBounds(kittyLocation))
					{
						if (mostImportantGroup->membersSet.Contains(kittyLocation))
							kittyConnections += 1.0f;
					}
				}
				
				float score = 0.0f;
				if (connections == 0.0f && kittyConnections > 0.0f && liberties == 4.0f)
					score = liberties * kittyConnections * 2.0f;	// Favor kitty connections over actual connections.
				else if (connections > 0.0f && liberties > 0.0f)
					score = liberties * connections;

				// TODO: Make the move on a temp matrix.  Does it reduce our overall territory count?  If so, score gits negative hit.
				//       Does the important group merge with another group to get even bigger?  If so, big bump to score.

				return score;
			}, stonePlacement);

			if (!foundBest)
				return false;

			this->phaseTickCount++;
			break;
		}
	}

	return true;
}