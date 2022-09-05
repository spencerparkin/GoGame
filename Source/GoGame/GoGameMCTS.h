#pragma once

#include "CoreMinimal.h"
#include "GoGameMatrix.h"

#define GO_GAME_MCTS_PLAY_NEAR_OCCUPANCIES
#define GO_GAME_MCTS_MAX_TAXICAB_DISTANCE	2
#define GO_GAME_MCTS_MAX_ROLLOUT_DEPTH		3

class GoGameMCTS
{
public:
	GoGameMCTS();
	virtual ~GoGameMCTS();

	bool PerformSingleIteration();
	bool GetEstimatedBestMove(GoGameMatrix::CellLocation& stonePlacement) const;
	bool PruneAtRoot(const GoGameMatrix::CellLocation& stonePlacement);

	class Node
	{
	public:
		Node();
		virtual ~Node();

		double CalculateUCB() const;
		bool CrackOpen(const GoGameMatrix* gameMatrix);
		Node* SelectLeaf(GoGameMatrix* gameMatrix);
		void Backpropagate(double reward);

		TArray<Node*> childNodeArray;
		Node* parentNode;

		GoGameMatrix::CellLocation stonePlacement;

		double visitCount;
		double rewardCount;

		static const double ucbConstant;
	};

	double PerformRollout(GoGameMatrix* trialGameMatrix);
	void PerformRolloutRecursive(GoGameMatrix* trialGameMatrix, double& reward, int depth, int maxDepth);

	Node* rootNode;
	int totalIterationCount;
	GoGameMatrix* gameMatrix;
	EGoGameCellState favoredPlayer;

	class BoardStatus
	{
	public:
		BoardStatus();
		virtual ~BoardStatus();

		void GatherStatusInfo(GoGameMatrix* gameMatrix, EGoGameCellState favoredPlayer);

		int favoredTerritoryCount;
		int favoredCaptureCount;
		int favoredLibertyCount;
		int favoredAtariCount;
		int opponentTerritoryCount;
		int opponentCaptureCount;
		int opponentLibertyCount;
		int opponentAtariCount;
	};

	BoardStatus* baseStatus;
};