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

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void UpdateAppearance();

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void HandleClick(UPrimitiveComponent* ClickedComp, FKey ButtonClicked);

	UFUNCTION(BlueprintCallable, Category = GoGame)
	EGoGameCellState GetPieceColor();

	UFUNCTION(BlueprintImplementableEvent, Category = GoGame)
	void UpdateRender();

	UPROPERTY(BlueprintReadWrite, Category = GoGame)
	bool highlighted;

	GoGameMatrix::CellLocation cellLocation;
};