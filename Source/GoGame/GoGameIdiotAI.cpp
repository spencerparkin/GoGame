#include "GoGameIdiotAI.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGameIdiotAI, Log, All);

// The goal in go is to capture the most territory.  The goal here, however, is to just keep from losing by a landslide.
// To capture territory, one usually has to try to think about forming structure on the board.  I have no idea how to program that here.
// Rather, my goal here is to just attack the player at every opportunity.  I'm sure that the result will still be a very
// mediocre go player AI, but I can at least have fun with this.
GoGameIdiotAI::GoGameIdiotAI(EGoGameCellState favoredColor) : GoGameAI(favoredColor)
{
}

/*virtual*/ GoGameIdiotAI::~GoGameIdiotAI()
{
}

/*virtual*/ void GoGameIdiotAI::BeginThinking()
{
	GoGameAI::BeginThinking();

	// This AI is fast enough that we don't need to kick off any threads or do any ticking.
	// Just calculate it now.  Of course, this probably also indicates how dumb we are, because
	// a good AI would need a lot more processing time to make a decision.
	this->CalculateStonePlacement();
}

/*virtual*/ void GoGameIdiotAI::StopThinking()
{
	GoGameAI::StopThinking();
}

/*virtual*/ bool GoGameIdiotAI::TickThinking()
{
	// Indicate that we're done thinking.
	return true;
}

bool GoGameIdiotAI::ScoreAndSelectBestPlacement(TFunctionRef<float(GoGameMatrix* gameMatrix, const GoGameMatrix::CellLocation& cellLocation)> scoreFunction)
{
	GoGameMatrix* gameMatrix = this->gameState->GetCurrentMatrix();
	GoGameMatrix* forbiddenMatrix = this->gameState->GetForbiddenMatrix();
	TSet<GoGameMatrix::CellLocation> cellLocationSet;
	gameMatrix->GenerateAllPossiblePlacements(cellLocationSet, forbiddenMatrix);

	TArray<Candidate> candidateArray;
	for (GoGameMatrix::CellLocation cellLocation : cellLocationSet)
	{
		Candidate candidate;
		candidate.cellLocation = cellLocation;
		candidate.score = scoreFunction(gameMatrix, cellLocation);
		candidateArray.Add(candidate);
	}

	if (candidateArray.Num() == 0)
		return false;

	candidateArray.Sort([](const Candidate& candidateA, const Candidate& candidateB) -> bool {
		return candidateA.score > candidateB.score;
	});

	if (candidateArray[0].score <= 0.0f)
		return false;

	// TODO: Select randomly from all that share the same highest score.
	this->stonePlacement = candidateArray[0].cellLocation;
	return true;
}

bool GoGameIdiotAI::CalculateStonePlacement()
{
	if (this->favoredPlayer == EGoGameCellState::Empty)
		return false;

	GoGameMatrix* gameMatrix = this->gameState->GetCurrentMatrix();
	if (!gameMatrix)
		return false;

	if (!gameMatrix || gameMatrix->GetWhoseTurn() != this->favoredPlayer)
		return false;

	GoGameMatrix* forbiddenMatrix = this->gameState->GetForbiddenMatrix();
	TSet<GoGameMatrix::CellLocation> validMovesSet;
	gameMatrix->GenerateAllPossiblePlacements(validMovesSet, forbiddenMatrix);

	if (this->CaptureOpponentGroupsInAtari(validMovesSet))
		return true;

	// Note that we try to put an opponent's group in atari before saving ourselves from atari.
	// This can work as a defense as long as a favored group in atari isn't needed to keep
	// the said opponent's group in atari.  Even so, it still might be a bad idea to go on
	// the offensive instead of the defensive, because our guarenteed capture may not be
	// anywhere near as substantial as the opponent's guarenteed capture.
	if (this->PutOpponentGroupsInAtari(validMovesSet))
		return true;

	if (this->SaveFavoredAtariGroupsFromCapture(validMovesSet))
		return true;

	if (this->PreventFavoredGroupsFromGettingIntoAtari(validMovesSet))
		return true;

	// TODO: It might be good to make a move that turns one or more of our groups immportal by eye-space or mutual-life.
	//       It's not hard to detect immortal groups, but it's hard to somehow plan to create them from an AI perspective.

	if (this->FightInDuelCluster(validMovesSet))
		return true;

	// If all else fails, we pass.
	this->stonePlacement.i = TNumericLimits<int>::Max();
	this->stonePlacement.j = TNumericLimits<int>::Max();
	return true;
}

