#include "GoGameAI.h"
#include "GoGameState.h"
#include "GoGameMinimax.h"

GoGameAI::GoGameAI()
{
}

/*virtual*/ GoGameAI::~GoGameAI()
{
}

GoGameMatrix::CellLocation GoGameAI::CalculateStonePlacement(AGoGameState* gameState)
{
	GoGameMatrix::CellLocation calculatedMove;

	if (gameState)
	{
		GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
		if (gameMatrix)
		{
			if (gameMatrix->GetMatrixSize() <= 8)
			{
				GoGameMinimax gameMinimax(5, gameMatrix->GetWhoseTurn());

				for (int i = 0; i < gameMatrix->GetMatrixSize(); i++)
				{
					for (int j = 0; j < gameMatrix->GetMatrixSize(); j++)
					{
						GoGameMatrix::CellLocation cellLocation(i, j);
						EGoGameCellState cellState;
						gameMatrix->GetCellState(cellLocation, cellState);
						if (cellState == EGoGameCellState::Empty)
							gameMinimax.branchingCellSet.Add(cellLocation);
					}
				}

				GoGameMatrix::CellLocation bestNextMove;
				if (gameMinimax.CalculateBestNextMove(gameState, bestNextMove))
					calculatedMove = bestNextMove;
			}
			else
			{
				// TODO: Here we need to figure where best to focus the minimax.
				//       We might even run minimax in two or more spots.
			}
		}
	}

	return calculatedMove;
}