#include "GoGameLevelScript.h"
#include "GoGameBoard.h"
#include "GoGameHUD.h"
#include "GoGameGroupMinimax.h"
#include "GoGameMatrix.h"
#include "GoGameState.h"
#include "Kismet/GameplayStatics.h"

AGoGameLevelScript::AGoGameLevelScript()
{
}

/*virtual*/ AGoGameLevelScript::~AGoGameLevelScript()
{
}

void AGoGameLevelScript::SetupHUD()
{
	if (!this->HasAuthority() || UKismetSystemLibrary::IsStandalone(this->GetWorld()))
	{
		UClass* hudClass = ::StaticLoadClass(UObject::StaticClass(), GetTransientPackage(), TEXT("WidgetBlueprint'/Game/GameHUD/GameHUD.GameHUD_C'"));
		if (hudClass)
		{
			UGoGameHUDWidget* hudWidget = Cast<UGoGameHUDWidget>(UUserWidget::CreateWidgetInstance(*this->GetWorld(), hudClass, TEXT("GoGameHUDWidget")));
			if (hudWidget)
			{
				TArray<AActor*> actorArray;
				UGameplayStatics::GetAllActorsOfClass(this->GetWorld(), AGoGameBoard::StaticClass(), actorArray);
				if (actorArray.Num() == 1)
				{
					AGoGameBoard* gameBoard = Cast<AGoGameBoard>(actorArray[0]);
					if (gameBoard)
						gameBoard->OnBoardAppearanceChanged.AddDynamic(hudWidget, &UGoGameHUDWidget::OnBoardAppearanceChanged);
				}

				hudWidget->AddToPlayerScreen(0);
			}
		}
	}
}

// For now, just do something simple for testing purposes.
// This logic is single-minded about going on the offensive.
// It doesn't yet ever take a defensive stance when needed.
// More importantly, it doesn't try to capture territory in the long game.  I have no idea how to program that.
void AGoGameLevelScript::LetComputerTakeTurn()
{
	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (gameState)
	{
		GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
		EGoGameCellState favoredPlayer = gameMatrix->GetWhoseTurn();
		EGoGameCellState opposingPlayer = (favoredPlayer == EGoGameCellState::Black) ? EGoGameCellState::White : EGoGameCellState::Black;

		// Collect all the opponants groups.
		TArray<GoGameMatrix::ConnectedRegion*> groupArray;
		gameMatrix->CollectAllRegionsOfType(opposingPlayer, groupArray);

		// Sort them by liberties, smallest to largest.
		groupArray.Sort([](const GoGameMatrix::ConnectedRegion& groupA, const GoGameMatrix::ConnectedRegion& groupB) -> bool {
			return groupA.libertiesSet.Num() < groupB.libertiesSet.Num();
		});

		for (int i = 0; i < groupArray.Num(); i++)
		{
			// Try to reduce this group's liberties.
			GoGameMatrix::ConnectedRegion* group = groupArray[i];
			GoGameMatrix::CellLocation targetGroupRep = *group->membersSet.begin();

			// TODO: It would be very useful to be able to detect whether the group was immortal or had mutual life with another group.
			//       It probably wouldn't be too hard to write an algorithm to figure this out.  We should not waste our time with groups
			//       that are immortal.  One way to do it is simply two turn on a mode where it is always one player's turn, then try
			//       to use the SetCellState function on all the liberties of the group in question.  Pop the matrix when done and turn
			//       off the mode.  That doesn't account for the mutual-life case, though.  I suppose you could just fill up the entire
			//       board and then see what opponent's groups are left standing.  Those would be the immortal groups.

			float lookAheadDepth = 4;
			GoGameGroupMinimax groupMinimax(lookAheadDepth, favoredPlayer);

			GoGameMatrix::CellLocation bestNextMove;
			if (groupMinimax.CalculateBestNextMove(gameState, targetGroupRep, bestNextMove))
			{
				bool altered = gameState->AlterGameState(bestNextMove, favoredPlayer);
				if (altered)
					break;
			}
		}
		
		for (int i = 0; i < groupArray.Num(); i++)
			delete groupArray[i];
	}
}

void AGoGameLevelScript::UndoLastMove()
{
	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (gameState && gameState->GetCurrentMatrix())
	{
		GoGameMatrix::CellLocation cellLocation(-1, -1);
		gameState->AlterGameState(cellLocation, gameState->GetCurrentMatrix()->GetWhoseTurn());
	}
}