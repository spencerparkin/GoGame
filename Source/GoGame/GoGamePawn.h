// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoGameMatrix.h"
#include "GoGamePawn.generated.h"

class AGoGameBoard;
class AGoGameBoardPiece;

UCLASS()
class GOGAME_API AGoGamePawn : public APawn
{
	GENERATED_BODY()

public:
	AGoGamePawn();
	virtual ~AGoGamePawn();

	void ExitGame();

	void MoveBoardLeftPressed();
	void MoveBoardLeftReleased();
	void MoveBoardRightPressed();
	void MoveBoardRightReleased();
	void MoveBoardUpPressed();
	void MoveBoardUpReleased();
	void MoveBoardDownPressed();
	void MoveBoardDownReleased();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	GoGameMatrix::ConnectedRegion* currentlySelectedRegion;

private:

	void SetHighlightOfCurrentlySelectedRegion(bool highlighted);

	FRotator rotationRate;
	FRotator rotationRateDelta;
	FRotator rotationRateDrag;
	FRotator maxRotationRate;
	FRotator minRotationRate;

	UPROPERTY()
	AGoGameBoard* gameBoard;
};
