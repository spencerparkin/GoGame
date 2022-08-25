// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GoGameMatrix.h"
#include "GoGameState.generated.h"

// TODO: UE documentation says this is replicated for multiplyer game-play.  Can we take advantage of
//       this to make this game work over a network?
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

	UFUNCTION(Client, Reliable)
	void ResetBoard(int boardSize, int playerID);

	void ResetBoard_Internal(int boardSize);

	virtual void Tick(float DeltaTime) override;

	// This is called by the server to tell all or a specific client to place a stone at the given location.
	UFUNCTION(Client, Reliable)
	void AlterGameState(int i, int j, int playerID);

	// This actually does the work.  We do it here so we can share code between client and server.
	bool AlterGameState_Internal(const GoGameMatrix::CellLocation& cellLocation);

	// This is called by the client to ask the server to try to place a stone at the given location.
	UFUNCTION(Server, Reliable)
	void TryAlterGameState(int i, int j);

	// A stack here facilitates both undo/redo and AI game-tree traversal.
	TArray<GoGameMatrix*> matrixStack;

	// This is somewhat redundant, but is more convenient for synchronizing clients with the server.
	TArray<GoGameMatrix::CellLocation> placementHistory;

	bool CanPerformRPC(int playerID);

	bool renderRefreshNeeded;
};