// TODO: Note that sometimes a single stone placement can capture multiple groups, but I'm not checking for that here; I should.
bool GoGameIdiotAI::CaptureOpponentGroupsInAtari(const TSet<GoGameMatrix::CellLocation>& validMovesSet)
{
	// Go find all opponent groups in atari.
	bool captureMoveMade = false;
	EGoGameCellState opponentPlayer = (this->favoredPlayer == EGoGameCellState::Black) ? EGoGameCellState::White : EGoGameCellState::Black;
	TArray<GoGameMatrix::ConnectedRegion*> opponentGroupArray;
	GoGameMatrix* gameMatrix = this->gameState->GetCurrentMatrix();
	gameMatrix->CollectAllRegionsOfType(opponentPlayer, opponentGroupArray);
	opponentGroupArray.Sort([](GoGameMatrix::ConnectedRegion& groupA, GoGameMatrix::ConnectedRegion& groupB) -> bool {
		return groupA.membersSet.Num() > groupB.membersSet.Num();
	});

	// Examine them largest to smallest.  Capture them now if necessary.
	for (int i = 0; i < opponentGroupArray.Num() && !captureMoveMade; i++)
	{
		GoGameMatrix::ConnectedRegion* opponentGroup = opponentGroupArray[i];
		if (opponentGroup->libertiesSet.Num() == 1)
		{
			GoGameMatrix::CellLocation capturePlacement = *opponentGroup->libertiesSet.begin();
			if (!validMovesSet.Contains(capturePlacement))
				continue;

			// Defering the capture is a smart move if our time can be better spent elsewhere.
			// Try to determine if the capture must be done now so that we don't lose the opportunity.
			bool deferCapture = true;

			// If the opponent placing a stone where we would to capture the group does help save the opponent group, then we cannot defer.
			GoGameMatrix* trialMatrix = new GoGameMatrix(gameMatrix);
			if (trialMatrix->SetCellState(capturePlacement, opponentPlayer, nullptr, true))
			{
				GoGameMatrix::ConnectedRegion* opponentGroupAttemptedSave = trialMatrix->SenseConnectedRegion(capturePlacement);
				if (opponentGroupAttemptedSave->libertiesSet.Num() > 1)
					deferCapture = false;
				delete opponentGroupAttemptedSave;
			}
			delete trialMatrix;

			// Lastly, if we still think we can defer, we first must check that no group of ours surrounding the
			// opponent's atari group is itself in atari.
			if (deferCapture)
			{
				TArray<GoGameMatrix::ConnectedRegion*> atariGroupArray;
				gameMatrix->FindAllGroupsInAtariForColor(this->favoredPlayer, atariGroupArray);
				for (int j = 0; j < atariGroupArray.Num() && deferCapture; j++)
					if (this->AnyTwoCellsAdjacent(atariGroupArray[j]->membersSet, opponentGroup->membersSet))
						deferCapture = false;
			}

			// Okay, if we cannot defer the capture, make the capture now.
			if (!deferCapture)
			{
				this->stonePlacement = capturePlacement;
				if (validMovesSet.Contains(this->stonePlacement))
					captureMoveMade = true;
			}
		}
	}

	for (GoGameMatrix::ConnectedRegion* opponentGroup : opponentGroupArray)
		delete opponentGroup;

	return captureMoveMade;
}

