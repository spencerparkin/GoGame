#include "GoGameMatrix.h"

BEGIN_FUNCTION_BUILD_OPTIMIZATION

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

	return true;
}

// Here we enforce the rules of the game.
bool GoGameMatrix::SetCellState(const CellLocation& cellLocation, EGoGameCellState cellState, const GoGameMatrix* forbiddenMatrix, bool ignoreWhoseTurnItIs /*= false*/)
{
	if (this->whoseTurn != cellState && !ignoreWhoseTurnItIs)
		return false;

	if (!this->IsInBounds(cellLocation))
		return false;

	if (this->squareMatrix[cellLocation.i][cellLocation.j] != EGoGameCellState::Empty)
		return false;

	// Tentatively place the stone on the board.
	this->squareMatrix[cellLocation.i][cellLocation.j] = cellState;

	// Remove any groups that may have been killed by the placement of the stone.
	for (int i = 0; i < 4; i++)
	{
		CellLocation adjacentLocation = cellLocation.GetAdjcentLocation(i);
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

	// After trying to remove any captured stones, if the placed stone has no liberties, then the move was invalid.
	ConnectedRegion* group = this->SenseConnectedRegion(cellLocation);
	check(group->type == ConnectedRegion::GROUP);
	if (group->libertiesSet.Num() == 0)
	{
		this->squareMatrix[cellLocation.i][cellLocation.j] = EGoGameCellState::Empty;
		return false;
	}

	// Lastly, the game must always move forward.  Make sure the board state is not that of the given forbidden matrix.
	if (forbiddenMatrix && this->CellStateSameAs(forbiddenMatrix))
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

bool GoGameMatrix::CellStateSameAs(const GoGameMatrix* gameMatrix) const
{
	if (this->squareMatrixSize != gameMatrix->squareMatrixSize)
		return false;

	for (int i = 0; i < this->squareMatrixSize; i++)
		for (int j = 0; j < this->squareMatrixSize; j++)
			if (this->squareMatrix[i][j] != gameMatrix->squareMatrix[i][j])
				return false;

	return true;
}

bool GoGameMatrix::GetCellState(const CellLocation& cellLocation, EGoGameCellState& cellState) const
{
	if (!this->IsInBounds(cellLocation))
		return false;

	cellState = this->squareMatrix[cellLocation.i][cellLocation.j];
	return true;
}

GoGameMatrix::CellLocation GoGameMatrix::CellLocation::GetAdjcentLocation(int adjacency) const
{
	CellLocation adjacentLocation = *this;
	
	if (adjacency == 0)
		adjacentLocation.i--;
	else if (adjacency == 1)
		adjacentLocation.i++;
	else if (adjacency == 2)
		adjacentLocation.j--;
	else if (adjacency == 3)
		adjacentLocation.j++;

	return adjacentLocation;
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

		for (int i = 0; i < 4; i++)
		{
			CellLocation adjacentLocation = nextCellLocation.GetAdjcentLocation(i);
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

	if (region->type == ConnectedRegion::TERRITORY)
	{
		if (blackBoundaryCount > 0 && whiteBondaryCount == 0)
			region->owner = EGoGameCellState::Black;
		else if (whiteBondaryCount > 0 && blackBoundaryCount == 0)
			region->owner = EGoGameCellState::White;
	}

	return region;
}

void GoGameMatrix::CollectAllRegionsOfType(EGoGameCellState targetCellState, TArray<ConnectedRegion*>& regionArray) const
{
	regionArray.Reset();

	for (int i = 0; i < this->squareMatrixSize; i++)
	{
		for (int j = 0; j < this->squareMatrixSize; j++)
		{
			EGoGameCellState cellState = this->squareMatrix[i][j];
			if (cellState == targetCellState)
			{
				CellLocation cellLocation(i, j);

				bool alreadyCounted = false;
				for (int k = 0; k < regionArray.Num() && !alreadyCounted; k++)
					if (regionArray[k]->membersSet.Contains(cellLocation))
						alreadyCounted = true;

				if (!alreadyCounted)
				{
					ConnectedRegion* region = this->SenseConnectedRegion(cellLocation);
					regionArray.Add(region);
				}
			}
		}
	}
}

EGoGameCellState GoGameMatrix::CalculateCurrentWinner(int& scoreDelta) const
{
	EGoGameCellState winner = EGoGameCellState::Empty;
	scoreDelta = 0;

	int blackTerritoryCount = 0;
	int whiteTerritoryCount = 0;

	TArray<ConnectedRegion*> territoryArray;
	this->CollectAllRegionsOfType(EGoGameCellState::Empty, territoryArray);
	for (int i = 0; i < territoryArray.Num(); i++)
	{
		ConnectedRegion* territory = territoryArray[i];
		check(territory->type == ConnectedRegion::TERRITORY);
		if (territory->owner == EGoGameCellState::Black)
			blackTerritoryCount += territory->membersSet.Num();
		else if (territory->owner == EGoGameCellState::White)
			whiteTerritoryCount += territory->membersSet.Num();
		delete territory;
	}

	blackTerritoryCount -= this->whiteCaptureCount;
	whiteTerritoryCount -= this->blackCaptureCount;

	scoreDelta = blackTerritoryCount - whiteTerritoryCount;
	if (scoreDelta > 0)
		winner = EGoGameCellState::Black;
	else if (scoreDelta < 0)
		winner = EGoGameCellState::White;

	scoreDelta = ::abs(scoreDelta);

	return winner;
}

GoGameMatrix::ConnectedRegion::ConnectedRegion()
{
}

/*virtual*/ GoGameMatrix::ConnectedRegion::~ConnectedRegion()
{
}

END_FUNCTION_BUILD_OPTIMIZATION