#pragma once

#include "CoreMinimal.h"
#include "GoGameBoardPiece.h"
#include "GoGameMatrix.h"
#include "GameFramework/Actor.h"
#include "UObject/SparseDelegate.h"
#include "GoGameBoard.generated.h"

class GoGameMatrix;
class AGoGameBoard;

DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE(FBoardAppearanceChangedSignature, AGoGameBoard, OnBoardAppearanceChanged);

// This class renders of a reflection of the current game state.
UCLASS()
class AGoGameBoard : public AActor
{
	GENERATED_BODY()

public:
	AGoGameBoard();
	virtual ~AGoGameBoard();

	virtual void BeginPlay() override;

	void GatherPieces(const TSet<GoGameMatrix::CellLocation>& locationSet, TArray<AGoGameBoardPiece*>& boardPieceArray);

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void UpdateAppearance();

	UFUNCTION(BlueprintImplementableEvent, Category = GoGame)
	void UpdateMaterial();

	UFUNCTION(BlueprintCallable, Category = GoGame)
	int GetMatrixSize();

	UPROPERTY(BlueprintAssignable, Category = GoGame)
	FBoardAppearanceChangedSignature OnBoardAppearanceChanged;

	GoGameMatrix* GetCurrentMatrix();

	UPROPERTY(EditAnywhere, Category = GoGame)
	TSoftClassPtr<AGoGameBoardPiece> gameBoardPieceClass;

	bool recreatePieces;
};