bool GoGameIdiotAI::AnyTwoCellsAdjacent(const TSet<GoGameMatrix::CellLocation>& cellSetA, const TSet<GoGameMatrix::CellLocation>& cellSetB)
{
	for (auto iterA = cellSetA.CreateConstIterator(); iterA; ++iterA)
	{
		const GoGameMatrix::CellLocation& cellA = *iterA;

		for (auto iterB = cellSetB.CreateConstIterator(); iterB; ++iterB)
		{
			const GoGameMatrix::CellLocation& cellB = *iterB;

			for (int i = 0; i < 4; i++)
			{
				GoGameMatrix::CellLocation cellAdjacency = cellA.GetAdjacentLocation(i);
				if (cellAdjacency == cellB)
					return true;
			}
		}
	}

	return false;
}

bool GoGameIdiotAI::PutOpponentGroupsInAtari(const TSet<GoGameMatrix::CellLocation>& validMovesSet)
{
	// Look for a move that puts one or more opponent groups in atari.  Favor those, if any, that maximize the opponent's overall atari state.
	EGoGameCellState opponentPlayer = (this->favoredPlayer == EGoGameCellState::Black) ? EGoGameCellState::White : EGoGameCellState::Black;
	GoGameMatrix* gameMatrix = this->gameState->GetCurrentMatrix();
	int opponentGroupsInAtari = gameMatrix->CountGroupsInAtariForColor(opponentPlayer);
	int largestAtariIncrease = 0;
	for (GoGameMatrix::CellLocation cellLocation : validMovesSet)
	{
		GoGameMatrix* trialMatrix = new GoGameMatrix(gameMatrix);
		trialMatrix->SetCellState(cellLocation, this->favoredPlayer, nullptr);
		int atariIncrease = trialMatrix->CountGroupsInAtariForColor(opponentPlayer) - opponentGroupsInAtari;
		if (atariIncrease > largestAtariIncrease)
		{
			largestAtariIncrease = atariIncrease;
			this->stonePlacement = cellLocation;
		}
		delete trialMatrix;
	}

	if (largestAtariIncrease > 0)
	{
		int favoredGroupsInAtari = gameMatrix->CountGroupsInAtariForColor(this->favoredPlayer);
		if (favoredGroupsInAtari == 0)
			return true;

		// TODO: Find all groups that have the found stone-placement as a liberty and that have 2 liberties.
		//       Are any of those groups putting a favored group in atari?  If so, we don't want to make
		//       the move; we want to go try to save our groups in atari instead.
	}

	return false;
}

// Note that we currently don't do anything here to try to avoid being put in a latter situation.
// Latters are bad if they are going to hit the edge of the board.  They're okay if they're going to hit a favored stone.
bool GoGameIdiotAI::SaveFavoredAtariGroupsFromCapture(const TSet<GoGameMatrix::CellLocation>& validMovesSet)
{
	// Go find favored groups in atari.
	GoGameMatrix* gameMatrix = this->gameState->GetCurrentMatrix();
	TArray<GoGameMatrix::ConnectedRegion*> favoredGroupsInAtariArray;
	gameMatrix->FindAllGroupsInAtariForColor(this->favoredPlayer, favoredGroupsInAtariArray);

	// For each one, can we perform a saving move?  It would be nice here if we could detect
	// which groups are a lost cause.  Sometimes sacrifices are necessary for the greater whole.
	bool savingMoveMade = false;
	int largestLibertyIncrease = 0;
	for (int i = 0; i < favoredGroupsInAtariArray.Num(); i++)
	{
		GoGameMatrix::ConnectedRegion* favoredGroup = favoredGroupsInAtariArray[i];
		check(favoredGroup->libertiesSet.Num() == 1);
		
		GoGameMatrix::CellLocation savingStonePlacement = *favoredGroup->libertiesSet.begin();
		if (!validMovesSet.Contains(savingStonePlacement))
			continue;

		GoGameMatrix* trialMatrix = new GoGameMatrix(gameMatrix);
		if (trialMatrix->SetCellState(savingStonePlacement, this->favoredPlayer, nullptr))
		{
			GoGameMatrix::ConnectedRegion* favoredGroupAttemptedSave = trialMatrix->SenseConnectedRegion(*favoredGroup->membersSet.begin());
			if (favoredGroupAttemptedSave->libertiesSet.Num() > 1)	// No longer in atari?
			{
				int libertyIncrase = favoredGroupAttemptedSave->libertiesSet.Num() - 1;
				if (libertyIncrase > largestLibertyIncrease)
				{
					this->stonePlacement = savingStonePlacement;
					savingMoveMade = true;
				}
			}
		}
		delete trialMatrix;
	}

	for (GoGameMatrix::ConnectedRegion* favoredGroup : favoredGroupsInAtariArray)
		delete favoredGroup;

	return savingMoveMade;
}

