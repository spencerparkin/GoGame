#pragma once

#include "CoreMinimal.h"
#include "GoGameMatrix.h"
#include "UMG/Public/Blueprint/UserWidget.h"
#include "GoGameMainMenu.generated.h"

class AGoGameBoard;
class GoGameMatrix;

UCLASS(Abstract, editinlinenew, BlueprintType, Blueprintable)
class GOGAME_API UGoGameMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGoGameMainMenuWidget(const FObjectInitializer& ObjectInitializer);
	virtual ~UGoGameMainMenuWidget();

	virtual bool Initialize() override;

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void OnPlayStandaloneGame();

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void OnPlayServerGame();

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void OnExitGame();

	UPROPERTY(BlueprintReadWrite, Category = GoGame)
	int boardSize;
};