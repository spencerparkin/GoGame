#include "GoGameMCTS.h"
#include "GoGameState.h"
#include "Kismet/GameplayStatics.h"

GoGameMCTS::GoGameMCTS()
{
	this->rootNode = nullptr;
	this->totalIterationCount = 0;
	this->gameMatrix = nullptr;
	this->favoredPlayer = EGoGameCellState::Empty;
	this->baseStatus = nullptr;
}

/*virtual*/ GoGameMCTS::~GoGameMCTS()
{
	delete this->gameMatrix;
	delete this->rootNode;
	delete this->baseStatus;
}

bool GoGameMCTS::PerformSingleIteration()
{
	if (this->favoredPlayer == EGoGameCellState::Empty)
		return false;

	if (!this->gameMatrix || this->gameMatrix->GetWhoseTurn() != this->favoredPlayer)
		return false;

	if (!this->baseStatus)
	{
		this->baseStatus = new BoardStatus();
		this->baseStatus->GatherStatusInfo(this->gameMatrix, this->favoredPlayer);
	}

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
	this->PerformRolloutRecursive(trialGameMatrix, reward, 1, GO_GAME_MCTS_MAX_ROLLOUT_DEPTH);
	return reward;
}

void GoGameMCTS::PerformRolloutRecursive(GoGameMatrix* trialGameMatrix, double& reward, int depth, int maxDepth)
{
	if (depth >= maxDepth)
	{
		// Accumulate reward, if any.
		BoardStatus currentStatus;
		currentStatus.GatherStatusInfo(trialGameMatrix, this->favoredPlayer);

		//reward += double(currentStatus.favoredTerritoryCount - this->baseStatus->favoredTerritoryCount);
		//reward -= double(currentStatus.opponentTerritoryCount - this->baseStatus->opponentTerritoryCount);
		
		reward += double(currentStatus.favoredCaptureCount - this->baseStatus->favoredCaptureCount) * 1000.0;
		reward -= double(currentStatus.opponentCaptureCount - this->baseStatus->opponentCaptureCount) * 1000.0;

		reward += double(currentStatus.favoredLibertyCount - this->baseStatus->favoredLibertyCount);
		reward -= double(currentStatus.opponentLibertyCount - this->baseStatus->opponentLibertyCount);
		
		reward -= double(currentStatus.favoredAtariCount - this->baseStatus->favoredAtariCount) * 10.0;
		reward += double(currentStatus.opponentAtariCount - this->baseStatus->favoredAtariCount) * 10.0;

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
#if defined GO_GAME_MCTS_PLAY_NEAR_OCCUPANCIES
				if (trialGameMatrix->TaxicabDistanceToNearestOccupiedCell(cellLocation) > GO_GAME_MCTS_MAX_TAXICAB_DISTANCE)
					continue;
#endif
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
	if (this->visitCount == 0.0)
		return TNumericLimits<double>::Max();

	check(this->parentNode != nullptr);
	check(this->parentNode->visitCount > 0.0);

	// Caluclate the upper confidence bound.
	double exploitationTerm = this->rewardCount / this->visitCount;
	double explorationTerm = ucbConstant * ::sqrt(::log(this->parentNode->visitCount) / this->visitCount);
	return exploitationTerm + explorationTerm;
}

GoGameMCTS::Node* GoGameMCTS::Node::SelectLeaf(GoGameMatrix* gameMatrix)
{
	Node* selectedNode = this;

	while (selectedNode && selectedNode->childNodeArray.Num() > 0)
	{
		Node* bestChildNode = nullptr;
		double ucbScoreMax = -TNumericLimits<double>::Max();
		for (int i = 0; i < selectedNode->childNodeArray.Num(); i++)
		{
			Node* childNode = selectedNode->childNodeArray[i];
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
#if defined GO_GAME_MCTS_PLAY_NEAR_OCCUPANCIES
		if (gameMatrix->TaxicabDistanceToNearestOccupiedCell(cellLocation) > GO_GAME_MCTS_MAX_TAXICAB_DISTANCE)
			continue;
#endif
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
		node->rewardCount += reward;
		node->visitCount += 1.0;
		node = node->parentNode;
	}
}

GoGameMCTS::BoardStatus::BoardStatus()
{
	this->favoredTerritoryCount = 0;
	this->favoredCaptureCount = 0;
	this->opponentTerritoryCount = 0;
	this->opponentCaptureCount = 0;
}

/*virtual*/ GoGameMCTS::BoardStatus::~BoardStatus()
{
}

void GoGameMCTS::BoardStatus::GatherStatusInfo(GoGameMatrix* gameMatrix, EGoGameCellState favoredPlayer)
{
	int scoreDelta = 0;
	int blackTerritoryCount = 0;
	int whiteTerritoryCount = 0;
	(void)gameMatrix->CalculateCurrentWinner(scoreDelta, blackTerritoryCount, whiteTerritoryCount);

	TArray<GoGameMatrix::ConnectedRegion*> blackGroupsArray;
	gameMatrix->CollectAllRegionsOfType(EGoGameCellState::Black, blackGroupsArray);
	int numBlackGroupsInAtari = 0;
	int totalBlackLiberties = 0;
	for (int i = 0; i < blackGroupsArray.Num(); i++)
	{
		GoGameMatrix::ConnectedRegion* blackGroup = blackGroupsArray[i];
		if (blackGroup->libertiesSet.Num() == 1)
			numBlackGroupsInAtari++;
		totalBlackLiberties += blackGroup->libertiesSet.Num();
		delete blackGroup;
	}

	TArray<GoGameMatrix::ConnectedRegion*> whiteGroupsArray;
	gameMatrix->CollectAllRegionsOfType(EGoGameCellState::White, whiteGroupsArray);
	int numWhiteGroupsInAtari = 0;
	int totalWhiteLiberties = 0;
	for (int i = 0; i < whiteGroupsArray.Num(); i++)
	{
		GoGameMatrix::ConnectedRegion* whiteGroup = whiteGroupsArray[i];
		if (whiteGroup->libertiesSet.Num() == 1)
			numWhiteGroupsInAtari++;
		totalWhiteLiberties += whiteGroup->libertiesSet.Num();
		delete whiteGroup;
	}

	if (favoredPlayer == EGoGameCellState::Black)
	{
		this->favoredTerritoryCount = blackTerritoryCount;
		this->favoredCaptureCount = gameMatrix->GetBlackCaptureCount();
		this->favoredLibertyCount = totalBlackLiberties;
		this->favoredAtariCount = numBlackGroupsInAtari;

		this->opponentTerritoryCount = whiteTerritoryCount;
		this->opponentCaptureCount = gameMatrix->GetWhiteCaptureCount();
		this->opponentLibertyCount = totalWhiteLiberties;
		this->opponentAtariCount = numWhiteGroupsInAtari;
	}
	else if (favoredPlayer == EGoGameCellState::White)
	{
		this->favoredTerritoryCount = whiteTerritoryCount;
		this->favoredCaptureCount = gameMatrix->GetWhiteCaptureCount();
		this->favoredLibertyCount = totalWhiteLiberties;
		this->favoredAtariCount = numWhiteGroupsInAtari;

		this->opponentTerritoryCount = blackTerritoryCount;
		this->opponentCaptureCount = gameMatrix->GetBlackCaptureCount();
		this->opponentLibertyCount = totalBlackLiberties;
		this->opponentAtariCount = numBlackGroupsInAtari;
	}
}