// Consider this case...
//   XXX
//  X OX
//  XOO		(O has 3 liberties.)
//  XX
// If it were X's turn, then the best move is...
//   XXX
//  X OX
//  XOO		(O still has 3 liberties, but is now essentially dead.)
//  XX X
// ...because now the O group cannot be saved.  But if it was O's turn, the following function doesn't even recognize this case!
// If O went there instead, we would have...
//   XXX
//  X OX
//  XOO     (O liberties again don't change, but the group is now essentially connected to a 4th O.)
//  XX O
// ...which gives the original O group a chance to survive.  Essentially the two O groups are connected, because they can always be connected if threatened.
// It seems a better go AI would know what liberties of a group can be used to save it, but further, can recognize a case when no liberties of a group can
// be used to save it, but rather, know how to save the group using a kitty-corner move.  I would term these as dead or useless liberties.
bool GoGameIdiotAI::PreventFavoredGroupsFromGettingIntoAtari(const TSet<GoGameMatrix::CellLocation>& validMovesSet)
{
	// Look for a move the opponent can make that puts one or more favored groups in atari.
	// If one is found, does it help us to make that move instead?
	// TODO: Sometimes a kitty-corner move is best in saving a group.  Can we recognize that?
	//       The current logic will solidify the kitty-corner "connection" to the group if necessary.
	bool savingMoveMade = false;
	EGoGameCellState opponentPlayer = (this->favoredPlayer == EGoGameCellState::Black) ? EGoGameCellState::White : EGoGameCellState::Black;
	int favoredGroupsInAtari = this->gameState->GetCurrentMatrix()->CountGroupsInAtariForColor(this->favoredPlayer);
	for (GoGameMatrix::CellLocation cellLocation : validMovesSet)
	{
		this->gameState->PushMatrix(new GoGameMatrix(this->gameState->GetCurrentMatrix()));

		if (this->gameState->GetCurrentMatrix()->SetCellState(cellLocation, opponentPlayer, nullptr, true))
		{
			int atariIncrease = this->gameState->GetCurrentMatrix()->CountGroupsInAtariForColor(this->favoredPlayer) - favoredGroupsInAtari;
			if (atariIncrease > 0)
			{
				this->gameState->PushMatrix(new GoGameMatrix(this->gameState->GetCurrentMatrix()));

				if (this->gameState->GetCurrentMatrix()->SetCellState(cellLocation, this->favoredPlayer, nullptr))
				{
					GoGameMatrix::ConnectedRegion* group = this->gameState->GetCurrentMatrix()->SenseConnectedRegion(cellLocation);
					if (group->libertiesSet.Num() >= 3)
					{
						this->stonePlacement = cellLocation;
						savingMoveMade = true;
					}

					delete group;
				}

				delete this->gameState->PopMatrix();
			}
		}

		delete this->gameState->PopMatrix();

		if (savingMoveMade)
			break;
	}

	return savingMoveMade;
}

