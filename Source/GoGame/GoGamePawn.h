// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoGamePawn.generated.h"

UCLASS()
class GOGAME_API AGoGamePawn : public APawn
{
	GENERATED_BODY()

public:
	AGoGamePawn();
	virtual ~AGoGamePawn();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
