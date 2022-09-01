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
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGamePawn, Log, All);

// TODO: Add zoom with mouse wheel.
AGoGamePawn::AGoGamePawn()
{
	this->myColor = int(EGoGameCellState::Black_or_White);
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

		FRotator oldRotation = this->gameBoard->GetActorRotation();
		FQuat quat = oldRotation.Quaternion();

		FVector pitchAxis(0.0f, 1.0f, 0.0f);
		float pitchAngle = this->rotationRate.Pitch * DeltaTime;
		FQuat pitchQuat(pitchAxis, pitchAngle);

		FVector yawAxis = quat.RotateVector(FVector(0.0f, 0.0f, 1.0f));
		float yawAngle = this->rotationRate.Yaw * DeltaTime;
		FQuat yawQuat(yawAxis, yawAngle);

		quat = pitchQuat * yawQuat * quat;

		FRotator newRotation = quat.Rotator();
		this->gameBoard->SetActorRotation(newRotation);

		FRotator diffRotation = newRotation - oldRotation;
		FVector diffRotationVec(diffRotation.Yaw, diffRotation.Pitch, diffRotation.Roll);
		float eps = 1e-3f;
		if (diffRotationVec.SquaredLength() >= eps && this->gameBoard->gamePointer)
			this->gameBoard->gamePointer->HandleBoardRotated();

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

				FCollisionQueryParams queryParams;
				if (gameBoard->gamePointer)
					queryParams.AddIgnoredActor(gameBoard->gamePointer);

				FHitResult hitResult;
				this->GetWorld()->LineTraceSingleByChannel(hitResult, traceStart, traceEnd, ECC_Visibility, queryParams);

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

/*virtual*/ void AGoGamePawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGoGamePawn, myColor);
}

void AGoGamePawn::OnRep_MyColorChanged()
{
	UE_LOG(LogGoGamePawn, Log, TEXT("My color rep-notify fired!  (myColor=%d)"), this->myColor);

	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (gameState)
		gameState->renderRefreshNeeded = true;
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
	}
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
		AGoGamePawn* gamePawn = Cast<AGoGamePawn>(playerController->GetPawn());
		if (gamePawn)
		{
			GoGameMatrix::CellLocation cellLocation;
			cellLocation.i = i;
			cellLocation.j = j;

			// Can the requested move be made?
			bool legalMove = false;
			bool altered = gameState->AlterGameState(cellLocation, EGoGameCellState(gamePawn->myColor), &legalMove);
			check(!altered);
			if (legalMove)
			{
				// Yes.  Now go tell all the clients to apply the move.
				// Note that this will also execute locally to change the servers game state too.
				this->AlterGameState_AllClients(cellLocation.i, cellLocation.j);
			}
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

