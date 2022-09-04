// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GoGameMode.generated.h"

class UGoGameOptions;

// To debug server: "$(SolutionDir)GoGame.uproject" GoGameLevel -server -game -log -notimeouts -skipcompile
// To debug client: "$(SolutionDir)GoGame.uproject" 127.0.0.1 -game -windowed -notimeouts -skipcompile

// Note that this game mode is used in the go game level, not the lobby/main-menu/front-end.
UCLASS()
class GOGAME_API AGoGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AGoGameMode();
	virtual ~AGoGameMode();

	virtual void InitGameState() override;

	virtual void PostLogin(APlayerController* playerController) override;
};
