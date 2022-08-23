#include "GoGameMatrix.h"

GoGameMatrix::GoGameMatrix()
{
	this->squareMatrixSize = 0;
	this->squareMatrix = nullptr;
	this->whiteCaptureCount = 0;
	this->blackCaptureCount = 0;
	this->whoseTurn = EGoGameCellState::Black;
}

GoGameMatrix::GoGameMatrix(const GoGameMatrix* gameMatrix)
{
	this->squareMatrixSize = 0;
	this->squareMatrix = nullptr;

	this->SetMatrixSize(gameMatrix->squareMatrixSize);

	for (int i = 0; i < this->squareMatrixSize; i++)
		for (int j = 0; j < this->squareMatrixSize; j++)
			this->squareMatrix[i][j] = gameMatrix->squareMatrix[i][j];

	this->whiteCaptureCount = gameMatrix->whiteCaptureCount;
	this->blackCaptureCount = gameMatrix->blackCaptureCount;

	this->whoseTurn = gameMatrix->whoseTurn;
}

/*virtual*/ GoGameMatrix::~GoGameMatrix()
{
	this->FreeMatrix();
}

bool GoGameMatrix::SetMatrixSize(int givenSize)
{
	if (givenSize < 2)
		return false;

	this->FreeMatrix();

	this->squareMatrixSize = givenSize;
	this->squareMatrix = new EGoGameCellState * [this->squareMatrixSize];
	for (int i = 0; i < this->squareMatrixSize; i++)
	{
		this->squareMatrix[i] = new EGoGameCellState[this->squareMatrixSize];
		for (int j = 0; j < this->squareMatrixSize; j++)
			this->squareMatrix[i][j] = EGoGameCellState::Empty;
	}

	return true;
}

int GoGameMatrix::GetMatrixSize()
{
	return this->squareMatrixSize;
}

void GoGameMatrix::FreeMatrix()
{
	if (this->squareMatrix)
	{
		for (int i = 0; i < this->squareMatrixSize; i++)
			delete[] this->squareMatrix[i];

		delete[] this->squareMatrix;
		this->squareMatrix = nullptr;
		this->squareMatrixSize = 0;
	}
}

bool GoGameMatrix::IsInBounds(const CellLocation& cellLocation) const
{
	if (cellLocation.i < 0 || cellLocation.i >= this->squareMatrixSize)
		return false;

	if (cellLocation.j < 0 || cellLocation.j >= this->squareMatrixSize)
		return false;

	return false;
}

// Here we enforce the rules of the game.  Note that there is one rule of Go that will need special handling here.
// It is the rule where we are not allowed to immediately return to a previous state of the board.  To enforce this,
// we may need to provide the matrix stack here as additional information.
bool GoGameMatrix::SetCellState(const CellLocation& cellLocation, EGoGameCellState cellState)
{
	if (this->whoseTurn != cellState)
		return false;

	if (!this->IsInBounds(cellLocation))
		return false;

	if (this->squareMatrix[cellLocation.i][cellLocation.j] != EGoGameCellState::Empty)
		return false;

	// Temtatively place the stone on the board.
	this->squareMatrix[cellLocation.i][cellLocation.j] = cellState;

	// Remove any groups that may have been killed by the placement of the stone.
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			CellLocation adjacentLocation;
			adjacentLocation.i = cellLocation.i + 2 * i - 1;
			adjacentLocation.j = cellLocation.j + 2 * j - 1;

			if (this->IsInBounds(adjacentLocation))
			{
				EGoGameCellState adjacentState = this->squareMatrix[adjacentLocation.i][adjacentLocation.j];
				if (adjacentState != EGoGameCellState::Empty && adjacentState != cellState)
				{
					ConnectedRegion* adjacentGroup = this->SenseConnectedRegion(adjacentLocation);
					check(adjacentGroup->type == ConnectedRegion::GROUP);
					if (adjacentGroup->libertiesSet.Num() == 0)
					{
						if (adjacentGroup->owner == EGoGameCellState::Black)
							whiteCaptureCount += adjacentGroup->membersSet.Num();
						else if (adjacentGroup->owner == EGoGameCellState::White)
							blackCaptureCount += adjacentGroup->membersSet.Num();

						for (CellLocation killedStoneLocation : adjacentGroup->membersSet)
							this->squareMatrix[killedStoneLocation.i][killedStoneLocation.j] = EGoGameCellState::Empty;
					}
				}
			}
		}
	}

	// After trying to remove any captured stones, if the placed stone has no liberties, then the move was invalid.
	ConnectedRegion* group = this->SenseConnectedRegion(cellLocation);
	check(group->type == ConnectedRegion::GROUP);
	if (group->libertiesSet.Num() == 0)
	{
		this->squareMatrix[cellLocation.i][cellLocation.j] = EGoGameCellState::Empty;
		return false;
	}

	// It is now the other player's turn.
	if (this->whoseTurn == EGoGameCellState::Black)
		this->whoseTurn = EGoGameCellState::White;
	else
		this->whoseTurn = EGoGameCellState::Black;

	return true;
}

