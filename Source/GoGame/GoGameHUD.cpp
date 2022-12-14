#include "GoGameHUD.h"
#include "GoGameBoard.h"
#include "GoGameMode.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "GoGameOptions.h"
#include "GoGamePawnHuman.h"
#include "GoGameModule.h"
#include "GoGamePlayerController.h"
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
	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (!gameState)
		return;

	GoGameMatrix* gameMatrix = gameState->GetCurrentMatrix();
	if (!gameMatrix)
		return;

	AGoGamePawn* gamePawn = Cast<AGoGamePawn>(UGameplayStatics::GetPlayerPawn(this->GetWorld(), 0));
	if (!gamePawn)
		return;

	UTextBlock* textBlock = Cast<UTextBlock>(this->WidgetTree->FindWidget("GameScoreTextBlock"));
	if (textBlock)
	{
		int scoreDelta = 0, blackTerritoryCount = 0, whiteTerritoryCount = 0;
		EGoGameCellState winner = gameMatrix->CalculateCurrentWinner(scoreDelta, blackTerritoryCount, whiteTerritoryCount);
		if (winner == EGoGameCellState::Empty)
			textBlock->SetText(FText::FromString("It's a tie!"));
		else
		{
			FFormatNamedArguments namedArgs;
			namedArgs.Add("winner", this->CellStateToText(winner));
			namedArgs.Add("scoreDelta", scoreDelta);
			namedArgs.Add("blackTerritory", blackTerritoryCount);
			namedArgs.Add("whiteTerritory", whiteTerritoryCount);
			namedArgs.Add("blackCaptures", gameMatrix->GetBlackCaptureCount());
			namedArgs.Add("whiteCaptures", gameMatrix->GetWhiteCaptureCount());
			textBlock->SetText(FText::Format(FTextFormat::FromString("{winner} is winning by {scoreDelta} point(s).  Back: {blackTerritory}/{blackCaptures}; White: {whiteTerritory}/{whiteCaptures}"), namedArgs));
		}
	}

	textBlock = Cast<UTextBlock>(this->WidgetTree->FindWidget("WhoseTurnTextBlock"));
	if (textBlock)
	{
		EGoGameCellState whoseTurn = gameMatrix->GetWhoseTurn();
		FFormatNamedArguments namedArgs;
		namedArgs.Add("myColor", this->CellStateToText(gamePawn->myColor));
		namedArgs.Add("whoseTurn", this->CellStateToText(whoseTurn));
		if (gamePawn->myColor == EGoGameCellState::Empty)
			textBlock->SetText(FText::Format(FTextFormat::FromString("You are a spectator. Waiting for player {whoseTurn} to place a stone."), namedArgs));
		else if (whoseTurn == gamePawn->myColor)
			textBlock->SetText(FText::Format(FTextFormat::FromString("You are player {myColor}. It's your turn!"), namedArgs));
		else
			textBlock->SetText(FText::Format(FTextFormat::FromString("You are player {myColor}. Waiting for player {whoseTurn} to place a stone."), namedArgs));
	}

	textBlock = Cast<UTextBlock>(this->WidgetTree->FindWidget("HoverHighlightTextBlock"));
	if (textBlock)
	{
		ESlateVisibility visibility = ESlateVisibility::Hidden;

		AGoGamePawnHuman* gamePawnHuman = Cast<AGoGamePawnHuman>(gamePawn);
		if (gamePawnHuman)
		{
			bool doHoverHighlights = false;
			GoGameModule* gameModule = (GoGameModule*)FModuleManager::Get().GetModule("GoGame");
			if (gameModule)
				doHoverHighlights = gameModule->gameOptions->showHoverHighlights;

			if (doHoverHighlights)
			{
				if (gamePawnHuman->currentlySelectedRegion)
				{
					visibility = ESlateVisibility::Visible;

					if (gamePawnHuman->currentlySelectedRegion->type == GoGameMatrix::ConnectedRegion::GROUP)
					{
						FFormatNamedArguments namedArgs;
						namedArgs.Add("groupSize", gamePawnHuman->currentlySelectedRegion->membersSet.Num());
						namedArgs.Add("groupOwner", this->CellStateToText(gamePawnHuman->currentlySelectedRegion->owner));
						if (gamePawnHuman->currentlySelectedRegion->libertiesSet.Num() == 1)
							textBlock->SetText(FText::Format(FTextFormat::FromString("A group of {groupSize} stone(s) owned by {groupOwner} in atari!"), namedArgs));
						else
						{
							namedArgs.Add("libertyCount", gamePawnHuman->currentlySelectedRegion->libertiesSet.Num());
							textBlock->SetText(FText::Format(FTextFormat::FromString("A group of {groupSize} stone(s) owned by {groupOwner} having {libertyCount} liberties."), namedArgs));
						}
					}
					else if (gamePawnHuman->currentlySelectedRegion->type == GoGameMatrix::ConnectedRegion::TERRITORY)
					{
						FFormatNamedArguments namedArgs;
						namedArgs.Add("territorySize", gamePawnHuman->currentlySelectedRegion->membersSet.Num());
						if (gamePawnHuman->currentlySelectedRegion->owner == EGoGameCellState::Empty)
							textBlock->SetText(FText::Format(FTextFormat::FromString("Contested territory of size {territorySize}."), namedArgs));
						else
						{
							namedArgs.Add("territoryOwner", this->CellStateToText(gamePawnHuman->currentlySelectedRegion->owner));
							textBlock->SetText(FText::Format(FTextFormat::FromString("Territory of size {territorySize} owned by player {territoryOwner}."), namedArgs));
						}
					}
				}
			}
		}

		textBlock->SetVisibility(visibility);
	}

	textBlock = Cast<UTextBlock>(this->WidgetTree->FindWidget("GameOverTextBlock"));
	if (textBlock)
	{
		ESlateVisibility visibility = gameMatrix->IsGameOver() ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
		textBlock->SetVisibility(visibility);
	}
}

END_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE