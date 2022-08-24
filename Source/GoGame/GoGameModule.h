// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UGoGameOptions;

class GoGameModule : public IModuleInterface
{
public:
	GoGameModule(void);
	virtual ~GoGameModule(void);

	virtual bool IsGameModule() const override { return true; }
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	UGoGameOptions* gameOptions;
};