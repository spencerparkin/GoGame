#pragma once

#include "CoreMinimal.h"
#include "GoGamePawn.h"
#include "GoGameMatrix.h"
#include "GoGamePawnHuman.generated.h"

UCLASS()
class GOGAME_API AGoGamePawnHuman : public AGoGamePawn
{
	GENERATED_BODY()

public:
	AGoGamePawnHuman();
	virtual ~AGoGamePawnHuman();

	void ExitGamePressed();
	void ForfeitTurnPressed();
	void MoveBoardLeftPressed();
	void MoveBoardLeftReleased();
	void MoveBoardRightPressed();
	void MoveBoardRightReleased();
	void MoveBoardUpPressed();
	void MoveBoardUpReleased();
	void MoveBoardDownPressed();
	void MoveBoardDownReleased();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	GoGameMatrix::ConnectedRegion* currentlySelectedRegion;

	void SetHighlightOfCurrentlySelectedRegion(bool highlighted);

	FRotator rotationRate;
	FRotator rotationRateDelta;
	FRotator rotationRateDrag;
	FRotator maxRotationRate;
	FRotator minRotationRate;
};