#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CreatureDefinition.generated.h"

/**
 * Defines the static data for a Creature Species (e.g., "Katraji").
 */
UCLASS(BlueprintType)
class CREATURESYSTEM_API UCreatureDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
    // Unique ID or Name for the creature species
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature Data")
    FName SpeciesName;

    // Base Max HP for calculation (simplified)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature Data")
    float BaseMaxHP;

    // Mapping of Minimum Level to Actor Class for spawning/evolution.
    // Example: Key=1 -> BabyClass, Key=20 -> AdultClass.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature Data")
    TMap<int32, TSubclassOf<AActor>> EvolutionMap;

    /**
     * Helper to get the correct class for a given level.
     * Iterates to find the highest key <= Level.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Data")
    TSubclassOf<AActor> GetActorClassForLevel(int32 Level) const
    {
        TSubclassOf<AActor> SelectedClass = nullptr;
        int32 HighestFoundLevel = -1;

        for (const auto& Elem : EvolutionMap)
        {
            if (Elem.Key <= Level && Elem.Key > HighestFoundLevel)
            {
                HighestFoundLevel = Elem.Key;
                SelectedClass = Elem.Value;
            }
        }
        return SelectedClass;
    }
};
