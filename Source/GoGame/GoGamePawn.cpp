// Fill out your copyright notice in the Description page of Project Settings.

#include "GoGamePawn.h"
#include "GoGameBoard.h"
#include "GoGameMisc.h"
#include "GoGameMatrix.h"
#include "GoGameMode.h"
#include "GoGameOptions.h"
#include "GoGameModule.h"
#include "GoGameState.h"
#include "GoGamePlayerController.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGamePawn, Log, All);

AGoGamePawn::AGoGamePawn()
{
	this->bReplicates = true;
	this->PrimaryActorTick.bCanEverTick = true;
	//this->AutoPossessPlayer = EAutoReceiveInput::Player0;		<-- This line of code breaks all of multiplayer!  Don't do it!

	//this->InputComponent = CreateDefaultSubobject<UInputComponent>(TEXT("PawnInputComponent"));

	this->gameBoard = nullptr;

	this->currentlySelectedRegion = nullptr;

	this->rotationRate = FRotator(0.0f, 0.0f, 0.0f);
	this->rotationRateDelta = FRotator(0.0f, 0.0f, 0.0f);
	this->maxRotationRate = FRotator(3.0f, 3.0f, 3.0f);
	this->minRotationRate = FRotator(-3.0f, -3.0f, -3.0f);
	this->rotationRateDrag = FRotator(30.0f, 30.0f, 30.0f);
}

/*virtual*/ AGoGamePawn::~AGoGamePawn()
{
}

void AGoGamePawn::BeginPlay()
{
	Super::BeginPlay();

	if (!this->gameBoard)
		this->gameBoard = Cast<AGoGameBoard>(UGameplayStatics::GetActorOfClass(this->GetWorld(), AGoGameBoard::StaticClass()));
}

void AGoGamePawn::ExitGame()
{
	FPlatformMisc::RequestExit(true);
}

void AGoGamePawn::MoveBoardLeftPressed()
{
	this->rotationRateDelta.Yaw += 30.0f;
}

void AGoGamePawn::MoveBoardLeftReleased()
{
	this->rotationRateDelta.Yaw -= 30.0f;
}

void AGoGamePawn::MoveBoardRightPressed()
{
	this->rotationRateDelta.Yaw -= 30.0f;
}

void AGoGamePawn::MoveBoardRightReleased()
{
	this->rotationRateDelta.Yaw += 30.0f;
}

void AGoGamePawn::MoveBoardUpPressed()
{
	this->rotationRateDelta.Pitch += 30.0f;
}

void AGoGamePawn::MoveBoardUpReleased()
{
	this->rotationRateDelta.Pitch -= 30.0f;
}

void AGoGamePawn::MoveBoardDownPressed()
{
	this->rotationRateDelta.Pitch -= 30.0f;
}

void AGoGamePawn::MoveBoardDownReleased()
{
	this->rotationRateDelta.Pitch += 30.0f;
}

BEGIN_FUNCTION_BUILD_OPTIMIZATION

void AGoGamePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (this->gameBoard)
	{
		this->rotationRate += this->rotationRateDelta;

		this->rotationRate.Yaw = FMath::Clamp(this->rotationRate.Yaw, this->minRotationRate.Yaw, this->maxRotationRate.Yaw);
		this->rotationRate.Pitch = FMath::Clamp(this->rotationRate.Pitch, this->minRotationRate.Pitch, this->maxRotationRate.Pitch);
		this->rotationRate.Roll = FMath::Clamp(this->rotationRate.Roll, this->minRotationRate.Roll, this->maxRotationRate.Roll);

		if (this->rotationRateDelta.Yaw == 0.0f)
			this->rotationRate.Yaw = GoGameMisc::Approach(this->rotationRate.Yaw, 0.0f, this->rotationRateDrag.Yaw * DeltaTime);

		if (this->rotationRateDelta.Pitch == 0.0f)
			this->rotationRate.Pitch = GoGameMisc::Approach(this->rotationRate.Pitch, 0.0f, this->rotationRateDrag.Pitch * DeltaTime);

		if (this->rotationRateDelta.Roll == 0.0f)
			this->rotationRate.Roll = GoGameMisc::Approach(this->rotationRate.Roll, 0.0f, this->rotationRateDrag.Roll * DeltaTime);

		FQuat quat = this->gameBoard->GetActorRotation().Quaternion();

		FVector pitchAxis(0.0f, 1.0f, 0.0f);
		float pitchAngle = this->rotationRate.Pitch * DeltaTime;
		FQuat pitchQuat(pitchAxis, pitchAngle);

		FVector yawAxis = quat.RotateVector(FVector(0.0f, 0.0f, 1.0f));
		float yawAngle = this->rotationRate.Yaw * DeltaTime;
		FQuat yawQuat(yawAxis, yawAngle);

		quat = pitchQuat * yawQuat * quat;

		this->gameBoard->SetActorRotation(quat.Rotator());

		bool doHoverHighlights = false;
		GoGameModule* gameModule = (GoGameModule*)FModuleManager::Get().GetModule("GoGame");
		if (gameModule)
			doHoverHighlights = gameModule->gameOptions->showHoverHighlights;

		if (doHoverHighlights)
		{
			APlayerController* playerController = Cast<APlayerController>(this->GetController());
			if (playerController)
			{
				GoGameMatrix::ConnectedRegion* newlySelectedRegion = nullptr;

				FVector location, direction;
				playerController->DeprojectMousePositionToWorld(location, direction);

				FVector traceStart = location;
				FVector traceEnd = location + direction * 1000.0f;

				FHitResult hitResult;
				this->GetWorld()->LineTraceSingleByChannel(hitResult, traceStart, traceEnd, ECC_Visibility);

				if (hitResult.GetHitObjectHandle().IsValid())
				{
					AGoGameBoardPiece* boardPiece = hitResult.GetHitObjectHandle().FetchActor<AGoGameBoardPiece>();
					if (boardPiece)
					{
						GoGameMatrix* gameMatrix = this->gameBoard->GetCurrentMatrix();
						if (gameMatrix)
							newlySelectedRegion = gameMatrix->SenseConnectedRegion(boardPiece->cellLocation);
					}
				}

				bool changeSelection = false;

				if (newlySelectedRegion && !this->currentlySelectedRegion)
					changeSelection = true;
				else if (!newlySelectedRegion && this->currentlySelectedRegion)
					changeSelection = true;
				else if (newlySelectedRegion && this->currentlySelectedRegion)
				{
					TSet<GoGameMatrix::CellLocation> intersectionSet = this->currentlySelectedRegion->membersSet.Intersect(newlySelectedRegion->membersSet);
					if (intersectionSet.Num() == 0)
						changeSelection = true;
				}

				if (!changeSelection)
					delete newlySelectedRegion;
				else
				{
					// TODO: There is a bug here where the highlight is not complete when a stone is first placed.  Fix it.

					if (this->currentlySelectedRegion)
					{
						this->SetHighlightOfCurrentlySelectedRegion(false);

						delete this->currentlySelectedRegion;
					}

					this->currentlySelectedRegion = newlySelectedRegion;

					if (this->currentlySelectedRegion)
					{
						this->SetHighlightOfCurrentlySelectedRegion(true);
					}

					this->gameBoard->OnBoardAppearanceChanged.Broadcast();
				}
			}
		}
	}
}

void AGoGamePawn::SetHighlightOfCurrentlySelectedRegion(bool highlighted)
{
	TArray<AGoGameBoardPiece*> boardPieceArray;

	if (this->currentlySelectedRegion->type == GoGameMatrix::ConnectedRegion::GROUP ||
		(this->currentlySelectedRegion->type == GoGameMatrix::ConnectedRegion::TERRITORY &&
			this->currentlySelectedRegion->owner != EGoGameCellState::Empty))	// The absense of a highlight makes it easy to identify contested territory.
	{
		this->gameBoard->GatherPieces(this->currentlySelectedRegion->membersSet, boardPieceArray);
		for (int i = 0; i < boardPieceArray.Num(); i++)
			boardPieceArray[i]->highlighted = highlighted;
	}

	if (this->currentlySelectedRegion->type == GoGameMatrix::ConnectedRegion::GROUP)
	{
		this->gameBoard->GatherPieces(this->currentlySelectedRegion->libertiesSet, boardPieceArray);
		for (int i = 0; i < boardPieceArray.Num(); i++)
			boardPieceArray[i]->highlighted = highlighted;
	}
}

// Client called, server run.
void AGoGamePawn::RequestSetup_Implementation()
{
	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (gameState)
	{
		UE_LOG(LogGoGamePawn, Log, TEXT("Replicating game state for client!"));
		this->ResetBoard(gameState->GetCurrentMatrix()->GetMatrixSize());

		// Replay the whole game history for the client.
		UE_LOG(LogGoGamePawn, Log, TEXT("Sending %d stone placements..."), gameState->placementHistory.Num());
		for (int i = 0; i < gameState->placementHistory.Num(); i++)
		{
			const GoGameMatrix::CellLocation& cellLocation = gameState->placementHistory[i];
			this->AlterGameState_OwningClient(cellLocation.i, cellLocation.j);
		}

		// Look at existing players to see what color the client should be.  Empty means a spectator.
		EGoGameCellState color = EGoGameCellState::Black;
		AGoGamePlayerController* playerController = Cast<AGoGamePlayerController>(this->Owner);
		if (playerController)
		{
			for (FConstPlayerControllerIterator iter = this->GetWorld()->GetPlayerControllerIterator(); iter; ++iter)
			{
				AGoGamePlayerController* existingPlayerController = Cast<AGoGamePlayerController>(iter->Get());
				if (existingPlayerController && existingPlayerController != playerController)
				{
					// TODO: There is a race condition here that could result in two players getting the same color.  Hmmm...  How do we fix it?
					//       One idea is to make the color a replicated value on the pawn.  The server's tick could scan all connected player controller's
					//       pawns and make sure they're all assigned a unique color or set as spectator.  Note that the replicated value could have a
					//       on-changed callback attached to it that fires the appearanced changed signal.
					if (existingPlayerController->NetConnection && existingPlayerController->NetConnection->GetConnectionState() == EConnectionState::USOCK_Open)
					{
						if (existingPlayerController->myColor == color)
						{
							UE_LOG(LogGoGamePawn, Log, TEXT("Found existing pawn with color %d."), color);
							if (color == EGoGameCellState::Black)
								color = EGoGameCellState::White;
							else if (color == EGoGameCellState::White)
							{
								color = EGoGameCellState::Empty;
								break;
							}
						}
					}
				}
			}
		}

		// Tell the client and assign the color locally as well.
		UE_LOG(LogGoGamePawn, Log, TEXT("Client gets color %d."), color);
		this->AssignColor(color);
		playerController->myColor = color;
	}
}

