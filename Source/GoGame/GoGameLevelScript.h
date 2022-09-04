#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "GoGameLevelScript.generated.h"

UCLASS(BlueprintType, Blueprintable)
class GOGAME_API AGoGameLevelScript : public ALevelScriptActor
{
	GENERATED_BODY()

public:
	AGoGameLevelScript();
	virtual ~AGoGameLevelScript();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = GoGame)
	void SetupHUD();
};