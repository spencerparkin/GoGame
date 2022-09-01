#include "GoGamePauseMenu.h"
#include "GoGameModule.h"
#include "GoGameOptions.h"

UGoGamePauseMenuWidget::UGoGamePauseMenuWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
	this->gameOptions = nullptr;
}

/*virtual*/ UGoGamePauseMenuWidget::~UGoGamePauseMenuWidget()
{
}

/*virtual*/ bool UGoGamePauseMenuWidget::Initialize()
{
	if (!Super::Initialize())
		return false;

	GoGameModule* gameModule = (GoGameModule*)FModuleManager::Get().GetModule("GoGame");
	if (gameModule)
		this->gameOptions = gameModule->gameOptions;

	return true;
}