#pragma once

#include "GoGameMatrix.h"

class AGoGameState;

// TODO: Derive from base AI class and override asynchronous interface so that
//       the user can see a bit of time pass before the AI places a stone.
class GoGameIdiotAI
{
public:
	GoGameIdiotAI(EGoGameCellState favoredColor);
	virtual ~GoGameIdiotAI();

	bool CalculateStonePlacement(AGoGameState* gameState, GoGameMatrix::CellLocation& stonePlacement);

private:

	bool CaptureOpponentGroupsInAtari(AGoGameState* gameState, GoGameMatrix::CellLocation& stonePlacement, const TSet<GoGameMatrix::CellLocation>& validMovesSet);
	bool PutOpponentGroupsInAtari(AGoGameState* gameState, GoGameMatrix::CellLocation& stonePlacement, const TSet<GoGameMatrix::CellLocation>& validMovesSet);
	bool SaveFavoredAtariGroupsFromCapture(AGoGameState* gameState, GoGameMatrix::CellLocation& stonePlacement, const TSet<GoGameMatrix::CellLocation>& validMovesSet);
	bool PreventFavoredGroupsFromGettingIntoAtari(AGoGameState* gameState, GoGameMatrix::CellLocation& stonePlacement, const TSet<GoGameMatrix::CellLocation>& validMovesSet);
	bool FightInDuelCluster(AGoGameState* gameState, GoGameMatrix::CellLocation& stonePlacement, const TSet<GoGameMatrix::CellLocation>& validMovesSet);

	EGoGameCellState favoredPlayer;

	struct Candidate
	{
		GoGameMatrix::CellLocation cellLocation;
		float score;
	};

	bool ScoreAndSelectBestPlacement(AGoGameState* gameState, TFunctionRef<float(GoGameMatrix* gameMatrix, const GoGameMatrix::CellLocation& cellLocation)> scoreFunction, GoGameMatrix::CellLocation& stonePlacement);

	// A duel cluster is a bunch of stones in contention with one another on the board.
	// Precisely, it is found using a BFS from any initial stone of any color to any
	// other stone of any color, provided there is a path between them that travels along
	// occupied cells that are adjacent or kitty-corner.
	class DuelCluster
	{
	public:
		DuelCluster();
		virtual ~DuelCluster();

		void Generate(GoGameMatrix* gameMatrix, const GoGameMatrix::CellLocation& rootCell, EGoGameCellState favoredPlayer);
		void Clear();

		int GetTotalNumberOfFavoredStones() const;
		int GetTotalNumberOfOpponentStones() const;

		float TotalFavoredToOpponentStoneRatio() const;

		void ForAllStones(TFunctionRef<void(const GoGameMatrix::CellLocation& cellLocation)> visitFunction);

		TArray<GoGameMatrix::ConnectedRegion*> favoredGroupsArray;
		TArray<GoGameMatrix::ConnectedRegion*> opponentGroupsArray;
	};
	
	void FindAllDuelClusters(GoGameMatrix* gameMatrix, TArray<DuelCluster*>& duelClusterArray);
};