#pragma once

#include "GoGameAI.h"

class AGoGameState;

class GoGameIdiotAI : public GoGameAI
{
public:
	GoGameIdiotAI(EGoGameCellState favoredColor);
	virtual ~GoGameIdiotAI();

	virtual void BeginThinking() override;
	virtual void StopThinking() override;
	virtual bool TickThinking() override;

private:

	bool CalculateStonePlacement();
	bool CaptureOpponentGroupsInAtari(const TSet<GoGameMatrix::CellLocation>& validMovesSet);
	bool PutOpponentGroupsInAtari(const TSet<GoGameMatrix::CellLocation>& validMovesSet);
	bool SaveFavoredAtariGroupsFromCapture(const TSet<GoGameMatrix::CellLocation>& validMovesSet);
	bool PreventFavoredGroupsFromGettingIntoAtari(const TSet<GoGameMatrix::CellLocation>& validMovesSet);
	bool FightInDuelCluster(const TSet<GoGameMatrix::CellLocation>& validMovesSet);

	struct Candidate
	{
		GoGameMatrix::CellLocation cellLocation;
		float score;
	};

	bool ScoreAndSelectBestPlacement(TFunctionRef<float(GoGameMatrix* gameMatrix, const GoGameMatrix::CellLocation& cellLocation)> scoreFunction);

	// A duel cluster is a bunch of stones in contention with one another on the board.
	// Precisely, it is found using a BFS from any initial stone of any color to any
	// other stone of any color, provided there is a path between them that travels along
	// occupied cells that are adjacent or kitty-corner.
	class DuelCluster
	{
	public:
		DuelCluster();
		virtual ~DuelCluster();

		void Generate(GoGameMatrix* gameMatrix, const GoGameMatrix::CellLocation& rootCell, EGoGameCellState favoredStone);
		void Clear();

		int GetTotalNumberOfFavoredStones() const;
		int GetTotalNumberOfOpponentStones() const;

		float TotalFavoredToOpponentStoneRatio() const;

		void ForAllStones(TFunctionRef<void(const GoGameMatrix::CellLocation& cellLocation)> visitFunction);

		TArray<GoGameMatrix::ConnectedRegion*> favoredGroupsArray;
		TArray<GoGameMatrix::ConnectedRegion*> opponentGroupsArray;
	};
	
	void FindAllDuelClusters(GoGameMatrix* gameMatrix, TArray<DuelCluster*>& duelClusterArray);

	bool AnyTwoCellsAdjacent(const TSet<GoGameMatrix::CellLocation>& cellSetA, const TSet<GoGameMatrix::CellLocation>& cellSetB);
};