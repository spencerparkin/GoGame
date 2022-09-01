#include "GoGamePointer.h"
#include "GoGameBoard.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoGamePointer, Log, All);

AGoGamePointer::AGoGamePointer()
{
}

/*virtual*/ AGoGamePointer::~AGoGamePointer()
{
}

void AGoGamePointer::GetBounceVector(FVector& bounceVector)
{
	bounceVector = FVector(0.0f, 0.0f, 1.0f);

	AGoGameBoard* gameBoard = Cast<AGoGameBoard>(this->Owner);
	if(gameBoard)
	{
		FQuat quat = gameBoard->GetActorRotation().Quaternion();
		bounceVector = quat.RotateVector(bounceVector);

		//UE_LOG(LogGoGamePointer, Display, TEXT("Bounce vector: %f, %f, %f"), bounceVector.X, bounceVector.Y, bounceVector.Z);
	}
}