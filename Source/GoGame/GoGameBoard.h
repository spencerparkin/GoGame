#pragma once

#include "CoreMinimal.h"
#include "GoGameBoardPiece.h"
#include "GameFramework/Actor.h"
#include "GoGameBoard.generated.h"

class GoGameMatrix;

// This class renders of a reflection of the current game state.
UCLASS()
class AGoGameBoard : public AActor
{
	GENERATED_BODY()

public:
	AGoGameBoard();
	virtual ~AGoGameBoard();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = GoGame)
	void UpdateMaterial();

	UFUNCTION(BlueprintCallable, Category = GoGame)
	int GetMatrixSize();

	GoGameMatrix* GetCurrentMatrix();

	UPROPERTY(EditAnywhere, Category = GoGame)
	TSoftClassPtr<AGoGameBoardPiece> gameBoardPieceClass;
};