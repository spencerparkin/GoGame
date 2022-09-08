#include "GoGameAI.h"

GoGameAI::GoGameAI(EGoGameCellState favoredColor)
{
	this->favoredPlayer = favoredColor;
	this->gameState = nullptr;
}

/*virtual*/ GoGameAI::~GoGameAI()
{
}

/*virtual*/ void GoGameAI::BeginThinking()
{
	this->stonePlacement.i = TNumericLimits<int>::Max();
	this->stonePlacement.j = TNumericLimits<int>::Max();
}

/*virtual*/ void GoGameAI::StopThinking()
{
}

/*virtual*/ bool GoGameAI::TickThinking()
{
	return false;
}