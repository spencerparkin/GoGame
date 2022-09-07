#include "GoGameIdiotAI.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "Kismet/GameplayStatics.h"

GoGameIdiotAI::GoGameIdiotAI(EGoGameCellState favoredColor)
{
	this->favoredPlayer = favoredColor;
}

/*virtual*/ GoGameIdiotAI::~GoGameIdiotAI()
{
}

// To goal in go is to capture the most territory.  The goal here, however, is to just keep from losing by a landslide.
// To capture territory, one usually has to try to think about forming structure on the board.  I have no idea how to program that here.
// Rather, my goal here is to just attack the player at every opportunity.  I'm sure that the result will still be a very
// mediocre go player AI, but I can at least have fun with this.
bool GoGameIdiotAI::ScoreAndSelectBestPlacement(AGoGameState* gameState, TFunctionRef<float(GoGameMatrix* gameMatrix, const GoGameMatrix::CellLocation& cellLocation)> scoreFunction, GoGameMatrix::CellLocation& stonePlacement)
{
	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
	GoGameMatrix* forbiddenMatrix = gameState->GetForbiddenMatrix();
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
	stonePlacement = candidateArray[0].cellLocation;
	return true;
}

bool GoGameIdiotAI::CalculateStonePlacement(AGoGameState* gameState, GoGameMatrix::CellLocation& stonePlacement)
{
	if (this->favoredPlayer == EGoGameCellState::Empty)
		return false;

	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
	if (!gameMatrix)
		return false;

	if (!gameMatrix || gameMatrix->GetWhoseTurn() != this->favoredPlayer)
		return false;

	GoGameMatrix* forbiddenMatrix = gameState->GetForbiddenMatrix();
	TSet<GoGameMatrix::CellLocation> validMovesSet;
	gameMatrix->GenerateAllPossiblePlacements(validMovesSet, forbiddenMatrix);

	if (this->CaptureOpponentGroupsInAtari(gameState, stonePlacement, validMovesSet))
		return true;

	// Note that we try to put an opponent's group in atari before saving ourselves from atari.
	// This can work as a defense as long as a favored group in atari isn't needed to keep
	// the said opponent's group in atari.  Even so, it still might be a bad idea to go on
	// the offensive instead of the defensive, because our guarenteed capture may not be
	// anywhere near as substantial as the opponent's guarenteed capture.
	if (this->PutOpponentGroupsInAtari(gameState, stonePlacement, validMovesSet))
		return true;

	if (this->SaveFavoredAtariGroupsFromCapture(gameState, stonePlacement, validMovesSet))
		return true;

	if (this->PreventFavoredGroupsFromGettingIntoAtari(gameState, stonePlacement, validMovesSet))
		return true;

	/*
	TODO: Make sure we don't play inside territory generally.  You can if it's big enough, but usually it's best not to do so.

	TSet<GoGameMatrix::CellLocation> favoredTerritorySet;
	gameMatrix->FindAllTerritoryOfColor(this->favoredPlayer, favoredTerritorySet);

	TSet<GoGameMatrix::CellLocation> opponentTerritorySet;
	gameMatrix->FindAllTerritoryOfColor(opponentPlayer, opponentTerritorySet);
	*/
	
	// A go game player must fight multiple battles on multiple fronts simultaneously.
	// This is my attempt to so do.  First, go identify all those fronts.
	TArray<DuelCluster*> duelClusterArray;
	this->FindAllDuelClusters(gameMatrix, duelClusterArray);
	if (duelClusterArray.Num() == 0)
	{
		// There aren't any.  This means we're the first to place a stone.  Do so reasonably.
		bool foundBest = this->ScoreAndSelectBestPlacement(gameState, [](GoGameMatrix* givenGameMatrix, const GoGameMatrix::CellLocation& cellLocation) -> float {
			float distanceToEdge = givenGameMatrix->ShortestDistanceToBoardEdge(cellLocation);
			float distanceToCenter = givenGameMatrix->ShortestDistanceToBoardCenter(cellLocation);
			float score = distanceToEdge * distanceToCenter;
			return score;
		}, stonePlacement);
		check(foundBest);
		return true;
	}
	else
	{
		// Identify the front that needs our attention most.
		duelClusterArray.Sort([](const DuelCluster& clusterA, const DuelCluster& clusterB) -> bool {
			float ratioA = clusterA.TotalFavoredToOpponentStoneRatio();
			float ratioB = clusterB.TotalFavoredToOpponentStoneRatio();
			return ratioA < ratioB;
		});

		// For now, just choose the first in our sorted list.  We should select more carefully, though,
		// because some clusters may have become a lost cause.
		//...

		// TODO: Look for a group in the cluster to attack.  A best liberty reduction
		//       placement is one that maximizes the empty spaces next to the placed stone.
		//       At least, that's what I think generally and can't think of a counter-example,
		//       but it wouldn't surprise me if there were several.

		for (DuelCluster* duelCluster : duelClusterArray)
			delete duelCluster;

		return true;
	}

	// If all else fails, we pass.
	stonePlacement.i = TNumericLimits<int>::Max();
	stonePlacement.j = TNumericLimits<int>::Max();
	return true;
}

