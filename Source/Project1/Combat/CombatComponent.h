#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FCombatTypes.h"
#include "CombatComponent.generated.h"

class ABaseProjectile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUnitDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, NewHealthPercent);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECT1_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatComponent();

    // --- Stats ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
    FUnitStats Stats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Team")
    int32 TeamID = 0;

    // --- Runtime State ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State")
    float CurrentHealth;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State")
    bool bIsDead = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State")
    bool bCanAttack = true;

    UPROPERTY(BlueprintReadWrite, Category = "Combat|State")
    AActor* CurrentTarget = nullptr;

    // --- Projectile ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Projectile")
    TSubclassOf<ABaseProjectile> ProjectileClass;

    // --- Events (bind in Blueprint for animations, UI, etc.) ---
    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnUnitDeath OnUnitDeath;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnHealthChanged OnHealthChanged;

    // --- Functions ---
    /** Unified attack: melee units deal direct damage, ranged/flying units fire a projectile. Returns true if the attack was executed. */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool Attack();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void FireProjectile();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void MeleeAttack();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ApplyDamage(float DamageAmount, AActor* DamageInstigator);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool IsInAttackRange(AActor* Target) const;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    AActor* FindBestTarget() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
    float GetHealthPercent() const;

    // --- Static Helpers ---
    UFUNCTION(BlueprintCallable, Category = "Combat|TypeEffectiveness")
    static float GetTypeEffectiveness(ECharType Attacker, ECharType Defender);

protected:
    virtual void BeginPlay() override;

private:
    void ResetAttackCooldown();
    FTimerHandle CooldownTimerHandle;
};


