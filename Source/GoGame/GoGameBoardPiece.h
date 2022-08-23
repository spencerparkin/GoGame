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

	GoGameMatrix::CellLocation cellLocation;
};