// Server called, run on client.
void AGoGamePawn::AssignColor_Implementation(EGoGameCellState color)
{
	AGoGamePlayerController* playerController = Cast<AGoGamePlayerController>(this->Owner);
	if (playerController)
		playerController->myColor = color;
}

// Server called, client run.
void AGoGamePawn::ResetBoard_Implementation(int boardSize)
{
	UE_LOG(LogGoGamePawn, Log, TEXT("ResetBoard RPC called!"));

	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (gameState)
		gameState->ResetBoard(boardSize);
}

// Server called, client run (all clients).
void AGoGamePawn::AlterGameState_AllClients_Implementation(int i, int j)
{
	UE_LOG(LogGoGamePawn, Log, TEXT("AlterGameState_AllClients RPC called!"));

	this->AlterGameState_Shared(i, j);
}

// Server called, client run (client owning pawn).
void AGoGamePawn::AlterGameState_OwningClient_Implementation(int i, int j)
{
	UE_LOG(LogGoGamePawn, Log, TEXT("AlterGameState_OwningClient RPC called!"));

	this->AlterGameState_Shared(i, j);
}

void AGoGamePawn::AlterGameState_Shared(int i, int j)
{
	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (gameState)
	{
		GoGameMatrix::CellLocation cellLocation;
		cellLocation.i = i;
		cellLocation.j = j;

		UE_LOG(LogGoGamePawn, Log, TEXT("Client placing stone at location (%d, %d) as per the server's request."), cellLocation.i, cellLocation.j);

		bool altered = gameState->AlterGameState(cellLocation, EGoGameCellState::Black_or_White);
		if (!altered)
		{
			UE_LOG(LogGoGamePawn, Error, TEXT("Board alteration failed!  This shouldn't happen as the server vets all moves before relaying them to the client."));
		}
	}
}

// Client called, server run.
void AGoGamePawn::TryAlterGameState_Implementation(int i, int j)
{
	UE_LOG(LogGoGamePawn, Log, TEXT("TryAlterGameState RPC called!"));

	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	AGoGamePlayerController* playerController = Cast<AGoGamePlayerController>(this->Owner);
	if (gameState && playerController)
	{
		GoGameMatrix::CellLocation cellLocation;
		cellLocation.i = i;
		cellLocation.j = j;

		// Can the requested move be made?
		bool legalMove = false;
		bool altered = gameState->AlterGameState(cellLocation, playerController->myColor, &legalMove);
		check(!altered);
		if (legalMove)
		{
			// Yes.  Now go tell all the clients to apply the move.
			// Note that this will also execute locally to change the servers game state too.
			this->AlterGameState_AllClients(cellLocation.i, cellLocation.j);
		}
	}
}

END_FUNCTION_BUILD_OPTIMIZATION

void AGoGamePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	this->InputComponent->BindAction("ExitGame", IE_Pressed, this, &AGoGamePawn::ExitGame);

	this->InputComponent->BindAction("RotateBoardLeft", IE_Pressed, this, &AGoGamePawn::MoveBoardLeftPressed);
	this->InputComponent->BindAction("RotateBoardLeft", IE_Released, this, &AGoGamePawn::MoveBoardLeftReleased);
	this->InputComponent->BindAction("RotateBoardRight", IE_Pressed, this, &AGoGamePawn::MoveBoardRightPressed);
	this->InputComponent->BindAction("RotateBoardRight", IE_Released, this, &AGoGamePawn::MoveBoardRightReleased);
	this->InputComponent->BindAction("RotateBoardUp", IE_Pressed, this, &AGoGamePawn::MoveBoardUpPressed);
	this->InputComponent->BindAction("RotateBoardUp", IE_Released, this, &AGoGamePawn::MoveBoardUpReleased);
	this->InputComponent->BindAction("RotateBoardDown", IE_Pressed, this, &AGoGamePawn::MoveBoardDownPressed);
	this->InputComponent->BindAction("RotateBoardDown", IE_Released, this, &AGoGamePawn::MoveBoardDownReleased);
}

