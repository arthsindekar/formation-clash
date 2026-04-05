#include "CombatComponent.h"
#include "BaseProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = Stats.MaxHealth;

    // Apply movement speed to owner if it's a Character
    if (ACharacter* CharOwner = Cast<ACharacter>(GetOwner()))
    {
        if (UCharacterMovementComponent* Movement = CharOwner->GetCharacterMovement())
        {
            Movement->MaxWalkSpeed = Stats.MovementSpeed;
        }
    }

}

float UCombatComponent::GetHealthPercent() const
{
    return (Stats.MaxHealth > 0.f) ? CurrentHealth / Stats.MaxHealth : 0.f;
}

float UCombatComponent::GetTypeEffectiveness(ECharType Attacker, ECharType Defender)
{
    // Melee beats Ranged, Ranged beats Flying, Flying beats Melee
    if ((Attacker == ECharType::Melee  && Defender == ECharType::Ranged) ||
        (Attacker == ECharType::Ranged && Defender == ECharType::Flying) ||
        (Attacker == ECharType::Flying && Defender == ECharType::Melee))
    {
        return 1.5f; // Super effective
    }

    if ((Attacker == ECharType::Melee  && Defender == ECharType::Flying) ||
        (Attacker == ECharType::Ranged && Defender == ECharType::Melee) ||
        (Attacker == ECharType::Flying && Defender == ECharType::Ranged))
    {
        return 0.75f; // Not very effective
    }

    return 1.0f; // Neutral
}

void UCombatComponent::ApplyDamage(float DamageAmount, AActor* DamageInstigator)
{
    if (bIsDead) return;

    // Apply type effectiveness if instigator has a CombatComponent
    if (DamageInstigator)
    {
        if (UCombatComponent* InstigatorCombat =
            DamageInstigator->FindComponentByClass<UCombatComponent>())
        {
            float Multiplier = GetTypeEffectiveness(
                InstigatorCombat->Stats.CharType, Stats.CharType);
            DamageAmount *= Multiplier;
        }
    }

    CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, Stats.MaxHealth);
    OnHealthChanged.Broadcast(GetHealthPercent());

    if (CurrentHealth <= 0.f)
    {
        bIsDead = true;

        // Stop the behavior tree
        APawn* PawnOwner = Cast<APawn>(GetOwner());
        if (PawnOwner)
        {
            if (AAIController* AIC = Cast<AAIController>(PawnOwner->GetController()))
            {
                if (AIC->GetBrainComponent())
                {
                    AIC->GetBrainComponent()->StopLogic("Dead");
                }
            }
        }

        // Broadcast death event (bind in BP to play death anim, disable collision, etc.)
        OnUnitDeath.Broadcast();
    }
}

bool UCombatComponent::IsInAttackRange(AActor* Target) const
{
    if (!Target || !GetOwner()) return false;
    float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation());
    return Distance <= Stats.AttackRange;
}

bool UCombatComponent::Attack()
{
    if (!bCanAttack || bIsDead || !CurrentTarget) return false;

    if (!IsInAttackRange(CurrentTarget)) return false;

    if (Stats.CharType == ECharType::Melee)
    {
        MeleeAttack();
    }
    else
    {
        FireProjectile();
    }

    return true;
}

void UCombatComponent::MeleeAttack()
{
    if (!bCanAttack || bIsDead || !CurrentTarget) return;

    AActor* Owner = GetOwner();
    if (!Owner) return;

    // Deal direct damage to the target
    UCombatComponent* TargetCombat = CurrentTarget->FindComponentByClass<UCombatComponent>();
    if (TargetCombat && !TargetCombat->bIsDead)
    {
        TargetCombat->ApplyDamage(Stats.AttackDamage, Owner);

        /*if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
                FString::Printf(TEXT("%s melee attacked %s for %.0f base damage"),
                    *Owner->GetName(), *CurrentTarget->GetName(), Stats.AttackDamage));
        }*/
    }

    // Start cooldown
    bCanAttack = false;
    GetWorld()->GetTimerManager().SetTimer(
        CooldownTimerHandle, this,
        &UCombatComponent::ResetAttackCooldown,
        Stats.AttackCooldown, false);
}

