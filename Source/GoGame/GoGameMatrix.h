#pragma once

#include "CoreMinimal.h"
#include "Misc/Crc.h"

UENUM(BlueprintType)
enum class EGoGameCellState : uint8
{
	Empty,
	Black,
	White,
	Black_or_White
};

class GoGameMatrix
{
public:
	GoGameMatrix();
	GoGameMatrix(const GoGameMatrix* gameMatrix);
	virtual ~GoGameMatrix();

	bool SetMatrixSize(int givenSize);
	int GetMatrixSize();

	struct CellLocation
	{
		int i, j;

		bool operator==(const CellLocation& location) const
		{
			return this->i == location.i && this->j == location.j;
		}

		CellLocation GetAdjcentLocation(int i) const;
	};

	bool SetCellState(const CellLocation& cellLocation, EGoGameCellState cellState, const GoGameMatrix* forbiddenMatrix);
	bool GetCellState(const CellLocation& cellLocation, EGoGameCellState& cellState) const;

	bool IsInBounds(const CellLocation& cellLocation) const;

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
	EGoGameCellState CalculateCurrentWinner(int& scoreDelta) const;
	bool CellStateSameAs(const GoGameMatrix* gameMatrix) const;

private:

	void FreeMatrix();

	int squareMatrixSize;
	EGoGameCellState** squareMatrix;

	int whiteCaptureCount;
	int blackCaptureCount;

	EGoGameCellState whoseTurn;
};

inline uint32 GetTypeHash(const GoGameMatrix::CellLocation& cellLocation)
{
	return FCrc::MemCrc32(&cellLocation, sizeof(cellLocation));
}