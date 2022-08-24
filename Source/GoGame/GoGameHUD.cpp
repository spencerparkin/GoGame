#include "GoGameHUD.h"
#include "GoGameBoard.h"
#include "GoGameMode.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
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

void UGoGameHUDWidget::OnGameStateChanged()
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
		{
			textBlock->SetText(FText::FromString("It's a tie!"));
		}
		else
		{
			FFormatNamedArguments namedArgs;
			if (winner == EGoGameCellState::Black)
				namedArgs.Add("winner", FText::FromString("Black"));
			else
				namedArgs.Add("winner", FText::FromString("White"));

			namedArgs.Add("scoreDelta", scoreDelta);

			textBlock->SetText(FText::Format(FTextFormat::FromString("{winner} is winning by {scoreDelta} point(s)."), namedArgs));
		}
	}

	textBlock = Cast<UTextBlock>(this->WidgetTree->FindWidget("WhoseTurnTextBlock"));
	if (textBlock)
	{
		EGoGameCellState whoseTurn = gameMatrix->GetWhoseTurn();

		FFormatNamedArguments namedArgs;
		if (whoseTurn == EGoGameCellState::Black)
			namedArgs.Add("whoseTurn", FText::FromString("Black"));
		else
			namedArgs.Add("whoseTurn", FText::FromString("White"));

		textBlock->SetText(FText::Format(FTextFormat::FromString("Waiting for player {whoseTurn} to place a stone."), namedArgs));
	}
}

END_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE