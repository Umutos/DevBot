#include "Bots/Tasks/BTTask_RechargeBattery.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "JackalRobot.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_RechargeBattery::UBTTask_RechargeBattery()
{
	NodeName = "Recharge Battery";
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_RechargeBattery::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	
	if (!AIController) 
		return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	
	if (!BB) 
		return EBTNodeResult::Failed;

	FVector StationLocation = BB->GetValueAsVector(ChargingStationLocationKey.SelectedKeyName);

	if (StationLocation.IsNearlyZero())
	{
		UE_LOG(LogTemp, Warning, TEXT("RechargeBattery: Aucune station de recharge connue !"));
		return EBTNodeResult::Failed;
	}

	bIsCharging = false;

	EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
		StationLocation,
		AcceptanceRadius,
		true,
		true,
		false,
		true,
		nullptr,
		true
	);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		UE_LOG(LogTemp, Warning, TEXT("RechargeBattery: Navigation vers la station échouée."));
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_RechargeBattery::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AJackalRobot* Jackal = Cast<AJackalRobot>(AIController->GetPawn());
	if (!Jackal)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (!bIsCharging)
	{
		EPathFollowingStatus::Type MoveStatus = AIController->GetMoveStatus();
		
		if (MoveStatus == EPathFollowingStatus::Type::Idle)
		{
			bIsCharging = true;
			UE_LOG(LogTemp, Log, TEXT("RechargeBattery: Arrivé à la station, début de recharge."));
		}
		else if (MoveStatus == EPathFollowingStatus::Type::Moving)
			return;
	}

	if (bIsCharging)
	{
		Jackal->RechargeBattery(DeltaSeconds);

		if (Jackal->BatteryState == EBatteryState::BatteryFull)
		{
			UE_LOG(LogTemp, Log, TEXT("RechargeBattery: Batterie pleine !"));
			bIsCharging = false;
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
}