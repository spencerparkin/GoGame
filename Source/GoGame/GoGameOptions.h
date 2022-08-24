#pragma once

#include "CoreMinimal.h"
#include "CoreUObject.h"
#include "GoGameOptions.generated.h"

// Being a UObject, can we easily expose this through some kind of UI?
UCLASS()
class UGoGameOptions : public UObject
{
	GENERATED_BODY()

public:
	UGoGameOptions();
	virtual ~UGoGameOptions();

	UPROPERTY(BlueprintReadWrite, Category = GoGame)
	int boardDimension;

	UPROPERTY(BlueprintReadWrite, Category = GoGame)
	bool showHoverHighlights;
};