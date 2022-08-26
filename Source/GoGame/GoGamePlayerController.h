#pragma once

#include "CoreMinimal.h"
#include "GoGameMatrix.h"
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
	virtual void SetPawn(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetViewTarget(class AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams = FViewTargetTransitionParams()) override;

	enum ControlType
	{
		HUMAN,
		COMPUTER
	};

	ControlType controlType;

	EGoGameCellState myColor;

	UPROPERTY()
	ACameraActor* cameraActor;

	UPROPERTY()
	AGoGamePawn* gamePawn;
};

