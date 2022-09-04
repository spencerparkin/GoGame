#include "GoGamePawnAI.h"

AGoGamePawnAI::AGoGamePawnAI()
{
}

/*virtual*/ AGoGamePawnAI::~AGoGamePawnAI()
{
}

/*virtual*/ void AGoGamePawnAI::BeginPlay()
{
	Super::BeginPlay();

	//...
}

/*virtual*/ void AGoGamePawnAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//...
	
	// TODO: Here we should always be do one or more iterations of MCTS, even if it's not our turn.
	//       That way we don't waste any idle time at all.  We do have to watch our memory consumption, though, I suppose.
	//       Good resource: https://www.youtube.com/watch?v=UXW2yZndl7U
	//       The UCB function has an exploration component and an exploitation component.
	//       Note that we can persist our tree across turns, and prune at the root when the turn changes.
}