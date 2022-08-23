#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GoGamePlayerController.generated.h"

UCLASS()
class AGoGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGoGamePlayerController();
	virtual ~AGoGamePlayerController();
};

