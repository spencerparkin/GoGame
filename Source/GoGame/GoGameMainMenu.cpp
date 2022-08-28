#include "GoGameMainMenu.h"
#include "Kismet/GameplayStatics.h"
#include "UMG/Public/Components/EditableTextBox.h"
#include "Blueprint/WidgetTree.h"

#define LOCTEXT_NAMESPACE "GoGameMainMenuWidget"

UGoGameMainMenuWidget::UGoGameMainMenuWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

/*virtual*/ UGoGameMainMenuWidget::~UGoGameMainMenuWidget()
{
}

/*virtual*/ bool UGoGameMainMenuWidget::Initialize()
{
	if (!Super::Initialize())
		return false;

	return true;
}

BEGIN_FUNCTION_BUILD_OPTIMIZATION

void UGoGameMainMenuWidget::OnPlayStandaloneGame()
{
	// TODO: Add a way to travel back to the main menu from the go-game level.
	UGameplayStatics::OpenLevel(this->GetWorld(), TEXT("GoGameLevel"));
}

void UGoGameMainMenuWidget::OnPlayServerGame()
{
	UEditableTextBox* textBox = Cast<UEditableTextBox>(this->WidgetTree->FindWidget("ServerAddrEditableTextBox"));
	if (textBox)
	{
		// TODO: Setup transition level to see how that works?  Might be nice to display something like "connecting..." while waiting to connect to the server and load the level.
		FString serverAddress = textBox->Text.ToString();
		UGameplayStatics::OpenLevel(this->GetWorld(), *serverAddress);
	}
}

END_FUNCTION_BUILD_OPTIMIZATION

void UGoGameMainMenuWidget::OnExitGame()
{
#if !WITH_EDITOR
	FPlatformMisc::RequestExit(true);
#endif
}

#undef LOCTEXT_NAMESPACE