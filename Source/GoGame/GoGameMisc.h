#pragma once

#include "CoreMinimal.h"

class GOGAME_API GoGameMisc
{
public:
	
	static float Approach(float value, float valueTarget, float valueDelta)
	{
		valueDelta = FMath::Abs(valueDelta);

		float delta = valueTarget - value;

		if (FMath::Abs(delta) < valueDelta)
			value = valueTarget;
		else
			value += valueDelta * FMath::Sign(delta);

		return value;
	}
};