#include "BTService_UpdateCombat.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Project1/Combat/CombatComponent.h"

UBTService_UpdateCombat::UBTService_UpdateCombat()
{
    NodeName = "Update Combat Status";
    Interval = 0.25f;
    RandomDeviation = 0.05f;

    TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombat, TargetActorKey), AActor::StaticClass());
    IsInAttackRangeKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombat, IsInAttackRangeKey));
    CanAttackKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombat, CanAttackKey));
}

void UBTService_UpdateCombat::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC) return;

    APawn* Pawn = AIC->GetPawn();
    if (!Pawn) return;

    UCombatComponent* Combat = Pawn->FindComponentByClass<UCombatComponent>();
    if (!Combat) return;

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return;

    // Get current target from blackboard
    AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));

    if (Target)
    {
        // Check if target is dead — if so, clear it
        UCombatComponent* TargetCombat = Target->FindComponentByClass<UCombatComponent>();
        if (TargetCombat && TargetCombat->bIsDead)
        {
            BB->ClearValue(TargetActorKey.SelectedKeyName);
            Combat->CurrentTarget = nullptr;
            BB->SetValueAsBool(IsInAttackRangeKey.SelectedKeyName, false);
            BB->SetValueAsBool(CanAttackKey.SelectedKeyName, false);
            return;
        }

        // Update range and cooldown status
        bool bInRange = Combat->IsInAttackRange(Target);
        BB->SetValueAsBool(IsInAttackRangeKey.SelectedKeyName, bInRange);
        BB->SetValueAsBool(CanAttackKey.SelectedKeyName, Combat->bCanAttack);

        float Distance = FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation());
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 0.3f, FColor::White,
                FString::Printf(TEXT("%s → %s | Dist: %.0f | Range: %.0f | InRange: %d | CanAttack: %d"),
                    *Pawn->GetName(), *Target->GetName(), Distance, Combat->Stats.AttackRange, bInRange, Combat->bCanAttack));
        }
    }
    else
    {
        BB->SetValueAsBool(IsInAttackRangeKey.SelectedKeyName, false);
        BB->SetValueAsBool(CanAttackKey.SelectedKeyName, false);
    }
}