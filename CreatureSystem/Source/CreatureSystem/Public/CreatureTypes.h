#pragma once

#include "CoreMinimal.h"
#include "CreatureDefinition.h"
#include "CreatureTypes.generated.h"

UENUM(BlueprintType)
enum class ECreatureSortMethod : uint8
{
    Newest      UMETA(DisplayName = "Newest First"),
    Oldest      UMETA(DisplayName = "Oldest First"),
    LevelHigh   UMETA(DisplayName = "Highest Level"),
    LevelLow    UMETA(DisplayName = "Lowest Level")
};

/**
 * Represents a unique instance of a caught creature.
 */
USTRUCT(BlueprintType)
struct FCreatureInstance
{
	GENERATED_BODY()

public:
    // The static species definition (e.g., "Katraji")
    // MEM_01: Use TObjectPtr for UPROPERTY
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Instance")
    TObjectPtr<UCreatureDefinition> CreatureDefinition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Instance")
    int32 CurrentLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Instance")
    float CurrentHP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Instance")
    float MaxHP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Instance")
    float CurrentXP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Instance")
    float XPToNextLevel;

    // Stores calculated stats (Attack, Defense, etc.) for UI display.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Instance")
    TMap<FName, float> Attributes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Instance")
    bool bIsDead;

    // Used for sorting by "Newest". Incrementing counter from Subsystem.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Instance")
    int32 CaptureIndex;

    // Which box this creature belongs to in storage.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Instance")
    int32 StorageBoxIndex;

    FCreatureInstance()
        : CreatureDefinition(nullptr)
        , CurrentLevel(1)
        , CurrentHP(100.f)
        , MaxHP(100.f)
        , CurrentXP(0.f)
        , XPToNextLevel(100.f)
        , bIsDead(false)
        , CaptureIndex(0)
        , StorageBoxIndex(0)
    {}

    // Equality operator for TArray::Contains or generic checks
    bool operator==(const FCreatureInstance& Other) const
    {
        return CreatureDefinition == Other.CreatureDefinition;
    }
};

/**
 * Wrapper struct for saving/loading the entire collection.
 * Use this in your SaveGame object.
 */
USTRUCT(BlueprintType)
struct FCreatureCollectionSaveData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Save Data")
    TArray<FCreatureInstance> Party;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Save Data")
    TMap<FName, FCreatureInstance> Storage;

    // Persist the global capture counter
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Save Data")
    int32 TotalCaptureCount = 0;
};