void GoGameIdiotAI::FindAllDuelClusters(GoGameMatrix* gameMatrix, TArray<DuelCluster*>& duelClusterArray)
{
	duelClusterArray.Reset();

	TSet<GoGameMatrix::CellLocation> coveredCellsSet;

	for (int i = 0; i < gameMatrix->GetMatrixSize(); i++)
	{
		for (int j = 0; j < gameMatrix->GetMatrixSize(); j++)
		{
			GoGameMatrix::CellLocation cellLocation(i, j);
			EGoGameCellState cellState;
			gameMatrix->GetCellState(cellLocation, cellState);
			if (cellState != EGoGameCellState::Empty && !coveredCellsSet.Contains(cellLocation))
			{
				DuelCluster* duelCluster = new DuelCluster();
				duelCluster->Generate(gameMatrix, cellLocation, this->favoredPlayer);
				duelClusterArray.Add(duelCluster);
				duelCluster->ForAllStones([&coveredCellsSet](const GoGameMatrix::CellLocation& duelCellLocation) {
					coveredCellsSet.Add(duelCellLocation);
				});
			}
		}
	}
}

bool GoGameIdiotAI::FightInDuelCluster(const TSet<GoGameMatrix::CellLocation>& validMovesSet)
{
	GoGameMatrix* gameMatrix = this->gameState->GetCurrentMatrix();
	EGoGameCellState opponentPlayer = (this->favoredPlayer == EGoGameCellState::Black) ? EGoGameCellState::White : EGoGameCellState::Black;

	TSet<GoGameMatrix::CellLocation> favoredTerritorySet;
	gameMatrix->FindAllTerritoryOfColor(this->favoredPlayer, favoredTerritorySet);

	TSet<GoGameMatrix::CellLocation> opponentTerritorySet;
	gameMatrix->FindAllTerritoryOfColor(opponentPlayer, opponentTerritorySet);

	TSet<GoGameMatrix::CellLocation> opponentImmortalStonesSet;
	gameMatrix->FindAllImmortalStonesOfColor(opponentPlayer, opponentImmortalStonesSet);
	UE_LOG(LogGoGameIdiotAI, Display, TEXT("Found %d immortal opponent stones!"), opponentImmortalStonesSet.Num());

	// A go game player must fight multiple battles on multiple fronts simultaneously.
	// This is my attempt to do so.  First, go identify all those fronts.
	TArray<DuelCluster*> duelClusterArray;
	this->FindAllDuelClusters(gameMatrix, duelClusterArray);
	if (duelClusterArray.Num() == 0)
	{
		// There aren't any.  This means we're the first to place a stone.  Do so reasonably.
		bool foundBest = this->ScoreAndSelectBestPlacement([](GoGameMatrix* givenGameMatrix, const GoGameMatrix::CellLocation& cellLocation) -> float {
			float distanceToEdge = givenGameMatrix->ShortestDistanceToBoardEdge(cellLocation);
			float distanceToCenter = givenGameMatrix->ShortestDistanceToBoardCenter(cellLocation);
			float score = distanceToEdge * distanceToCenter;
			return score;
		});
		check(foundBest);
		return true;
	}
	
	// Identify the front that needs our attention most.
	duelClusterArray.Sort([](const DuelCluster& clusterA, const DuelCluster& clusterB) -> bool {
		float ratioA = clusterA.TotalFavoredToOpponentStoneRatio();
		float ratioB = clusterB.TotalFavoredToOpponentStoneRatio();
		return ratioA < ratioB;
	});

	// Not sure how to detect if a cluster is a lost cause, but try to find the one that is most urgent.
	// I'm guessign this is the one earliest in the sorted list that contains an oponnent's stone.
	DuelCluster* mostUrgentDuelCluster = nullptr;
	for (int i = 0; i < duelClusterArray.Num() && !mostUrgentDuelCluster; i++)
	{
		DuelCluster* duelCluster = duelClusterArray[i];
		if (duelCluster->opponentGroupsArray.Num() > 0)
			mostUrgentDuelCluster = duelCluster;
	}
	check(mostUrgentDuelCluster);

	// An optimal liberty reduction placement is one that maximizes the empty spaces next to the placed stone.
	// At least, that's what I think generally and can't think of a counter-example, but it wouldn't surprise
	// me if there were several.  Anyhow, here is an example...
	//    X X X X
	//   XOOOOOOOX
	//    X . X X
	// Here, the best move for X is where the period is, because this move makes the O group essentially dead.
	// Other moves can do this too, but the point is clear, and notice that the X's preceding the period are
	// following the same rule.
	int largestEmptyCellCount = 0;
	for (int i = 0; i < mostUrgentDuelCluster->opponentGroupsArray.Num(); i++)
	{
		// Don't consider attacking any groups of the opponent that are immortal.
		GoGameMatrix::ConnectedRegion* opponentGroup = mostUrgentDuelCluster->opponentGroupsArray[i];
		TSet<GoGameMatrix::CellLocation> intersectionSet = opponentGroup->membersSet.Intersect(opponentImmortalStonesSet);
		if (intersectionSet.Num() > 0)
			continue;

		for (GoGameMatrix::CellLocation opponentLibertyCell : opponentGroup->libertiesSet)
		{
			if (!validMovesSet.Contains(opponentLibertyCell))
				continue;

			// This isn't quite right.  There are sometimes good reasons to play inside of territory.
			// For example, doing so might create two eyes to make one's group immortal.
			if (favoredTerritorySet.Contains(opponentLibertyCell))
				continue;

			// This isn't quite right either.  Anyhow, here we're estimating that the territory is too small to play within.
			if (opponentTerritorySet.Contains(opponentLibertyCell) && opponentTerritorySet.Num() < 10)
				continue;

			int emptyCellCount = 0;
			for (int j = 0; j < 4; j++)
			{
				GoGameMatrix::CellLocation adjCellLocation = opponentLibertyCell.GetAdjacentLocation(j);
				EGoGameCellState cellState;
				if (gameMatrix->GetCellState(adjCellLocation, cellState) && cellState == EGoGameCellState::Empty)
					emptyCellCount++;
			}
			check(emptyCellCount < 4);	// It wouldn't be a liberty otherwise.

			// Of course, there could be more than one cell location matching our criteria.
			// Choosing between them which is best is not obvious to me at all.
			if (emptyCellCount > largestEmptyCellCount)
			{
				largestEmptyCellCount = emptyCellCount;
				this->stonePlacement = opponentLibertyCell;
			}
		}
	}

	for (DuelCluster* duelCluster : duelClusterArray)
		delete duelCluster;

	return largestEmptyCellCount > 0;
}

