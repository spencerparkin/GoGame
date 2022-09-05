#pragma once

#include "CoreMinimal.h"
#include "Misc/Crc.h"

UENUM(BlueprintType)
enum class EGoGameCellState : uint8
{
	Empty,
	Black,
	White
};

class GoGameMatrix
{
public:
	GoGameMatrix();
	GoGameMatrix(const GoGameMatrix* gameMatrix);
	virtual ~GoGameMatrix();

	bool SetMatrixSize(int givenSize);
	int GetMatrixSize() const;

	class CellLocation
	{
	public:
		CellLocation()
		{
			this->i = -1;
			this->j = -1;
		}

		CellLocation(int i, int j)
		{
			this->i = i;
			this->j = j;
		}

		int i, j;

		bool operator==(const CellLocation& location) const
		{
			return this->i == location.i && this->j == location.j;
		}

		CellLocation GetAdjcentLocation(int i) const;
	};

	bool SetCellState(const CellLocation& cellLocation, EGoGameCellState cellState, const GoGameMatrix* forbiddenMatrix, bool ignoreWhoseTurnItIs = false);
	bool GetCellState(const CellLocation& cellLocation, EGoGameCellState& cellState) const;

	bool IsInBounds(const CellLocation& cellLocation) const;
	bool IsLiberty(const CellLocation& cellLocation) const;

	bool Pass(EGoGameCellState cellState);

	class ConnectedRegion
	{
	public:
		ConnectedRegion();
		virtual ~ConnectedRegion();

		enum Type
		{
			GROUP,
			TERRITORY
		};

		// A connected region can be a group or territory.
		Type type;

		// These are the board locations comprising the group or territory.
		TSet<CellLocation> membersSet;

		// These are the liberties of the group, or empty if territory.
		TSet<CellLocation> libertiesSet;

		// If a group, this is the owner of the group.
		// If territory, this is the owner of the territory, or empty if the territory is still contested.
		EGoGameCellState owner;
	};

	ConnectedRegion* SenseConnectedRegion(const CellLocation& cellLocation) const;
	EGoGameCellState GetWhoseTurn() const { return this->whoseTurn; }
	EGoGameCellState CalculateCurrentWinner(int& scoreDelta, int& blackTerritoryCount, int& whiteTerritoryCount) const;
	bool CellStateSameAs(const GoGameMatrix* gameMatrix) const;
	void CollectAllRegionsOfType(EGoGameCellState targetCellState, TArray<ConnectedRegion*>& regionArray) const;
	bool FindAllImmortalGroupsOfColor(EGoGameCellState color, TArray<GoGameMatrix::ConnectedRegion*>& immortalGroupArray) const;
	int GetBlackCaptureCount() const { return this->blackCaptureCount; }
	int GetWhiteCaptureCount() const { return this->whiteCaptureCount; }
	bool IsGameOver() const { return this->gameOver; }
	void GenerateAllPossiblePlacements(TSet<GoGameMatrix::CellLocation>& cellLocationSet) const;
	int TaxicabDistanceToNearestOccupiedCell(const GoGameMatrix::CellLocation& cellLocation) const;

private:

	void FreeMatrix();

	bool gameOver;
	int consecutivePassCount;

	int squareMatrixSize;
	EGoGameCellState** squareMatrix;

	int whiteCaptureCount;
	int blackCaptureCount;

	EGoGameCellState whoseTurn;

	void TurnFlip();
};

inline uint32 GetTypeHash(const GoGameMatrix::CellLocation& cellLocation)
{
	return FCrc::MemCrc32(&cellLocation, sizeof(cellLocation));
}