// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GoGameMode.generated.h"

class UGoGameOptions;

/**
 * 
 */
UCLASS()
class GOGAME_API AGoGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AGoGameMode();
	virtual ~AGoGameMode();

	virtual void InitGameState() override;

	virtual void PostLogin(APlayerController* playerController) override;

	virtual void Tick(float DeltaTime) override;

	struct ClientNeedsSetup
	{
		APlayerController* playerController;
		int countDown;
	};

	typedef TDoubleLinkedList<ClientNeedsSetup> ClientNeedsSetupList;
	ClientNeedsSetupList clientNeedsSetupList;

	void SetupClient(APlayerController* playerController);
};
