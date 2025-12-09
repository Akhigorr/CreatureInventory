#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CreatureTypes.h"
#include "CreatureVesselInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UCreatureVesselInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for Actors (Pawns/Characters) that can represent a Creature from the Collection.
 */
class CREATURESYSTEM_API ICreatureVesselInterface
{
	GENERATED_BODY()

public:
    /**
     * Called immediately after spawning to inject the saved creature stats (HP, Level, etc.).
     * Implement this in Blueprint or C++ to apply stats to your Character.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Creature Vessel")
    void CallInitializeCreature(const FCreatureInstance& CreatureData);

    /**
     * Called before saving or unpossessing to get the latest state (e.g., modified HP/XP) back into the system.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Creature Vessel")
    void CallGetUpdatedCreatureData(FCreatureInstance& OutCreatureData);
};
