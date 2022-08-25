#pragma once

#include "CoreMinimal.h"
#include "CoreUObject.h"
#include "GoGameOptions.generated.h"

// Being a UObject, can we easily expose this through some kind of UI?
// Note that these should be local-only options that do not require synchronization across the network.
UCLASS()
class UGoGameOptions : public UObject
{
	GENERATED_BODY()

public:
	UGoGameOptions();
	virtual ~UGoGameOptions();

	UPROPERTY(BlueprintReadWrite, Category = GoGame)
	bool showHoverHighlights;
};