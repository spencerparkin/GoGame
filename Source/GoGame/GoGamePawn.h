// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoGameMatrix.h"
#include "GoGamePawn.generated.h"

class AGoGameBoard;
class AGoGameBoardPiece;

UCLASS()
class GOGAME_API AGoGamePawn : public APawn
{
	GENERATED_BODY()

public:
	AGoGamePawn();
	virtual ~AGoGamePawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_MyColorChanged();

	UFUNCTION(Server, Reliable)
	void RequestSetup();

	UFUNCTION(Client, Reliable)
	void ResetBoard(int boardSize);

	UFUNCTION(NetMulticast, Reliable)
	void AlterGameState_AllClients(int i, int j);

	UFUNCTION(Client, Reliable)
	void AlterGameState_OwningClient(int i, int j);

	void AlterGameState_Shared(int i, int j);

	UFUNCTION(Server, Reliable)
	void TryAlterGameState(int i, int j);

	UPROPERTY()
	AGoGameBoard* gameBoard;

	UPROPERTY(ReplicatedUsing=OnRep_MyColorChanged)
	EGoGameCellState myColor;
};
