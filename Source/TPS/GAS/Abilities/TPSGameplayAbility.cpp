// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSGameplayAbility.h"
#include "AbilitySystemComponent.h"

UTPSGameplayAbility::UTPSGameplayAbility()
{
}

void UTPSGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	if (ActivateAbilityOnGranted)
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);
	}
}
