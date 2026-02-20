#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Props/JackalInteractiveProp.h"
#include "SpatialMemoryComponent.generated.h"

class AJackalRobot;


USTRUCT(BlueprintType)
struct FSpatialGridCell
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntVector CellCoordinates = FIntVector::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsCellExplored = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> CellPropIDs;
};

USTRUCT(BlueprintType)
struct FSpatialMemoryElement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EInteractivePropType DetectedObject = EInteractivePropType::Unknown;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ElementReward = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int ElementID = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Timestamp = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(
	FOnMapUpdated
);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BERYL_API USpatialMemoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USpatialMemoryComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** If true, automatically marks cells as explored while the owner moves */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpatialMemory|Grid")
	bool bAutoExploreCells = true;

	/** If true, draws the grid around the robot (green=explored, red=unexplored, cyan=current) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpatialMemory|Debug")
	bool bDrawDebugGrid = true;

	/** How many cells around the robot to draw */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpatialMemory|Debug")
	int32 DebugGridRadius = 5;

	UPROPERTY(BlueprintAssignable, Category = "SpatialMemory")
	FOnMapUpdated OnMapUpdated;

	UFUNCTION(BlueprintCallable, Category = "SpatialMemory|Objects")
	void AddSpatialMemoryElement(FSpatialMemoryElement NewElement);

	UFUNCTION(BlueprintCallable, Category = "SpatialMemory|Objects")
	void StoreDetectedActorsArray(TArray<AActor*> DetectedActorsArray);

	UFUNCTION(BlueprintCallable, Category = "SpatialMemory|Objects")
	void StoreDetectedActor(AActor* DetectedActor);

	UFUNCTION(BlueprintCallable, Category = "SpatialMemory|Objects")
	void MergeSpatialMemoryMaps(TArray<FSpatialMemoryElement> OtherMap);

	UFUNCTION(BlueprintCallable, Category = "SpatialMemory|Objects")
	TArray<FVector> GetAllPropsTypePos(EInteractivePropType type);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpatialMemory|Objects")
	TArray<FSpatialMemoryElement> GetSpatialItemsMemory();

	/** Mark a cell as explored in the spatial grid */
	UFUNCTION(BlueprintCallable, Category = "SpatialMemory|Grid")
	void ExploreCell(FIntVector CellID);

	/** Get the grid cell the owner is currently in */
	UFUNCTION(BlueprintCallable, Category = "SpatialMemory|Grid")
	FIntVector GetCurrentCell();

	/** Check if a specific cell has been explored */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpatialMemory|Grid")
	bool IsCellExplored(const FIntVector& CellID) const;

	/** Find the nearest unexplored cell using spiral search */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpatialMemory|Grid")
	FIntVector FindNearestUnexploredCell() const;

	/** Convert a world location to grid cell coordinates */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpatialMemory|Grid")
	FIntVector WorldToCell(const FVector& WorldLocation) const;

	/** Convert grid cell coordinates to world center position */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpatialMemory|Grid")
	FVector CellToWorld(const FIntVector& Cell) const;

	UFUNCTION(BlueprintCallable)
	TArray<FSpatialGridCell> GetSpatialGrid();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SpatialMemory|Objects")
	TArray<FSpatialMemoryElement> SpatialItemsMemory;

	UPROPERTY(EditDefaultsOnly)
	FIntVector CellSize = FIntVector(500, 500, 100); // Default cell size is 5 by 5 by 1 meters

	UPROPERTY(BlueprintReadOnly)
	TArray<FSpatialGridCell> SpatialGrid;

	/** Tracks the last cell marked as explored to avoid redundant calls */
	FIntVector LastExploredCell = FIntVector(MAX_int32, MAX_int32, MAX_int32);

	/** Draws debug grid lines around the owner */
	void DrawDebugGridAroundOwner();

};