bool GoGameMatrix::GetCellState(const CellLocation& cellLocation, EGoGameCellState& cellState) const
{
	if (!this->IsInBounds(cellLocation))
		return false;

	cellState = this->squareMatrix[cellLocation.i][cellLocation.j];
	return true;
}

GoGameMatrix::ConnectedRegion* GoGameMatrix::SenseConnectedRegion(const CellLocation& cellLocation) const
{
	if (!this->IsInBounds(cellLocation))
		return nullptr;

	EGoGameCellState matchState = this->squareMatrix[cellLocation.i][cellLocation.j];

	ConnectedRegion* region = new ConnectedRegion();
	region->owner = matchState;
	region->type = (matchState == EGoGameCellState::Empty) ? ConnectedRegion::TERRITORY : ConnectedRegion::GROUP;

	TSet<CellLocation> cellLocationQueue;
	cellLocationQueue.Add(cellLocation);

	int blackBoundaryCount = 0;
	int whiteBondaryCount = 0;

	while (cellLocationQueue.Num() > 0)
	{
		TSet<CellLocation>::TIterator iter = cellLocationQueue.CreateIterator();
		CellLocation nextCellLocation = *iter;
		cellLocationQueue.Remove(*iter);

		region->membersSet.Add(nextCellLocation);

		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				CellLocation adjacentLocation;
				adjacentLocation.i = nextCellLocation.i + 2 * i - 1;
				adjacentLocation.j = nextCellLocation.j + 2 * j - 1;

				if (this->IsInBounds(adjacentLocation))
				{
					if (region->type == ConnectedRegion::TERRITORY)
					{
						if (this->squareMatrix[adjacentLocation.i][adjacentLocation.j] == EGoGameCellState::Black)
							blackBoundaryCount++;
						else if (this->squareMatrix[adjacentLocation.i][adjacentLocation.j] == EGoGameCellState::White)
							whiteBondaryCount++;
					}
					else if (region->type == ConnectedRegion::GROUP)
					{
						if (this->squareMatrix[adjacentLocation.i][adjacentLocation.j] == EGoGameCellState::Empty)
						{
							if (!region->libertiesSet.Contains(adjacentLocation))
								region->libertiesSet.Add(adjacentLocation);
						}
					}

					if (this->squareMatrix[adjacentLocation.i][adjacentLocation.j] == matchState)
					{
						if (!cellLocationQueue.Contains(adjacentLocation) && !region->membersSet.Contains(adjacentLocation))
							cellLocationQueue.Add(adjacentLocation);
					}
				}
			}
		}
	}

	if (region->type == ConnectedRegion::TERRITORY)
	{
		if (blackBoundaryCount > 0 && whiteBondaryCount == 0)
			region->owner = EGoGameCellState::Black;
		else if (whiteBondaryCount > 0 && blackBoundaryCount == 0)
			region->owner = EGoGameCellState::White;
	}

	return region;
}

GoGameMatrix::ConnectedRegion::ConnectedRegion()
{
}

/*virtual*/ GoGameMatrix::ConnectedRegion::~ConnectedRegion()
{
}