#include "GoGameLevelScript.h"
#include "GoGameBoard.h"
#include "GoGameHUD.h"
#include "GoGameGroupMinimax.h"
#include "GoGameMatrix.h"
#include "GoGameState.h"
#include "GoGameAI.h"
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
	if (gameState && gameState->GetCurrentMatrix())
	{
		GoGameAI gameAI;
		GoGameMatrix::CellLocation calculatedMove = gameAI.CalculateStonePlacement(gameState);
		if (gameState->GetCurrentMatrix()->IsInBounds(calculatedMove))
			gameState->AlterGameState(calculatedMove, gameState->GetCurrentMatrix()->GetWhoseTurn());
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