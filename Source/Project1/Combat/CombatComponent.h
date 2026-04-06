#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FCombatTypes.h"
#include "CombatComponent.generated.h"

class ABaseProjectile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUnitDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, NewHealthPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCellDeath, int32, CellIndex, int32, CellsRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCellHealthChanged, int32, CellIndex, float, NewCellHealthPercent, int32, CellsAlive);

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

    // --- Cell State (for Team units) ---
    /** HP of each individual cell */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cells")
    TArray<float> CellHealths;

    /** Number of cells currently alive */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cells")
    int32 CellsAlive = 0;

    /** HP per cell (set on BeginPlay) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cells")
    float CellMaxHealth = 0.f;

    /** Damage per cell */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Cells")
    float CellDamage = 0.f;

    /** Whether this unit is a Team unit */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat|Cells")
    bool IsTeamUnit() const { return Stats.AttackPattern == EAttackPattern::Team; }

    // --- Projectile ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Projectile")
    TSubclassOf<ABaseProjectile> ProjectileClass;

    // --- Events ---
    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnUnitDeath OnUnitDeath;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnHealthChanged OnHealthChanged;

    /** Fired when a cell dies — use in BP to hide the corresponding visual */
    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnCellDeath OnCellDeath;

    /** Fired when a cell takes damage — use in BP to update per-cell HP bars */
    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnCellHealthChanged OnCellHealthChanged;

    // --- Functions ---
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

    UFUNCTION(BlueprintCallable, Category = "Combat|TypeEffectiveness")
    static float GetPatternEffectiveness(EAttackPattern Attacker, EAttackPattern Defender);

    /** Returns the combined multiplier (unit type * attack pattern) */
    UFUNCTION(BlueprintCallable, Category = "Combat|TypeEffectiveness")
    static float GetCombinedEffectiveness(ECharType AtkType, EAttackPattern AtkPattern, ECharType DefType, EAttackPattern DefPattern);

    /** Auto-configures stats based on CharType and AttackPattern */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ApplyAutoStats();

protected:
    virtual void BeginPlay() override;

private:
    void ResetAttackCooldown();
    void StartCooldown();

    /** Deal damage to a single target, routing through cell logic if target is Team unit */
    void DealDamageToTarget(AActor* Target);

    /** Apply damage to a specific cell of this Team unit. Returns true if cell died. */
    bool ApplyDamageToCell(int32 CellIndex, float DamageAmount);

    /** Find the first living cell index */
    int32 GetFirstLivingCellIndex() const;

    /** Get all living enemies */
    TArray<AActor*> GetAllEnemies() const;

    /** Get enemies within a radius of a location */
    TArray<AActor*> GetEnemiesInRadius(FVector Center, float Radius) const;

    FTimerHandle CooldownTimerHandle;
};
