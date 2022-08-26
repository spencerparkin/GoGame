// Copyright Epic Games, Inc. All Rights Reserved.

#include "GoGameMode.h"
#include "GoGamePlayerController.h"
#include "GoGamePawn.h"
#include "GoGameState.h"
#include "GoGameMatrix.h"
#include "GoGameModule.h"
#include "GoGameOptions.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGameMode, Log, All);

AGoGameMode::AGoGameMode()
{
	this->PrimaryActorTick.bCanEverTick = true;
	this->PlayerControllerClass = AGoGamePlayerController::StaticClass();
	this->DefaultPawnClass = AGoGamePawn::StaticClass();
	this->GameStateClass = AGoGameState::StaticClass();
}

/*virtual*/ AGoGameMode::~AGoGameMode()
{
}

/*virtual*/ void AGoGameMode::InitGameState()
{
	Super::InitGameState();

	AGoGameState* gameState = Cast<AGoGameState>(this->GameState);
	if (gameState)
	{
		UE_LOG(LogGoGameMode, Log, TEXT("Create initial game state on server!"));
		gameState->ResetBoard(19);
	}
}

/*virtual*/ void AGoGameMode::Tick(float DeltaTime)
{
	if (GIsServer)
	{
		ClientNeedsSetupList::TDoubleLinkedListNode* node = this->clientNeedsSetupList.GetHead();
		while (node)
		{
			ClientNeedsSetupList::TDoubleLinkedListNode* nextNode = node->GetNextNode();
			ClientNeedsSetup& clientNeedsSetup = node->GetValue();
			if (clientNeedsSetup.countDown > 0)
				clientNeedsSetup.countDown--;
			else
			{
				this->SetupClient(clientNeedsSetup.playerController);
				this->clientNeedsSetupList.RemoveNode(node);
			}

			node = nextNode;
		}
	}
}

void AGoGameMode::SetupClient(APlayerController* playerController)
{
	APlayerState* playerState = playerController->GetPlayerState<APlayerState>();
	AGoGamePawn* gamePawn = playerState ? playerState->GetPawn<AGoGamePawn>() : nullptr;
	AGoGameState* gameState = Cast<AGoGameState>(UGameplayStatics::GetGameState(this->GetWorld()));
	if (gameState && gamePawn)
	{
		UE_LOG(LogGoGameMode, Log, TEXT("Replicating game state for client!"));
		gamePawn->ResetBoard(gameState->GetCurrentMatrix()->GetMatrixSize());

		// Replay the whole game history for the client.
		for (int i = 0; i < gameState->placementHistory.Num(); i++)
		{
			const GoGameMatrix::CellLocation& cellLocation = gameState->placementHistory[i];
			gamePawn->AlterGameState_OwningClient(cellLocation.i, cellLocation.j);
		}
	}
}

/*virtual*/ void AGoGameMode::PostLogin(APlayerController* playerController)
{
	Super::PostLogin(playerController);

	if (this->GetLocalRole() == ENetRole::ROLE_Authority)
	{
		// Total hack: Defer setup of client for a while, because it needs its state object replicated before it can be setup.
		ClientNeedsSetup clientNeedsSetup;
		clientNeedsSetup.playerController = playerController;
		clientNeedsSetup.countDown = 100;
		this->clientNeedsSetupList.AddTail(clientNeedsSetup);
	}
}