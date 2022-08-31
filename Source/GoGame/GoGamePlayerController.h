#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GoGamePlayerController.generated.h"

class ACameraActor;
class AGoGamePawn;

UCLASS()
class AGoGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGoGamePlayerController();
	virtual ~AGoGamePlayerController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	enum ControlType
	{
		HUMAN,
		COMPUTER
	};

	ControlType controlType;

	UPROPERTY()
	ACameraActor* cameraActor;
};

