#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GoGamePointer.generated.h"

UCLASS()
class AGoGamePointer : public AActor
{
	GENERATED_BODY()

public:
	AGoGamePointer();
	virtual ~AGoGamePointer();

	UFUNCTION(BlueprintImplementableEvent, Category = GoGame)
	void HandleBoardRotated();

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void GetBounceVector(FVector& bounceVector);
};