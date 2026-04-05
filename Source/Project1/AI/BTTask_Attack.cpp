#include "BTTask_Attack.h"
#include "AIController.h"
#include "Project1/Combat/CombatComponent.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = "Attack";
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) { UE_LOG(LogTemp, Error, TEXT("Attack: No AIC")); return EBTNodeResult::Failed; }

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn) { UE_LOG(LogTemp, Error, TEXT("Attack: No Pawn")); return EBTNodeResult::Failed; }

	UCombatComponent* Combat = Pawn->FindComponentByClass<UCombatComponent>();
	if (!Combat) { UE_LOG(LogTemp, Error, TEXT("Attack: No CombatComponent")); return EBTNodeResult::Failed; }

	if (!Combat->bCanAttack)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attack: %s can't attack (cooldown)"), *Pawn->GetName());
		return EBTNodeResult::Failed;
	}

	if (Combat->bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attack: %s is dead"), *Pawn->GetName());
		return EBTNodeResult::Failed;
	}

	bool bResult = Combat->Attack();
	UE_LOG(LogTemp, Warning, TEXT("Attack: %s Attack() returned %d"), *Pawn->GetName(), bResult);
	return bResult ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
