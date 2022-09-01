#include "GoGamePlayerController.h"
#include "GoGamePawn.h"
#include "Camera/CameraActor.h"
#include "Kismet/KismetSystemLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGamePlayerController, Log, All);

AGoGamePlayerController::AGoGamePlayerController()
{
	this->cameraActor = nullptr;
	this->PrimaryActorTick.bCanEverTick = true;
	this->controlType = ControlType::HUMAN;
	this->bShowMouseCursor = true;
	this->bEnableClickEvents = true;
	this->bEnableTouchEvents = true;
	this->DefaultMouseCursor = EMouseCursor::Crosshairs;
}

/*virtual*/ AGoGamePlayerController::~AGoGamePlayerController()
{
}

/*virtual*/ void AGoGamePlayerController::BeginPlay()
{
	if (!this->HasAuthority() || UKismetSystemLibrary::IsStandalone(this->GetWorld()))
	{
		FTransform transform;
		transform.SetLocation(FVector(0.0f, 0.0f, 100.0f));
		transform.SetRotation(FRotator(-90.0f, 0.0f, 0.0f).Quaternion());

		static int cameraCount = 0;
		FActorSpawnParameters spawnParams;
		spawnParams.Name = *FString::Format(TEXT("GoGameCamera{0}"), { cameraCount++ });

		this->cameraActor = this->GetWorld()->SpawnActorAbsolute<ACameraActor>(ACameraActor::StaticClass(), transform, spawnParams);
	}
}

BEGIN_FUNCTION_BUILD_OPTIMIZATION

/*virtual*/ void AGoGamePlayerController::Tick(float DeltaTime)
{
	if (!this->HasAuthority() || UKismetSystemLibrary::IsStandalone(this->GetWorld()))
	{
		AActor* viewTarget = this->GetViewTarget();
		if(viewTarget != this->cameraActor)
		{
			UE_LOG(LogGoGamePlayerController, Log, TEXT("Fixing view target!!!"));
			this->SetViewTarget(this->cameraActor);
		}
	}
}

END_FUNCTION_BUILD_OPTIMIZATION