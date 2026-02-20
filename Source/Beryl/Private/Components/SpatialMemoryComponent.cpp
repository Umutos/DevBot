// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SpatialMemoryComponent.h"
#include "JackalRobot.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"


// Sets default values for this component's properties
USpatialMemoryComponent::USpatialMemoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f; // check twice per second, not every frame
}

void USpatialMemoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAutoExploreCells)
	{
		FIntVector CurrentCell = GetCurrentCell();
		// Only call ExploreCell if the cell changed (avoid spam)
		if (CurrentCell != LastExploredCell)
		{
			ExploreCell(CurrentCell);
			LastExploredCell = CurrentCell;
		}
	}

	if (bDrawDebugGrid)
	{
		DrawDebugGridAroundOwner();
	}
}

void USpatialMemoryComponent::DrawDebugGridAroundOwner()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FIntVector CurrentCell = GetCurrentCell();

	for (int32 dx = -DebugGridRadius; dx <= DebugGridRadius; dx++)
	{
		for (int32 dy = -DebugGridRadius; dy <= DebugGridRadius; dy++)
		{
			FIntVector CellCoord(CurrentCell.X + dx, CurrentCell.Y + dy, CurrentCell.Z);
			FVector CellCenter = CellToWorld(CellCoord);

			// Cell corners (on XY plane)
			FVector HalfSize(CellSize.X * 0.5f, CellSize.Y * 0.5f, 0.f);
			FVector Min = CellCenter - HalfSize;
			FVector Max = CellCenter + HalfSize;
			float Z = CellCenter.Z;

			bool bExplored = IsCellExplored(CellCoord);
			FColor Color = bExplored ? FColor::Green : FColor::Red;

			// Is it the current cell?
			if (dx == 0 && dy == 0)
			{
				Color = FColor::Cyan;
			}

			// Draw the 4 edges of the cell
			DrawDebugLine(World, FVector(Min.X, Min.Y, Z), FVector(Max.X, Min.Y, Z), Color, false, 0.6f, 0, 1.5f);
			DrawDebugLine(World, FVector(Max.X, Min.Y, Z), FVector(Max.X, Max.Y, Z), Color, false, 0.6f, 0, 1.5f);
			DrawDebugLine(World, FVector(Max.X, Max.Y, Z), FVector(Min.X, Max.Y, Z), Color, false, 0.6f, 0, 1.5f);
			DrawDebugLine(World, FVector(Min.X, Max.Y, Z), FVector(Min.X, Min.Y, Z), Color, false, 0.6f, 0, 1.5f);
		}
	}
}

void USpatialMemoryComponent::AddSpatialMemoryElement(FSpatialMemoryElement NewElement)
{
	// Avoid duplicates: if an element with the same ID already exists, update it
	for (int32 i = 0; i < SpatialItemsMemory.Num(); i++)
	{
		if (SpatialItemsMemory[i].ElementID == NewElement.ElementID)
		{
			// Update with newer data if timestamp is more recent
			if (NewElement.Timestamp >= SpatialItemsMemory[i].Timestamp)
			{
				SpatialItemsMemory[i] = NewElement;
				OnMapUpdated.Broadcast();
			}
			return;
		}
	}

	SpatialItemsMemory.Add(NewElement);
	OnMapUpdated.Broadcast();
}

void USpatialMemoryComponent::StoreDetectedActorsArray(TArray<AActor*> DetectedActorsArray)
{
	for (int i = 0; i < DetectedActorsArray.Num(); i++)
	{
		StoreDetectedActor(DetectedActorsArray[i]);
	}
}

void USpatialMemoryComponent::StoreDetectedActor(AActor* DetectedActor)
{
	// Safety check
	if (!DetectedActor) return;

	AJackalInteractiveProp* InteractiveProp = Cast<AJackalInteractiveProp>(DetectedActor);
	if (!InteractiveProp) return;

	if (InteractiveProp->GetPropType() == EInteractivePropType::Unknown)
		return;

	FSpatialMemoryElement NewElement;
	NewElement.Timestamp = GetWorld()->GetTimeSeconds();
	NewElement.ElementID = DetectedActor->GetUniqueID();

	if (AJackalRobot* DetectedJackalRobot = Cast<AJackalRobot>(DetectedActor))
	{
		NewElement.ElementReward = 0.0f;
		NewElement.DetectedObject = EInteractivePropType::JackalRobot;
		NewElement.Location = DetectedJackalRobot->GetBodyMesh()->GetComponentLocation();
	}
	else
	{
		NewElement.ElementReward = InteractiveProp->GetReward();
		NewElement.DetectedObject = InteractiveProp->GetPropType();
		NewElement.Location = InteractiveProp->GetActorLocation();
	}

	AddSpatialMemoryElement(NewElement);

	// Also mark the cell where the object was found as explored
	FIntVector ObjectCell = WorldToCell(NewElement.Location);
	ExploreCell(ObjectCell);
}


void USpatialMemoryComponent::MergeSpatialMemoryMaps(TArray<FSpatialMemoryElement> OtherMap)
{
	for (const FSpatialMemoryElement& OtherElement : OtherMap)
	{
		// Check if we already know this element
		bool bAlreadyKnown = false;
		for (int32 i = 0; i < SpatialItemsMemory.Num(); i++)
		{
			if (SpatialItemsMemory[i].ElementID == OtherElement.ElementID)
			{
				bAlreadyKnown = true;
				// Keep the most recent observation
				if (OtherElement.Timestamp > SpatialItemsMemory[i].Timestamp)
				{
					SpatialItemsMemory[i] = OtherElement;
				}
				break;
			}
		}

		if (!bAlreadyKnown)
		{
			SpatialItemsMemory.Add(OtherElement);
		}
	}

	OnMapUpdated.Broadcast();
}

