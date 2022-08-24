#include "GoGameHUD.h"
#include "GoGameBoard.h"
#include "GoGameMode.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "GoGameOptions.h"
#include "GoGamePawn.h"
#include "Kismet/GameplayStatics.h"
#include "UMG/Public/Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"

#define LOCTEXT_NAMESPACE "GoGameHUDWidget"

UGoGameHUDWidget::UGoGameHUDWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

/*virtual*/ UGoGameHUDWidget::~UGoGameHUDWidget()
{
}

/*virtual*/ bool UGoGameHUDWidget::Initialize()
{
	if (!Super::Initialize())
		return false;

	return true;
}

BEGIN_FUNCTION_BUILD_OPTIMIZATION

FText UGoGameHUDWidget::CellStateToText(EGoGameCellState cellState)
{
	FText text;

	switch (cellState)
	{
		case EGoGameCellState::Empty:
		{
			text = FText::FromString("Empty");
			break;
		}
		case EGoGameCellState::Black:
		{
			text = FText::FromString("Black");
			break;
		}
		case EGoGameCellState::White:
		{
			text = FText::FromString("White");
			break;
		}
	}

	return text;
}

void UGoGameHUDWidget::OnBoardAppearanceChanged()
{
	AGoGameMode* gameMode = Cast<AGoGameMode>(UGameplayStatics::GetGameMode(this->GetWorld()));
	if (!gameMode)
		return;

	AGoGameState* gameState = Cast<AGoGameState>(gameMode->GameState);
	if (!gameState)
		return;

	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
	if (!gameMatrix)
		return;

	UTextBlock* textBlock = Cast<UTextBlock>(this->WidgetTree->FindWidget("GameScoreTextBlock"));
	if (textBlock)
	{
		int scoreDelta = 0;
		EGoGameCellState winner = gameMatrix->CalculateCurrentWinner(scoreDelta);
		if (winner == EGoGameCellState::Empty)
			textBlock->SetText(FText::FromString("It's a tie!"));
		else
		{
			FFormatNamedArguments namedArgs;
			namedArgs.Add("winner", this->CellStateToText(winner));
			namedArgs.Add("scoreDelta", scoreDelta);
			textBlock->SetText(FText::Format(FTextFormat::FromString("{winner} is winning by {scoreDelta} point(s)."), namedArgs));
		}
	}

	textBlock = Cast<UTextBlock>(this->WidgetTree->FindWidget("WhoseTurnTextBlock"));
	if (textBlock)
	{
		EGoGameCellState whoseTurn = gameMatrix->GetWhoseTurn();
		FFormatNamedArguments namedArgs;
		namedArgs.Add("whoseTurn", this->CellStateToText(whoseTurn));
		textBlock->SetText(FText::Format(FTextFormat::FromString("Waiting for player {whoseTurn} to place a stone."), namedArgs));
	}

	textBlock = Cast<UTextBlock>(this->WidgetTree->FindWidget("HoverHighlightTextBlock"));
	if (textBlock)
	{
		ESlateVisibility visibility = ESlateVisibility::Hidden;

		if (gameMode->gameOptions->showHoverHighlights)
		{
			TArray<AActor*> actorArray;
			UGameplayStatics::GetAllActorsOfClass(this->GetWorld(), AGoGamePawn::StaticClass(), actorArray);
			if (actorArray.Num() == 1)
			{
				AGoGamePawn* gamePawn = Cast<AGoGamePawn>(actorArray[0]);
				if (gamePawn && gamePawn->currentlySelectedRegion)
				{
					visibility = ESlateVisibility::Visible;

					if (gamePawn->currentlySelectedRegion->type == GoGameMatrix::ConnectedRegion::GROUP)
					{
						FFormatNamedArguments namedArgs;
						namedArgs.Add("groupSize", gamePawn->currentlySelectedRegion->membersSet.Num());
						namedArgs.Add("groupOwner", this->CellStateToText(gamePawn->currentlySelectedRegion->owner));
						if(gamePawn->currentlySelectedRegion->libertiesSet.Num() == 1)
							textBlock->SetText(FText::Format(FTextFormat::FromString("A group of {groupSize} stone(s) owned by {groupOwner} in atari!"), namedArgs));
						else
						{
							namedArgs.Add("libertyCount", gamePawn->currentlySelectedRegion->libertiesSet.Num());
							textBlock->SetText(FText::Format(FTextFormat::FromString("A group of {groupSize} stone(s) owned by {groupOwner} having {libertyCount} liberties."), namedArgs));
						}
					}
					else if (gamePawn->currentlySelectedRegion->type == GoGameMatrix::ConnectedRegion::TERRITORY)
					{
						FFormatNamedArguments namedArgs;
						namedArgs.Add("territorySize", gamePawn->currentlySelectedRegion->membersSet.Num());
						if (gamePawn->currentlySelectedRegion->owner == EGoGameCellState::Empty)
							textBlock->SetText(FText::Format(FTextFormat::FromString("Contested territory of size {territorySize}."), namedArgs));
						else
						{
							namedArgs.Add("territoryOwner", this->CellStateToText(gamePawn->currentlySelectedRegion->owner));
							textBlock->SetText(FText::Format(FTextFormat::FromString("Territory of size {territorySize} owned by player {territoryOwner}."), namedArgs));
						}
					}
				}
			}
		}

		textBlock->SetVisibility(visibility);
	}
}

END_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE