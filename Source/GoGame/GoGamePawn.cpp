// Fill out your copyright notice in the Description page of Project Settings.

#include "GoGamePawn.h"
#include "GoGameBoard.h"
#include "GoGameMisc.h"
#include "GoGameMatrix.h"
#include "GoGameMode.h"
#include "GoGameOptions.h"
#include "GoGameModule.h"
#include "Kismet/GameplayStatics.h"

AGoGamePawn::AGoGamePawn()
{
	this->PrimaryActorTick.bCanEverTick = true;
	this->AutoPossessPlayer = EAutoReceiveInput::Player0;

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

