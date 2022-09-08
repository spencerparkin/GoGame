#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "GoGameLevelScript.generated.h"

class AGoGamePawnHuman;
class AGoGamePawnAI;

UCLASS(BlueprintType, Blueprintable)
class GOGAME_API AGoGameLevelScript : public ALevelScriptActor
{
	GENERATED_BODY()

public:
	AGoGameLevelScript();
	virtual ~AGoGameLevelScript();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void SetupHUD();

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void UndoLastTwoMoves();

	UPROPERTY()
	AGoGamePawnHuman* gamePawnHumanBlack;

	UPROPERTY()
	AGoGamePawnHuman* gamePawnHumanWhite;

	UPROPERTY()
	AGoGamePawnAI* gamePawnAI;
};