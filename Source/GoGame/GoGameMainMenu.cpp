#include "GoGameMainMenu.h"
#include "Kismet/GameplayStatics.h"
#include "UMG/Public/Components/EditableTextBox.h"
#include "Blueprint/WidgetTree.h"

#define LOCTEXT_NAMESPACE "GoGameMainMenuWidget"

UGoGameMainMenuWidget::UGoGameMainMenuWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
	this->boardSize = 19;
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
	FString options = FString::Format(TEXT("BoardSize={0}"), { this->boardSize });
	UGameplayStatics::OpenLevel(this->GetWorld(), TEXT("GoGameLevel"), true, options);
}

void UGoGameMainMenuWidget::OnPlayServerGame()
{
	UEditableTextBox* textBox = Cast<UEditableTextBox>(this->WidgetTree->FindWidget("ServerAddrEditableTextBox"));
	if (textBox)
	{
		// TODO: Setup transition level to see how that works?  Might be nice to display something like "connecting..." while waiting to connect to the server and load the level.
		FString serverAddress = textBox->Text.ToString();
		UGameplayStatics::OpenLevel(this->GetWorld(), *serverAddress, true);
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