// Copyright Epic Games, Inc. All Rights Reserved.

#include "GoGameModule.h"
#include "GoGameOptions.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE(GoGameModule, GoGame, "GoGame");

GoGameModule::GoGameModule(void)
{
	this->gameOptions = nullptr;
}

/*virtual*/ GoGameModule::~GoGameModule(void)
{
}

/*virtual*/ void GoGameModule::StartupModule()
{
	this->gameOptions = ::NewObject<UGoGameOptions>();
	this->gameOptions->AddToRoot();
}

/*virtual*/ void GoGameModule::ShutdownModule()
{
	this->gameOptions->RemoveFromRoot();
	this->gameOptions = nullptr;
}