#pragma once

#include "CoreMinimal.h"
#include "GoGameMatrix.h"
#include "UMG/Public/Blueprint/UserWidget.h"
#include "GoGameHUD.generated.h"

class AGoGameBoard;
class GoGameMatrix;

UCLASS(Abstract, editinlinenew, BlueprintType, Blueprintable)
class GOGAME_API UGoGameHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGoGameHUDWidget(const FObjectInitializer& ObjectInitializer);
	virtual ~UGoGameHUDWidget();

	virtual bool Initialize() override;

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void OnBoardAppearanceChanged();

	FText CellStateToText(EGoGameCellState cellState);
};