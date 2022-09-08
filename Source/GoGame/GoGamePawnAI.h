// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GoGamePawn.h"
#include "GoGamePawnAI.generated.h"

class GoGameAI;

UCLASS()
class GOGAME_API AGoGamePawnAI : public AGoGamePawn
{
	GENERATED_BODY()

public:
	AGoGamePawnAI();
	virtual ~AGoGamePawnAI();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	enum State
	{
		STANDBY,
		BEGIN_TURN,
		TICK_TURN,
		END_TURN,
		WAIT_FOR_TURN_FLIP
	};

	State state;
	GoGameAI* gameAI;
	float turnBeginTime;
	float turnMaxTime;
	float turnMinTime;
};