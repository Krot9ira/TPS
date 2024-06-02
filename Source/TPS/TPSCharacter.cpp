// Copyright Epic Games, Inc. All Rights Reserved.

#include "TPSCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "TPSPlayerState.h"
#include "GAS/Abilities/TPSGameplayAbility.h"
#include "TPSCharacterMovementComponent.h"
#include "TPS.h"
#include "AbilitySystemComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ATPSCharacter

ATPSCharacter::ATPSCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UTPSCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

bool ATPSCharacter::IsAlive() const
{
	return GetHealth() > 0.0f;
}

void ATPSCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	ATPSPlayerState* PS = GetPlayerState<ATPSPlayerState>();
	if (PS)
	{
		// Set the ASC on the Server. Clients do this in OnRep_PlayerState()
		AbilitySystemComponent = Cast<UAbilitySystemComponent>(PS->GetAbilitySystemComponent());

		// AI won't have PlayerControllers so we can init again here just to be sure
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);

		InitializeAttributes(PS);
		AddCharacerAbilities();
	}
	
}

void ATPSCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	ATPSPlayerState* PS = GetPlayerState<ATPSPlayerState>();
	if (PS)
	{
		// Set the ASC for clients. Server does this in PossessedBy.
		AbilitySystemComponent = Cast<UAbilitySystemComponent>(PS->GetAbilitySystemComponent());

		// Init ASC Actor Info for clients. Server will init its ASC when it possesses a new Actor.
		AbilitySystemComponent->InitAbilityActorInfo(PS, this);

		InitializeAttributes(PS);

		BindASCInput();
	}
	
}

void ATPSCharacter::AddCharacerAbilities()
{
	if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent.IsValid() || CharacterAbilityesGiven)
	{
		return;
	}

	for (TSubclassOf<UTPSGameplayAbility>& StartupAbility : CharacterAbilities)
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(StartupAbility, 1, static_cast<int32>(StartupAbility.GetDefaultObject()->AbilityInputID), this));
	}

	CharacterAbilityesGiven = true;
}

void ATPSCharacter::BindASCInput()
{
	if (!ASCInputBound && AbilitySystemComponent.IsValid() && IsValid(InputComponent))
	{
		FTopLevelAssetPath AbilityEnumAssetPath = FTopLevelAssetPath(FName("/Script/TPS"), FName("EAbilityInputID"));
		AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, FGameplayAbilityInputBinds(FString("ConfirmTarget"),
			FString("CancelTarget"), AbilityEnumAssetPath, static_cast<int32>(EAbilityInputID::Confirm), static_cast<int32>(EAbilityInputID::Cancel)));

		ASCInputBound = true;
	}
}

void ATPSCharacter::InitializeAttributes(ATPSPlayerState* PS)
{
	if (!AbilitySystemComponent.IsValid())
	{
		return;
	}
	if (!DefaultAttributes)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Missing DefaultAttributes for %s. Please fill in the characers blueprint."), *FString(__FUNCTION__), *GetName());
	}

	AttributeSet = PS->GetCharacterAttributeSet();


	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle NewHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributes, Level, EffectContext);

	if (NewHandle.IsValid())
	{
		FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), AbilitySystemComponent.Get());
	}
}

void ATPSCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
}

UAbilitySystemComponent* ATPSCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent.Get();
}

float ATPSCharacter::GetHealth() const
{
	if (AttributeSet.IsValid())
	{
		return AttributeSet->GetHealth();
	}
	return 0.0f;
}


//////////////////////////////////////////////////////////////////////////
// Input

void ATPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Running
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ATPSCharacter::Sprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ATPSCharacter::StopSprint);

		// Fire
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ATPSCharacter::Fire);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ATPSCharacter::StopFire);

		// Scope
		EnhancedInputComponent->BindAction(ScopeAction, ETriggerEvent::Started, this, &ATPSCharacter::Scope);
		EnhancedInputComponent->BindAction(ScopeAction, ETriggerEvent::Completed, this, &ATPSCharacter::StopScope);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATPSCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATPSCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}

	BindASCInput();
}

void ATPSCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ATPSCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ATPSCharacter::SendAbilityLocalInput(const FInputActionValue& Value, int32 AbilityID)
{
	if (!AbilitySystemComponent.IsValid())
	{
		return;
	}
	bool Pressed = Value.Get<bool>();
	if (Pressed)
	{
		AbilitySystemComponent->AbilityLocalInputPressed(AbilityID);
	}
	else
	{
		AbilitySystemComponent->AbilityLocalInputReleased(AbilityID);
	}
}

void ATPSCharacter::Sprint(const FInputActionValue& Value)
{
	if (!IsAlive())
	{
		return;
	}

	SendAbilityLocalInput(Value, static_cast<int32>(EAbilityInputID::Sprint));
}

void ATPSCharacter::StopSprint(const FInputActionValue& Value)
{
	if (!IsAlive())
	{
		return;
	}

	SendAbilityLocalInput(Value, static_cast<int32>(EAbilityInputID::Sprint));
}

void ATPSCharacter::Fire(const FInputActionValue& Value)
{
	if (!IsAlive())
	{
		return;
	}

	SendAbilityLocalInput(Value, static_cast<int32>(EAbilityInputID::Fire));
}

void ATPSCharacter::StopFire(const FInputActionValue& Value)
{
	if (!IsAlive())
	{
		return;
	}

	SendAbilityLocalInput(Value, static_cast<int32>(EAbilityInputID::Fire));
}

void ATPSCharacter::Scope(const FInputActionValue& Value)
{
	if (!IsAlive())
	{
		return;
	}

	SendAbilityLocalInput(Value, static_cast<int32>(EAbilityInputID::Scope));
}

void ATPSCharacter::StopScope(const FInputActionValue& Value)
{
	if (!IsAlive())
	{
		return;
	}

	SendAbilityLocalInput(Value, static_cast<int32>(EAbilityInputID::Scope));
}
