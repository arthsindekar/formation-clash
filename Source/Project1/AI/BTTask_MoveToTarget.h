#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToTarget.generated.h"

UCLASS()
class PROJECT1_API UBTTask_MoveToTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveToTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	/** How close the unit needs to get before the task succeeds */
	UPROPERTY(EditAnywhere, Category = "Movement")
	float AcceptableRadius = 100.f;
};