TArray<FVector> USpatialMemoryComponent::GetAllPropsTypePos(EInteractivePropType type)
{
	TArray<FVector> allProp;
	for (int i = 0; i < SpatialItemsMemory.Num(); i++)
	{
		if (SpatialItemsMemory[i].DetectedObject == type)
			allProp.Add(SpatialItemsMemory[i].Location);
	}

	return allProp;
}

TArray<FSpatialMemoryElement> USpatialMemoryComponent::GetSpatialItemsMemory()
{
	return SpatialItemsMemory;
}

FIntVector USpatialMemoryComponent::WorldToCell(const FVector& WorldLocation) const
{
	return FIntVector(
		FMath::FloorToInt(WorldLocation.X / CellSize.X),
		FMath::FloorToInt(WorldLocation.Y / CellSize.Y),
		FMath::FloorToInt(WorldLocation.Z / CellSize.Z)
	);
}

FVector USpatialMemoryComponent::CellToWorld(const FIntVector& Cell) const
{
	return FVector(
		Cell.X * CellSize.X + CellSize.X * 0.5f,
		Cell.Y * CellSize.Y + CellSize.Y * 0.5f,
		Cell.Z * CellSize.Z + CellSize.Z * 0.5f
	);
}

void USpatialMemoryComponent::ExploreCell(FIntVector CellID)
{
	// Check if cell already exists in the grid
	for (FSpatialGridCell& Cell : SpatialGrid)
	{
		if (Cell.CellCoordinates == CellID)
		{
			Cell.bIsCellExplored = true;
			return;
		}
	}

	// Cell not found, create a new one
	FSpatialGridCell NewCell;
	NewCell.CellCoordinates = CellID;
	NewCell.bIsCellExplored = true;
	SpatialGrid.Add(NewCell);

	OnMapUpdated.Broadcast();
}

FIntVector USpatialMemoryComponent::GetCurrentCell()
{
	AActor* Owner = GetOwner();
	if (!Owner) return FIntVector::ZeroValue;

	// For physics-driven pawns like JackalRobot, GetActorLocation() may not
	// reflect the real position. Use BodyMesh component location instead.
	if (AJackalRobot* Jackal = Cast<AJackalRobot>(Owner))
	{
		if (Jackal->GetBodyMesh())
		{
			return WorldToCell(Jackal->GetBodyMesh()->GetComponentLocation());
		}
	}

	return WorldToCell(Owner->GetActorLocation());
}

bool USpatialMemoryComponent::IsCellExplored(const FIntVector& CellID) const
{
	for (const FSpatialGridCell& Cell : SpatialGrid)
	{
		if (Cell.CellCoordinates == CellID)
		{
			return Cell.bIsCellExplored;
		}
	}
	return false;
}

FIntVector USpatialMemoryComponent::FindNearestUnexploredCell() const
{
	AActor* Owner = GetOwner();
	if (!Owner) return FIntVector::ZeroValue;

	FIntVector CurrentCell = const_cast<USpatialMemoryComponent*>(this)->GetCurrentCell();
	
	// Récupérer le système de navigation
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());

	// Recherche en spirale vers l'extérieur
	int32 BestDistance = MAX_int32;
	FIntVector BestCell = CurrentCell;
	bool bFoundUnexplored = false;

	const int32 MaxSearchRadius = 20;
	for (int32 Radius = 1; Radius <= MaxSearchRadius; Radius++)
	{
		for (int32 dx = -Radius; dx <= Radius; dx++)
		{
			for (int32 dy = -Radius; dy <= Radius; dy++)
			{
				// Ne vérifier que le périmètre
				if (FMath::Abs(dx) != Radius && FMath::Abs(dy) != Radius)
					continue;

				FIntVector Candidate(CurrentCell.X + dx, CurrentCell.Y + dy, CurrentCell.Z);

				if (!IsCellExplored(Candidate))
				{
					// Convertir en position monde pour tester le NavMesh
					FVector WorldPos = CellToWorld(Candidate);
					FNavLocation ProjectedLoc;
					
					// Tolérance de projection (la moitié de la cellule)
					FVector ProjectionExtent(CellSize.X * 0.5f, CellSize.Y * 0.5f, CellSize.Z);

					bool bIsNavigable = false;
					if (NavSys)
					{
						bIsNavigable = NavSys->ProjectPointToNavigation(WorldPos, ProjectedLoc, ProjectionExtent);
					}

					if (bIsNavigable)
					{
						// La cellule est atteignable ! On calcule la distance.
						int32 Dist = FMath::Abs(dx) + FMath::Abs(dy); // Distance de Manhattan
						if (Dist < BestDistance)
						{
							BestDistance = Dist;
							BestCell = Candidate;
							bFoundUnexplored = true;
						}
					}
					else
					{
						const_cast<USpatialMemoryComponent*>(this)->ExploreCell(Candidate);
					}
				}
			}
		}

		if (bFoundUnexplored) break;
	}

	return BestCell;
}

TArray<FSpatialGridCell> USpatialMemoryComponent::GetSpatialGrid()
{
	return SpatialGrid;
}