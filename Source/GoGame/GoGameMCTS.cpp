#include "GoGameMCTS.h"
#include "GoGameState.h"
#include "Kismet/GameplayStatics.h"

GoGameMCTS::GoGameMCTS()
{
	this->rootNode = nullptr;
	this->totalIterationCount = 0;
	this->gameMatrix = nullptr;
	this->favoredPlayer = EGoGameCellState::Empty;
	this->baseCaptureCount = 0;
	this->baseTerritoryCount = 0;
}

/*virtual*/ GoGameMCTS::~GoGameMCTS()
{
	delete this->gameMatrix;
	delete this->rootNode;
}

bool GoGameMCTS::PerformSingleIteration()
{
	if (this->favoredPlayer == EGoGameCellState::Empty)
		return false;

	if (!this->gameMatrix || this->gameMatrix->GetWhoseTurn() != this->favoredPlayer)
		return false;

	if (!this->rootNode)
	{
		this->rootNode = new Node();
		this->rootNode->CrackOpen(this->gameMatrix);
	}

	GoGameMatrix* trialGameMatrix = new GoGameMatrix(this->gameMatrix);
	Node* leafNode = this->rootNode->SelectLeaf(trialGameMatrix);
	if (leafNode)
	{
		if (leafNode->visitCount > 0.0)
		{
			leafNode->CrackOpen(trialGameMatrix);
			leafNode = leafNode->SelectLeaf(trialGameMatrix);
		}

		if (leafNode)
		{
			double reward = this->PerformRollout(trialGameMatrix);
			leafNode->Backpropagate(reward);
		}
	}

	delete trialGameMatrix;

	this->totalIterationCount++;

	return true;
}

bool GoGameMCTS::GetEstimatedBestMove(GoGameMatrix::CellLocation& stonePlacement) const
{
	if (!this->rootNode)
		return false;

	double largestRewardCount = -TNumericLimits<double>::Max();
	const Node* bestNode = nullptr;

	for (int i = 0; i < this->rootNode->childNodeArray.Num(); i++)
	{
		const Node* childNode = this->rootNode->childNodeArray[i];
		if (childNode->rewardCount > largestRewardCount)
		{
			largestRewardCount = childNode->rewardCount;
			bestNode = childNode;
		}
	}

	if (!bestNode)
		return false;

	stonePlacement = bestNode->stonePlacement;
	return true;
}

// If this fails, then it could mean the user chose a move we never considered,
// so the MCTS object should be destroyed and a new one created.
bool GoGameMCTS::PruneAtRoot(const GoGameMatrix::CellLocation& stonePlacement)
{
	if (!this->rootNode || !this->gameMatrix)
		return false;

	Node* newRootNode = nullptr;

	for (int i = 0; i < this->rootNode->childNodeArray.Num(); i++)
	{
		Node* childNode = this->rootNode->childNodeArray[i];
		if (childNode->stonePlacement == stonePlacement)
		{
			newRootNode = childNode;
			this->rootNode->childNodeArray[i] = nullptr;
			break;
		}
	}

	if (!newRootNode)
		return false;

	delete this->rootNode;
	this->rootNode = newRootNode;

	bool success = this->gameMatrix->SetCellState(stonePlacement, this->gameMatrix->GetWhoseTurn(), nullptr);
	check(success);

	return true;
}

double GoGameMCTS::PerformRollout(GoGameMatrix* trialGameMatrix)
{
	double reward = 0.0;

	int scoreDelta = 0;
	int blackTerritoryCount = 0;
	int whiteTerritoryCount = 0;
	(void)trialGameMatrix->CalculateCurrentWinner(scoreDelta, blackTerritoryCount, whiteTerritoryCount);
	
	if (this->favoredPlayer == EGoGameCellState::Black)
	{
		this->baseCaptureCount = trialGameMatrix->GetBlackCaptureCount();
		this->baseTerritoryCount = blackTerritoryCount;
	}
	else if (this->favoredPlayer == EGoGameCellState::White)
	{
		this->baseCaptureCount = trialGameMatrix->GetWhiteCaptureCount();
		this->baseTerritoryCount = whiteTerritoryCount;
	}

	this->PerformRolloutRecursive(trialGameMatrix, reward, 1, 3);
	return reward;
}

