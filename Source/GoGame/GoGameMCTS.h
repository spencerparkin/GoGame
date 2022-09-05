#pragma once

#include "CoreMinimal.h"
#include "GoGameMatrix.h"

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
	GoGameMatrix* gameMatrix;
	EGoGameCellState favoredPlayer;
	int baseCaptureCount;
	int baseTerritoryCount;
};