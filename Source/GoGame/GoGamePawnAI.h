// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GoGamePawn.h"
#include "GoGamePawnAI.generated.h"

class GoGameMCTS;

UCLASS()
class GOGAME_API AGoGamePawnAI : public AGoGamePawn
{
	GENERATED_BODY()

public:
	AGoGamePawnAI();
	virtual ~AGoGamePawnAI();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	GoGameMCTS* gameMCTS;
	float thinkTimeStart;
	float thinkTimeMax;
	bool stonePlacementSubmitted;
};