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

	void Clear();

private:
	// A stack here facilitates both undo/redo and AI game-tree traversal.
	TArray<GoGameMatrix*> matrixStack;
};