void GoGameMCTS::PerformRolloutRecursive(GoGameMatrix* trialGameMatrix, double& reward, int depth, int maxDepth)
{
	if (depth >= maxDepth)
	{
		// Accumulate reward, if any.
		int scoreDelta = 0;
		int blackTerritoryCount = 0;
		int whiteTerritoryCount = 0;
		(void)trialGameMatrix->CalculateCurrentWinner(scoreDelta, blackTerritoryCount, whiteTerritoryCount);

		int currentCaptureCount = 0;
		int currentTerritoryCount = 0;

		if (this->favoredPlayer == EGoGameCellState::Black)
		{
			currentCaptureCount = gameMatrix->GetBlackCaptureCount();
			currentTerritoryCount = blackTerritoryCount;
		}
		else if (this->favoredPlayer == EGoGameCellState::White)
		{
			currentCaptureCount = gameMatrix->GetWhiteCaptureCount();
			currentTerritoryCount = whiteTerritoryCount;
		}

		if (currentCaptureCount > this->baseCaptureCount)
			reward += double(currentCaptureCount - this->baseCaptureCount);

		if (currentTerritoryCount > this->baseTerritoryCount)
			reward += double(currentTerritoryCount - this->baseTerritoryCount);

		return;
	}

	for (int i = 0; i < trialGameMatrix->GetMatrixSize(); i++)
	{
		for (int j = 0; j < trialGameMatrix->GetMatrixSize(); j++)
		{
			GoGameMatrix::CellLocation cellLocation(i, j);
			EGoGameCellState cellState;
			trialGameMatrix->GetCellState(cellLocation, cellState);
			if (cellState == EGoGameCellState::Empty)
			{
				// TODO: We might limit ourselves to free cells within a given distance of an occupied cell, just to reduce the branch factor.
				GoGameMatrix* subTrialGameMatrix = new GoGameMatrix(trialGameMatrix);
				if (subTrialGameMatrix->SetCellState(cellLocation, subTrialGameMatrix->GetWhoseTurn(), nullptr))
					this->PerformRolloutRecursive(subTrialGameMatrix, reward, depth + 1, maxDepth);
				delete subTrialGameMatrix;
			}
		}
	}
}

GoGameMCTS::Node::Node()
{
	this->rewardCount = 0.0;
	this->visitCount = 0.0;
	this->parentNode = nullptr;
}

/*virtual*/ GoGameMCTS::Node::~Node()
{
	for (int i = 0; i < this->childNodeArray.Num(); i++)
		delete this->childNodeArray[i];
}

const double GoGameMCTS::Node::ucbConstant = 2.0;

double GoGameMCTS::Node::CalculateUCB() const
{
	if (!this->visitCount)
		return TNumericLimits<double>::Max();

	// Caluclate the upper confidence bound.
	double exploitationTerm = this->rewardCount / this->visitCount;
	double explorationTerm = ucbConstant * ::sqrt(::log(parentNode->visitCount) / this->visitCount);
	return exploitationTerm + explorationTerm;
}

GoGameMCTS::Node* GoGameMCTS::Node::SelectLeaf(GoGameMatrix* gameMatrix)
{
	Node* selectedNode = this;

	while (selectedNode && selectedNode->childNodeArray.Num() > 0)
	{
		Node* bestChildNode = nullptr;
		double ucbScoreMax = -TNumericLimits<double>::Max();
		for (int i = 0; i < this->childNodeArray.Num(); i++)
		{
			Node* childNode = this->childNodeArray[i];
			double ucbScore = childNode->CalculateUCB();
			if (ucbScore > ucbScoreMax)
			{
				ucbScoreMax = ucbScore;
				bestChildNode = childNode;
			}
		}

		if (bestChildNode)
		{
			bool success = gameMatrix->SetCellState(bestChildNode->stonePlacement, gameMatrix->GetWhoseTurn(), nullptr);
			check(success);
		}

		selectedNode = bestChildNode;
	}

	return selectedNode;
}

bool GoGameMCTS::Node::CrackOpen(const GoGameMatrix* gameMatrix)
{
	if (this->childNodeArray.Num() > 0)
		return false;

	TSet<GoGameMatrix::CellLocation> cellLocationSet;
	gameMatrix->GenerateAllPossiblePlacements(cellLocationSet);
	for(GoGameMatrix::CellLocation& cellLocation : cellLocationSet)
	{
		Node* childNode = new Node();
		childNode->stonePlacement = cellLocation;
		childNode->parentNode = this;
		this->childNodeArray.Add(childNode);
	}

	return true;
}

void GoGameMCTS::Node::Backpropagate(double reward)
{
	Node* node = this;
	while (node)
	{
		this->rewardCount += reward;
		this->visitCount += 1.0;
		node = node->parentNode;
	}
}