// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSPlayerState.h"
#include "AbilitySystemComponent.h"

ATPSPlayerState::ATPSPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UCharacterAttributeSet>(TEXT("AttributeSet"));

	NetUpdateFrequency = 100.0f;
}

UAbilitySystemComponent* ATPSPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UCharacterAttributeSet* ATPSPlayerState::GetCharacterAttributeSet() const
{
	return AttributeSet;
}