GoGameIdiotAI::DuelCluster::DuelCluster()
{
}

/*virtual*/ GoGameIdiotAI::DuelCluster::~DuelCluster()
{
	this->Clear();
}

void GoGameIdiotAI::DuelCluster::Clear()
{
	for (GoGameMatrix::ConnectedRegion* group : this->favoredGroupsArray)
		delete group;

	for (GoGameMatrix::ConnectedRegion* group : this->opponentGroupsArray)
		delete group;

	this->favoredGroupsArray.Reset();
	this->opponentGroupsArray.Reset();
}

int GoGameIdiotAI::DuelCluster::GetTotalNumberOfFavoredStones() const
{
	int total = 0;
	for (GoGameMatrix::ConnectedRegion* group : this->favoredGroupsArray)
		total += group->membersSet.Num();
	return total;
}

int GoGameIdiotAI::DuelCluster::GetTotalNumberOfOpponentStones() const
{
	int total = 0;
	for (GoGameMatrix::ConnectedRegion* group : this->opponentGroupsArray)
		total += group->membersSet.Num();
	return total;
}

float GoGameIdiotAI::DuelCluster::TotalFavoredToOpponentStoneRatio() const
{
	float totalFavoredStones = this->GetTotalNumberOfFavoredStones();
	float totalOpponentStones = this->GetTotalNumberOfOpponentStones();
	
	if (totalOpponentStones == 0.0f)
		return TNumericLimits<float>::Max();

	return totalFavoredStones / totalOpponentStones;
}

