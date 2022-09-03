#pragma once

#include "GoGameMatrix.h"

class AGoGameState;

// I'm not under any delusion that I can make a good Go-game AI, but I can try to do something just for run.
class GoGameAI
{
public:
	GoGameAI();
	virtual ~GoGameAI();

	virtual GoGameMatrix::CellLocation CalculateStonePlacement(AGoGameState* gameState) = 0;
};

class GoGameAIMinimax : public GoGameAI
{
public:
	GoGameAIMinimax();
	virtual ~GoGameAIMinimax();

	virtual GoGameMatrix::CellLocation CalculateStonePlacement(AGoGameState* gameState) override;
};

class GoGameAIMonteCarloTreeSearch : public GoGameAI
{
public:
	GoGameAIMonteCarloTreeSearch();
	virtual ~GoGameAIMonteCarloTreeSearch();

	virtual GoGameMatrix::CellLocation CalculateStonePlacement(AGoGameState* gameState) override;
};