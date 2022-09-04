#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GoGamePlayerController.generated.h"

class ACameraActor;

UCLASS()
class AGoGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGoGamePlayerController();
	virtual ~AGoGamePlayerController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY()
	ACameraActor* cameraActor;
};

