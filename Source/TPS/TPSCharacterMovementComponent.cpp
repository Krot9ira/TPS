// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSCharacterMovementComponent.h"
#include "TPSCharacter.h"

UTPSCharacterMovementComponent::UTPSCharacterMovementComponent()
{
	SprintSpeedMultiplier = 1.4f;
	ADSSpeedMultiplier = 0.5f;
}

float UTPSCharacterMovementComponent::GetMaxSpeed() const
{
	ATPSCharacter* Owner = Cast<ATPSCharacter>(GetOwner());
	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() No Owner"), *FString(__FUNCTION__));
		return Super::GetMaxSpeed();
	}

	if (!Owner->IsAlive())
	{
		return 0.0f;
	}

	if (RequestToStartSprinting)
	{
		return Super::GetMaxSpeed() * SprintSpeedMultiplier;
	}

	if (RequestToStartADS)
	{
		return Super::GetMaxSpeed() * ADSSpeedMultiplier;
	}

	return Super::GetMaxSpeed();
}

void UTPSCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	//The Flags parameter contains the compressed input flags that are stored in the saved move.
	//UpdateFromCompressed flags simply copies the flags from the saved move into the movement component.
	//It basically just resets the movement component to the state when the move was made so it can simulate from there.
	RequestToStartSprinting = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;

	RequestToStartADS = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
}

FNetworkPredictionData_Client* UTPSCharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != NULL);

	if (!ClientPredictionData)
	{
		UTPSCharacterMovementComponent* MutableThis = const_cast<UTPSCharacterMovementComponent*>(this);

		MutableThis->ClientPredictionData = new FGDNetworkPredictionData_Client(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}

	return ClientPredictionData;
}

void UTPSCharacterMovementComponent::StartSprinting()
{
	RequestToStartSprinting = true;
}

void UTPSCharacterMovementComponent::StopSprinting()
{
	RequestToStartSprinting = false;
}

void UTPSCharacterMovementComponent::StartAimDownSights()
{
	RequestToStartADS = true;
}

void UTPSCharacterMovementComponent::StopAimDownSights()
{
	RequestToStartADS = false;
}

void UTPSCharacterMovementComponent::FGDSavedMove::Clear()
{
	Super::Clear();

	SavedRequestToStartSprinting = false;
	SavedRequestToStartADS = false;
}

uint8 UTPSCharacterMovementComponent::FGDSavedMove::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (SavedRequestToStartSprinting)
	{
		Result |= FLAG_Custom_0;
	}

	if (SavedRequestToStartADS)
	{
		Result |= FLAG_Custom_1;
	}

	return Result;
}

bool UTPSCharacterMovementComponent::FGDSavedMove::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
	//Set which moves can be combined together. This will depend on the bit flags that are used.
	if (SavedRequestToStartSprinting != ((FGDSavedMove*)&NewMove)->SavedRequestToStartSprinting)
	{
		return false;
	}

	if (SavedRequestToStartADS != ((FGDSavedMove*)&NewMove)->SavedRequestToStartADS)
	{
		return false;
	}

	return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void UTPSCharacterMovementComponent::FGDSavedMove::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	UTPSCharacterMovementComponent* CharacterMovement = Cast<UTPSCharacterMovementComponent>(Character->GetCharacterMovement());
	if (CharacterMovement)
	{
		SavedRequestToStartSprinting = CharacterMovement->RequestToStartSprinting;
		SavedRequestToStartADS = CharacterMovement->RequestToStartADS;
	}
}

void UTPSCharacterMovementComponent::FGDSavedMove::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	UTPSCharacterMovementComponent* CharacterMovement = Cast<UTPSCharacterMovementComponent>(Character->GetCharacterMovement());
	if (CharacterMovement)
	{
	}
}

UTPSCharacterMovementComponent::FGDNetworkPredictionData_Client::FGDNetworkPredictionData_Client(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr UTPSCharacterMovementComponent::FGDNetworkPredictionData_Client::AllocateNewMove()
{
	return FSavedMovePtr(new FGDSavedMove());
}