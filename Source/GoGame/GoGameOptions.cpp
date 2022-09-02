#include "GoGameOptions.h"

UGoGameOptions::UGoGameOptions()
{
	// TODO: How might we presist these on disk?  Does UE have something for that already?
	this->showHoverHighlights = false;
	this->showPointerToMostRecentlyPlacedStone = false;
}

/*virtual*/ UGoGameOptions::~UGoGameOptions()
{
}