// TODO: Note that sometimes a single stone placement can capture multiple groups, but I'm not checking for that here; I should.
bool GoGameIdiotAI::CaptureOpponentGroupsInAtari(AGoGameState* gameState, GoGameMatrix::CellLocation& stonePlacement, const TSet<GoGameMatrix::CellLocation>& validMovesSet)
{
	// Go find all opponent groups in atari.
	bool captureMoveMade = false;
	EGoGameCellState opponentPlayer = (this->favoredPlayer == EGoGameCellState::Black) ? EGoGameCellState::White : EGoGameCellState::Black;
	TArray<GoGameMatrix::ConnectedRegion*> opponentGroupArray;
	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
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
				// TODO: Check that here.
			}

			// Okay, if we cannot defer the capture, make the capture now.
			if (!deferCapture)
			{
				stonePlacement = capturePlacement;
				if (validMovesSet.Contains(stonePlacement))
					captureMoveMade = true;
			}
		}
	}

	for (GoGameMatrix::ConnectedRegion* opponentGroup : opponentGroupArray)
		delete opponentGroup;

	return captureMoveMade;
}

bool GoGameIdiotAI::PutOpponentGroupsInAtari(AGoGameState* gameState, GoGameMatrix::CellLocation& stonePlacement, const TSet<GoGameMatrix::CellLocation>& validMovesSet)
{
	// Look for a move that puts one or more opponent groups in atari.  Favor those, if any, that maximize the opponent's overall atari state.
	EGoGameCellState opponentPlayer = (this->favoredPlayer == EGoGameCellState::Black) ? EGoGameCellState::White : EGoGameCellState::Black;
	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
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
			stonePlacement = cellLocation;
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
bool GoGameIdiotAI::SaveFavoredAtariGroupsFromCapture(AGoGameState* gameState, GoGameMatrix::CellLocation& stonePlacement, const TSet<GoGameMatrix::CellLocation>& validMovesSet)
{
	// Go find favored groups in atari.
	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
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
		if (!validMovesSet.Contains(stonePlacement))
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
					stonePlacement = savingStonePlacement;
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

bool GoGameIdiotAI::PreventFavoredGroupsFromGettingIntoAtari(AGoGameState* gameState, GoGameMatrix::CellLocation& stonePlacement, const TSet<GoGameMatrix::CellLocation>& validMovesSet)
{
	// Look for a move the opponent can make that puts one or more favored groups in atari.
	// If one is found, does it help us to make that move instead?
	bool savingMoveMade = false;
	EGoGameCellState opponentPlayer = (this->favoredPlayer == EGoGameCellState::Black) ? EGoGameCellState::White : EGoGameCellState::Black;
	int favoredGroupsInAtari = gameState->GetCurrentMatrix()->CountGroupsInAtariForColor(this->favoredPlayer);
	for (GoGameMatrix::CellLocation cellLocation : validMovesSet)
	{
		gameState->PushMatrix(new GoGameMatrix(gameState->GetCurrentMatrix()));

		if (gameState->GetCurrentMatrix()->SetCellState(cellLocation, opponentPlayer, nullptr, true))
		{
			int atariIncrease = gameState->GetCurrentMatrix()->CountGroupsInAtariForColor(this->favoredPlayer) - favoredGroupsInAtari;
			if (atariIncrease > 0)
			{
				gameState->PushMatrix(new GoGameMatrix(gameState->GetCurrentMatrix()));

				if (gameState->GetCurrentMatrix()->SetCellState(cellLocation, this->favoredPlayer, nullptr))
				{
					GoGameMatrix::ConnectedRegion* group = gameState->GetCurrentMatrix()->SenseConnectedRegion(cellLocation);
					if (group->libertiesSet.Num() >= 3)
					{
						stonePlacement = cellLocation;
						savingMoveMade = true;
					}

					delete group;
				}

				delete gameState->PopMatrix();
			}
		}

		delete gameState->PopMatrix();

		if (savingMoveMade)
			break;
	}

	return savingMoveMade;
}

void GoGameIdiotAI::FindAllDuelClusters(GoGameMatrix* gameMatrix, TArray<DuelCluster*>& duelClusterArray)
{
	duelClusterArray.Reset();

	//...
}

GoGameIdiotAI::DuelCluster::DuelCluster()
{
}

/*virtual*/ GoGameIdiotAI::DuelCluster::~DuelCluster()
{
	for (GoGameMatrix::ConnectedRegion* group : this->favoredGroupsArray)
		delete group;

	for (GoGameMatrix::ConnectedRegion* group : this->opponentGroupsArray)
		delete group;
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