#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "GoGameLevelScript.generated.h"

class GoGameAI;

UCLASS(BlueprintType, Blueprintable)
class GOGAME_API AGoGameLevelScript : public ALevelScriptActor
{
	GENERATED_BODY()

public:
	AGoGameLevelScript();
	virtual ~AGoGameLevelScript();

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void SetupHUD();

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void LetComputerTakeTurn();

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void UndoLastMove();

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void ForfeitTurn();

	GoGameAI* gameAI;
};