#include "GoGamePlayerController.h"
#include "GoGamePawn.h"
#include "Camera/CameraActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGamePlayerController, Log, All);

AGoGamePlayerController::AGoGamePlayerController()
{
	this->PrimaryActorTick.bCanEverTick = true;
	this->cameraActor = nullptr;
	this->gamePawn = nullptr;
	this->myColor = EGoGameCellState::Black_or_White;
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
	if (this->GetLocalRole() != ENetRole::ROLE_Authority || !IsRunningDedicatedServer())
	{
		FTransform transform;
		transform.SetLocation(FVector(0.0f, 0.0f, 100.0f));
		transform.SetRotation(FRotator(-90.0f, 0.0f, 0.0f).Quaternion());

		static int cameraNumber = 0;
		FActorSpawnParameters spawnParams;
		spawnParams.Name = *FString::Format(TEXT("GoGameCamera{0}"), { cameraNumber++ });

		this->cameraActor = this->GetWorld()->SpawnActorAbsolute<ACameraActor>(ACameraActor::StaticClass(), transform, spawnParams);

		this->SetViewTarget(cameraActor);
	}
}

BEGIN_FUNCTION_BUILD_OPTIMIZATION

/*virtual*/ void AGoGamePlayerController::Tick(float DeltaTime)
{
	if ((this->GetLocalRole() != ENetRole::ROLE_Authority || !IsRunningDedicatedServer()) && this->cameraActor)
	{
		static FRotator rotator(-90.0f, 0.0f, 0.0f);
		this->cameraActor->SetActorRotation(rotator);
		this->SetViewTarget(cameraActor);		// UE is changing this out from underneath me so I have to set it every frame?!  Why?  WTF?!
	}

	static bool hack = false;
	if (hack)
		this->SetPawn(this->gamePawn);
}

END_FUNCTION_BUILD_OPTIMIZATION

/*virtual*/ void AGoGamePlayerController::SetViewTarget(class AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams /*= FViewTargetTransitionParams()*/)
{
	//UE_LOG(LogGoGamePlayerController, Log, TEXT("========================================================================"));
	//UE_LOG(LogGoGamePlayerController, Log, TEXT("Set view target on player controller (color=%d) from 0x%016x to 0x%016x."), int(this->myColor), this->GetViewTarget(), NewViewTarget);
	//UE_LOG(LogGoGamePlayerController, Log, TEXT("========================================================================"));

	Super::SetViewTarget(NewViewTarget, TransitionParams);
}

/*virtual*/ void AGoGamePlayerController::SetPawn(APawn* InPawn)
{
	//UE_LOG(LogGoGamePlayerController, Log, TEXT("========================================================================"));
	//UE_LOG(LogGoGamePlayerController, Log, TEXT("Set pawn on player controller (color=%d) from 0x%016x to 0x%016x."), int(this->myColor), this->GetPawn(), InPawn);
	//UE_LOG(LogGoGamePlayerController, Log, TEXT("========================================================================"));

	if (!this->gamePawn)
	{
		this->gamePawn = Cast<AGoGamePawn>(InPawn);
		if (this->gamePawn)
		{
			UE_LOG(LogGoGamePlayerController, Log, TEXT(" * * * GOT ASSIGNED ACTUAL PAWN WE CARE ABOUT!!! * * * "));
		}
	}

	Super::SetPawn(InPawn);
}