void GoGameIdiotAI::DuelCluster::ForAllStones(TFunctionRef<void(const GoGameMatrix::CellLocation& cellLocation)> visitFunction)
{
	for (GoGameMatrix::ConnectedRegion* group : this->favoredGroupsArray)
		for (GoGameMatrix::CellLocation cellLocation : group->membersSet)
			visitFunction(cellLocation);

	for (GoGameMatrix::ConnectedRegion* group : this->opponentGroupsArray)
		for (GoGameMatrix::CellLocation cellLocation : group->membersSet)
			visitFunction(cellLocation);
}

void GoGameIdiotAI::DuelCluster::Generate(GoGameMatrix* gameMatrix, const GoGameMatrix::CellLocation& rootCell, EGoGameCellState favoredStone)
{
	this->Clear();

	TSet<GoGameMatrix::CellLocation> clusterCellSet;

	TSet<GoGameMatrix::CellLocation> cellQueue;
	cellQueue.Add(rootCell);

	while (cellQueue.Num() > 0)
	{
		TSet<GoGameMatrix::CellLocation>::TIterator iter = cellQueue.CreateIterator();
		GoGameMatrix::CellLocation cellLocation = *iter;
		cellQueue.Remove(*iter);

		EGoGameCellState cellState;
		if (gameMatrix->GetCellState(cellLocation, cellState) && cellState != EGoGameCellState::Empty)
		{
			clusterCellSet.Add(cellLocation);

			for (int i = 0; i < 4; i++)
			{
				GoGameMatrix::CellLocation adjCellLocation = cellLocation.GetAdjacentLocation(i);
				if(gameMatrix->IsInBounds(adjCellLocation) && !clusterCellSet.Contains(adjCellLocation) && !cellQueue.Contains(adjCellLocation))
					cellQueue.Add(adjCellLocation);

				GoGameMatrix::CellLocation kittyCellLocation = cellLocation.GetKittyCornerLocation(i);
				if (gameMatrix->IsInBounds(kittyCellLocation) && !clusterCellSet.Contains(kittyCellLocation) && !cellQueue.Contains(kittyCellLocation))
					cellQueue.Add(kittyCellLocation);
			}
		}
	}

	while (clusterCellSet.Num() > 0)
	{
		TSet<GoGameMatrix::CellLocation>::TIterator iter = clusterCellSet.CreateIterator();
		GoGameMatrix::CellLocation clusterCell = *iter;

		GoGameMatrix::ConnectedRegion* group = gameMatrix->SenseConnectedRegion(clusterCell);
		check(group && group->type == GoGameMatrix::ConnectedRegion::GROUP);

		if (group->owner == favoredStone)
			this->favoredGroupsArray.Add(group);
		else
			this->opponentGroupsArray.Add(group);

		for (GoGameMatrix::CellLocation cellLocation : group->membersSet)
			if (clusterCellSet.Contains(cellLocation))
				clusterCellSet.Remove(cellLocation);
	}

	this->favoredGroupsArray.Sort([](GoGameMatrix::ConnectedRegion& groupA, GoGameMatrix::ConnectedRegion& groupB) -> bool {
		return groupA.membersSet.Num() < groupB.membersSet.Num();
	});

	this->opponentGroupsArray.Sort([](GoGameMatrix::ConnectedRegion& groupA, GoGameMatrix::ConnectedRegion& groupB) -> bool {
		return groupA.membersSet.Num() < groupB.membersSet.Num();
	});
}