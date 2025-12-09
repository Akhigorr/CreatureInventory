#pragma once

#include "CoreMinimal.h"
#include "CreatureDefinition.h"
#include "CreatureTypes.generated.h"

/**
 * Represents a unique instance of a caught creature.
 */
USTRUCT(BlueprintType)
struct FCreatureInstance
{
	GENERATED_BODY()

public:
    // The static species definition (e.g., "Katraji")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Instance")
    UCreatureDefinition* CreatureDefinition;

    // Unique Identifier for this specific instance (optional, but good for map keys if needed,
    // though the prompt implies Species Uniqueness, so Definition->SpeciesName might be enough.
    // However, keeping an Instance ID is safer.
    // Wait, the prompt said "capture one of even creature", implying Species Uniqueness.
    // So CreatureDefinition->SpeciesName is likely the unique Key.)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Instance")
    int32 CurrentLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Instance")
    float CurrentHP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Instance")
    float MaxHP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Instance")
    bool bIsDead;

    FCreatureInstance()
        : CreatureDefinition(nullptr)
        , CurrentLevel(1)
        , CurrentHP(100.f)
        , MaxHP(100.f)
        , bIsDead(false)
    {}

    // Equality operator for TArray::Contains or generic checks
    bool operator==(const FCreatureInstance& Other) const
    {
        return CreatureDefinition == Other.CreatureDefinition;
    }
};
