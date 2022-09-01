#pragma once

#include "CoreMinimal.h"
#include "UMG/Public/Blueprint/UserWidget.h"
#include "GoGamePauseMenu.generated.h"

class UGoGameOptions;

UCLASS(Abstract, editinlinenew, BlueprintType, Blueprintable)
class GOGAME_API UGoGamePauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGoGamePauseMenuWidget(const FObjectInitializer& ObjectInitializer);
	virtual ~UGoGamePauseMenuWidget();

	virtual bool Initialize() override;

	UPROPERTY(BlueprintReadWrite, Category = GoGame)
	UGoGameOptions* gameOptions;
};