#include "GoGameLevelScript.h"
#include "GoGameBoard.h"
#include "GoGameHUD.h"
#include "Kismet/GameplayStatics.h"

AGoGameLevelScript::AGoGameLevelScript()
{
}

/*virtual*/ AGoGameLevelScript::~AGoGameLevelScript()
{
}

void AGoGameLevelScript::SetupHUD()
{
	if (!IsRunningDedicatedServer())
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