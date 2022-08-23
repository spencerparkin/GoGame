#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GoGameMatrix.h"
#include "GoGameBoardPiece.generated.h"

UCLASS()
class AGoGameBoardPiece : public AActor
{
	GENERATED_BODY()

public:
	AGoGameBoardPiece();
	virtual ~AGoGameBoardPiece();

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void HandleClick(UPrimitiveComponent* ClickedComp, FKey ButtonClicked);

	GoGameMatrix::CellLocation cellLocation;
};