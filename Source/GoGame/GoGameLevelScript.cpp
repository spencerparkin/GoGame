#include "GoGameLevelScript.h"
#include "GoGameBoard.h"
#include "GoGameHUD.h"
#include "GoGamePawnHuman.h"
#include "GoGamePawnAI.h"
#include "GoGameMode.h"
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

/*virtual*/ void AGoGameLevelScript::BeginPlay()
{
	Super::BeginPlay();

	AGoGameMode* gameMode = Cast<AGoGameMode>(UGameplayStatics::GetGameMode(this->GetWorld()));
	if (gameMode)
	{
		FActorSpawnParameters spawnInfo;
		spawnInfo.Instigator = this->GetInstigator();	// TODO: I have no idea what this is.  Find out.
		spawnInfo.ObjectFlags |= RF_Transient;	// This means we don't want the pawn to presist in the map when we travel away from it to another map/level, I think.

		FTransform spawnTransform;
		spawnTransform.SetIdentity();

		// TODO: Maybe let user pick which color they want to be in the main-menu.  This presently doesn't work for networked mode, though, because the server decides.

		AGoGamePawnHuman* gamePawnHuman = Cast<AGoGamePawnHuman>(this->GetWorld()->SpawnActor<AGoGamePawnHuman>(AGoGamePawnHuman::StaticClass(), spawnTransform, spawnInfo));
		gamePawnHuman->myColor = UGameplayStatics::HasOption(gameMode->OptionsString, "StandaloneMode") ? EGoGameCellState::Black : EGoGameCellState::Empty;

		if (UGameplayStatics::HasOption(gameMode->OptionsString, "StandaloneMode"))
		{
			AGoGamePawnAI* gamePawnAI = Cast<AGoGamePawnAI>(this->GetWorld()->SpawnActor<AGoGamePawnAI>(AGoGamePawnAI::StaticClass(), spawnTransform, spawnInfo));
			gamePawnAI->myColor = EGoGameCellState::White;
		}

		APlayerController* playerController = UGameplayStatics::GetPlayerController(this->GetWorld(), 0);
		if (playerController)
			playerController->Possess(gamePawnHuman);
	}
}