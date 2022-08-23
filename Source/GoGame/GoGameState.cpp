// Fill out your copyright notice in the Description page of Project Settings.

#include "GoGameState.h"

AGoGameState::AGoGameState()
{
}

/*virtual*/ AGoGameState::~AGoGameState()
{
	this->Clear();
}

void AGoGameState::Clear()
{
	while (this->matrixStack.Num() > 0)
	{
		GoGameMatrix* gameMatrix = this->PopMatrix();
		delete gameMatrix;
	}
}

GoGameMatrix* AGoGameState::GetCurrentMatrix()
{
	if (this->matrixStack.Num() > 0)
		return this->matrixStack[this->matrixStack.Num() - 1];
	
	return nullptr;
}

void AGoGameState::PushMatrix(GoGameMatrix* gameMatrix)
{
	this->matrixStack.Push(gameMatrix);
}

GoGameMatrix* AGoGameState::PopMatrix()
{
	if (this->matrixStack.Num() == 0)
		return nullptr;

	GoGameMatrix* gameMatrix = this->matrixStack[this->matrixStack.Num() - 1];
	this->matrixStack.Pop();
	return gameMatrix;
}