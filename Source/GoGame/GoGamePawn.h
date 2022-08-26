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

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	GoGameMatrix::ConnectedRegion* currentlySelectedRegion;

	UFUNCTION(Server, Reliable)
	void RequestSetup();

	UFUNCTION(Client, Reliable)
	void AssignColor(EGoGameCellState color);

	UFUNCTION(Client, Reliable)
	void ResetBoard(int boardSize);

	UFUNCTION(NetMulticast, Reliable)
	void AlterGameState_AllClients(int i, int j);

	UFUNCTION(Client, Reliable)
	void AlterGameState_OwningClient(int i, int j);

	void AlterGameState_Shared(int i, int j);

	UFUNCTION(Server, Reliable)
	void TryAlterGameState(int i, int j);

	void SetHighlightOfCurrentlySelectedRegion(bool highlighted);

	FRotator rotationRate;
	FRotator rotationRateDelta;
	FRotator rotationRateDrag;
	FRotator maxRotationRate;
	FRotator minRotationRate;

	UPROPERTY()
	AGoGameBoard* gameBoard;
};
