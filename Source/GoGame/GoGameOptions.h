#pragma once

#include "CoreMinimal.h"
#include "CoreUObject.h"
#include "GoGameOptions.generated.h"

// Note that these should be local-only options that do not require synchronization across the network.
// TODO: Maybe inherit here from USaveGame and then load options on start-up and save them out on shutdown.
UCLASS()
class UGoGameOptions : public UObject
{
	GENERATED_BODY()

public:
	UGoGameOptions();
	virtual ~UGoGameOptions();

	UPROPERTY(BlueprintReadWrite, Category = GoGame)
	bool showHoverHighlights;

	UPROPERTY(BlueprintReadWrite, Category = GoGame)
	bool showPointerToMostRecentlyPlacedStone;
};