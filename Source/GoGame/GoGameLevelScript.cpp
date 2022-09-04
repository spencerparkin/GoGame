#include "GoGameLevelScript.h"
#include "GoGameBoard.h"
#include "GoGameHUD.h"
#include "GoGamePawnHuman.h"
#include "GoGamePawnAI.h"
#include "GoGameMode.h"
#include "GoGamePlayerController.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGameLevelScript, Log, All);

AGoGameLevelScript::AGoGameLevelScript()
{
	this->PrimaryActorTick.bCanEverTick = true;
	this->gamePawnHumanBlack = nullptr;
	this->gamePawnHumanWhite = nullptr;
	this->gamePawnAI = nullptr;
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

BEGIN_FUNCTION_BUILD_OPTIMIZATION

/*virtual*/ void AGoGameLevelScript::BeginPlay()
{
	Super::BeginPlay();

	FActorSpawnParameters spawnInfo;
	spawnInfo.Instigator = this->GetInstigator();	// TODO: I have no idea what this is.  Find out.
	spawnInfo.ObjectFlags |= RF_Transient;	// This means we don't want the pawn to presist in the map when we travel away from it to another map/level, I think.

	FTransform spawnTransform;
	spawnTransform.SetIdentity();

	this->gamePawnHumanBlack = Cast<AGoGamePawnHuman>(this->GetWorld()->SpawnActor<AGoGamePawnHuman>(AGoGamePawnHuman::StaticClass(), spawnTransform, spawnInfo));
	this->gamePawnHumanBlack->myColor = EGoGameCellState::Black;

	this->gamePawnHumanWhite = Cast<AGoGamePawnHuman>(this->GetWorld()->SpawnActor<AGoGamePawnHuman>(AGoGamePawnHuman::StaticClass(), spawnTransform, spawnInfo));
	this->gamePawnHumanWhite->myColor = EGoGameCellState::White;

	if (UKismetSystemLibrary::IsStandalone(this->GetWorld()))
	{
		APlayerController* playerController = UGameplayStatics::GetPlayerController(this->GetWorld(), 0);
		if (playerController)
		{
			playerController->Possess(this->gamePawnHumanBlack);	// TODO: Maybe the main-menu can let the user specify which they'd like to be.  For now, just always be black.

			this->gamePawnAI = Cast<AGoGamePawnAI>(this->GetWorld()->SpawnActor<AGoGamePawnAI>(AGoGamePawnAI::StaticClass(), spawnTransform, spawnInfo));
			this->gamePawnAI->myColor = EGoGameCellState::White;	// TODO: Make sure we're not the same color as the human player.  For now, it is safe to always choose white.
		}
	}
}

/*virtual*/ void AGoGameLevelScript::Tick(float DeltaTime)
{
	if (this->HasAuthority() && !UKismetSystemLibrary::IsStandalone(this->GetWorld()))
	{
		EGoGameCellState color = EGoGameCellState::Black;

		TArray<AGoGamePlayerController*> playerControllerArray;
		for (FConstPlayerControllerIterator iter = this->GetWorld()->GetPlayerControllerIterator(); iter; ++iter)
		{
			AGoGamePlayerController* playerController = Cast<AGoGamePlayerController>(iter->Get());
			if (playerController && playerController->NetConnection && playerController->NetConnection->GetConnectionState() == EConnectionState::USOCK_Open)
			{
				AGoGamePawnHuman* gamePawnHuman = Cast<AGoGamePawnHuman>(playerController->GetPawn());
				if (!gamePawnHuman)
				{
					if (!this->gamePawnHumanBlack->GetPlayerState())
						playerController->Possess(this->gamePawnHumanBlack);
					else if (!this->gamePawnHumanWhite->GetPlayerState())
						playerController->Possess(this->gamePawnHumanWhite);
				}
			}
		}
	}
}

END_FUNCTION_BUILD_OPTIMIZATION