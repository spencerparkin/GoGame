// Fill out your copyright notice in the Description page of Project Settings.


#include "GoGamePawn.h"

AGoGamePawn::AGoGamePawn()
{
	this->PrimaryActorTick.bCanEverTick = true;
	this->AutoPossessPlayer = EAutoReceiveInput::Player0;
}

/*virtual*/ AGoGamePawn::~AGoGamePawn()
{
}

void AGoGamePawn::BeginPlay()
{
	Super::BeginPlay();
}

void AGoGamePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// TODO: Might do a trace here to highlight what's under the mouse.  See the puzzle template for an example.
	//       Would be cool to highlight groups and territories as you hover the mouse.
}

void AGoGamePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// TODO: Might bind to mouse or keyboard for a way to rotate the game board.
}

