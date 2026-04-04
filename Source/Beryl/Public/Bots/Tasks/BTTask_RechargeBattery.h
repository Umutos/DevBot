#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BTTask_RechargeBattery.generated.h"

UCLASS()
class BERYL_API UBTTask_RechargeBattery : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_RechargeBattery();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector ChargingStationLocationKey;

	UPROPERTY(EditAnywhere, Category = "Config")
	float AcceptanceRadius = 150.f;

	bool bIsCharging = false;
};