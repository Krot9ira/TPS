// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "CharacterAttributeSet.h"
#include "TPSPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class TPS_API ATPSPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
private:
	/** Ability system component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ASC", meta = (AllowPrivateAccess = "true"))
	class UAbilitySystemComponent* AbilitySystemComponent;

protected:
	UPROPERTY(Transient)
	UCharacterAttributeSet* AttributeSet;

public:
	ATPSPlayerState();

	class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	class UCharacterAttributeSet* GetCharacterAttributeSet() const;
};