void UCombatComponent::FireProjectile()
{
    if (GEngine)
    {
        /*GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
            FString::Printf(TEXT("FireProjectile called - CanAttack: %d, IsDead: %d, HasTarget: %d, HasClass: %d"),
                bCanAttack, bIsDead, CurrentTarget != nullptr, ProjectileClass != nullptr));*/
    }
    
    if (!bCanAttack || bIsDead || !CurrentTarget || !ProjectileClass) return;

    AActor* Owner = GetOwner();
    if (!Owner) return;

    // Spawn location: chest height offset
    FVector SpawnLocation = Owner->GetActorLocation() + FVector(0.f, 0.f, 50.f);

    // Aim at target
    FVector TargetLocation = CurrentTarget->GetActorLocation() + FVector(0.f, 0.f, 50.f);
    FRotator SpawnRotation = (TargetLocation - SpawnLocation).Rotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Owner;
    SpawnParams.Instigator = Cast<APawn>(Owner);
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    if (GEngine)
    {
        /*GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange,
            FString::Printf(TEXT("Spawning class: %s at location: %s"),
                *ProjectileClass->GetName(), *SpawnLocation.ToString()));
        UE_LOG(LogTemp, Warning, TEXT("Spawning class: %s at location: %s"),
    *ProjectileClass->GetName(), *SpawnLocation.ToString());*/
    }

    AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(
    ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

    if (SpawnedActor)
    {
        ABaseProjectile* Projectile = Cast<ABaseProjectile>(SpawnedActor);
        if (Projectile)
        {
            Projectile->SetOwnerUnit(Owner, Stats.AttackDamage);
            /*if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Projectile spawned and cast succeeded!"));
            }*/
        }
        else
        {
            /*if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
                    FString::Printf(TEXT("Spawned but cast failed! Actual class: %s"),
                        *SpawnedActor->GetClass()->GetName()));
            }*/
        }
    }
    else
    {
        if (GEngine)
        {
            /*GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Spawn returned null!"));*/
        }
    }

    // Start cooldown
    bCanAttack = false;
    GetWorld()->GetTimerManager().SetTimer(
        CooldownTimerHandle, this,
        &UCombatComponent::ResetAttackCooldown,
        Stats.AttackCooldown, false);
}


void UCombatComponent::ResetAttackCooldown()
{
    bCanAttack = true;
}

AActor* UCombatComponent::FindBestTarget() const
{
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), GetOwner()->GetClass(), AllUnits);

    AActor* BestTarget = nullptr;
    float BestScore = -FLT_MAX;
    FVector MyLocation = GetOwner()->GetActorLocation();

    for (AActor* Unit : AllUnits)
    {
        if (Unit == GetOwner()) continue;

        UCombatComponent* OtherCombat = Unit->FindComponentByClass<UCombatComponent>();
        if (!OtherCombat || OtherCombat->TeamID == TeamID || OtherCombat->bIsDead)
            continue;

        float Score = 0.f;

        // Type effectiveness bonus (0 to 30)
        float Effectiveness = GetTypeEffectiveness(Stats.CharType, OtherCombat->Stats.CharType);
        Score += (Effectiveness - 0.75f) * 40.f; // Maps 0.75->0, 1.0->10, 1.5->30

        // Distance penalty (closer = better)
        float Distance = FVector::Dist(MyLocation, Unit->GetActorLocation());
        float MaxRange = 5000.f;
        Score += (1.f - FMath::Clamp(Distance / MaxRange, 0.f, 1.f)) * 20.f;

        // Low health bonus (finishing off weak targets)
        float HealthPct = OtherCombat->GetHealthPercent();
        Score += (1.f - HealthPct) * 15.f;

        if (Score > BestScore)
        {
            BestScore = Score;
            BestTarget = Unit;
        }
    }
    
    if (GEngine)
    {
        /*GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
            FString::Printf(TEXT("FindBestTarget: Result = %s"),
                BestTarget ? *BestTarget->GetName() : TEXT("NONE")));*/
    }

    return BestTarget;
}