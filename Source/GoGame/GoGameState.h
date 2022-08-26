// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GoGameMatrix.h"
#include "GoGameState.generated.h"

// TODO: Probably need to disolve this class entirely and move it to a class owned by the pawn.  This will let me get rid of the client setup hack.
UCLASS()
class GOGAME_API AGoGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AGoGameState();
	virtual ~AGoGameState();

	void PushMatrix(GoGameMatrix* gameMatrix);
	GoGameMatrix* PopMatrix();

	GoGameMatrix* GetCurrentMatrix();
	GoGameMatrix* GetForbiddenMatrix();

	void Clear();
	void ResetBoard(int boardSize);
	bool AlterGameState(const GoGameMatrix::CellLocation& cellLocation, bool* legalMove = nullptr);

	virtual void Tick(float DeltaTime) override;

	// A stack here facilitates both undo/redo and AI game-tree traversal.
	TArray<GoGameMatrix*> matrixStack;

	// This is somewhat redundant, but is more convenient for synchronizing clients with the server.
	TArray<GoGameMatrix::CellLocation> placementHistory;

	bool renderRefreshNeeded;
};
