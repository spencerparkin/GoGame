#include "GoGameLevelScript.h"
#include "GoGameBoard.h"
#include "GoGameHUD.h"
#include "GoGameMatrix.h"
#include "GoGameState.h"
#include "GoGameAI.h"
#include "Kismet/GameplayStatics.h"

AGoGameLevelScript::AGoGameLevelScript()
{
	this->gameAI = new GoGameAIMinimax();
}

/*virtual*/ AGoGameLevelScript::~AGoGameLevelScript()
{
	delete gameAI;
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

void AGoGameLevelScript::LetComputerTakeTurn()
{
	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (gameState && gameState->GetCurrentMatrix())
	{
		GoGameMatrix::CellLocation calculatedMove = this->gameAI->CalculateStonePlacement(gameState);
		if (calculatedMove.i != -1 && calculatedMove.j != -1)
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

void AGoGameLevelScript::ForfeitTurn()
{
	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (gameState && gameState->GetCurrentMatrix())
	{
		GoGameMatrix::CellLocation cellLocation(TNumericLimits<int>::Max(), TNumericLimits<int>::Max());
		gameState->AlterGameState(cellLocation, gameState->GetCurrentMatrix()->GetWhoseTurn());
	}
}