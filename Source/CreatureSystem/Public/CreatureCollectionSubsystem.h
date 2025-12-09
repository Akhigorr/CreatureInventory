#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "CreatureTypes.h"
#include "CreatureCollectionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllPartyDead);

/**
 * Subsystem to manage the player's creature collection (Party + Storage).
 * Automatically created for the Local Player.
 */
UCLASS()
class CREATURESYSTEM_API UCreatureCollectionSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // --- Configuration ---
    // Note: Since Subsystems aren't Actors, we can't easily "EditAnywhere" on an instance in the level.
    // However, we can expose these as BlueprintReadWrite variables that a GameMode/Controller configures on start,
    // or load them from a Global Settings object. For this plugin, we'll keep them as variables with defaults.

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Collection|Config")
    int32 MaxPartySize = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Collection|Config")
    int32 MaxStorageSize = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Collection|Config")
    bool bLockFirstPartySlot = true;

    // --- State ---

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Creature Collection|State")
    TArray<FCreatureInstance> Party;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Creature Collection|State")
    TMap<FName, FCreatureInstance> Storage;

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Creature Collection|Events")
    FOnAllPartyDead OnAllPartyDead;

    // --- API ---

    /**
     * Tries to add a new creature to the collection.
     * Checks uniqueness (Species) across both Party and Storage.
     * Tries Party first, then Storage.
     * Returns true if successfully added.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    bool AddCreature(FCreatureInstance NewCreature);

    /**
     * Swaps a creature from Storage to the Party.
     * @param SpeciesName The ID of the creature in Storage.
     * @param PartySlotIndex The index in the Party array to swap into.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    bool SwapCreatureFromStorage(FName SpeciesName, int32 PartySlotIndex);

    /**
     * Sends a creature from the Party to Storage.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    bool SendToStorage(int32 PartySlotIndex);

    /**
     * Heals all creatures in the Party.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    void HealAllParty();

    /**
     * Updates a specific creature's state (e.g., after taking damage).
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    void UpdatePartyMemberState(int32 PartySlotIndex, float NewCurrentHP, bool bIsDead);

    /**
     * Checks if all party members are dead.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    bool CheckAllPartyDead() const;

    /**
     * Helper to get the correct actor class based on level.
     */
    UFUNCTION(BlueprintPure, Category = "Creature Collection")
    TSubclassOf<AActor> GetCreatureEvolutionClass(const FCreatureInstance& Creature) const;

    /**
     * Checks if a species is already present in the collection (Party or Storage).
     * Useful for Spawners to decide whether to spawn a pickup.
     */
    UFUNCTION(BlueprintPure, Category = "Creature Collection")
    bool IsSpeciesCaught(FName SpeciesName) const;

    /**
     * Exports the current Party and Storage to a JSON string.
     * Useful for Save Games.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection|Persistence")
    FString ExportSaveData();

    /**
     * Imports Party and Storage from a JSON string.
     * WARNING: Overwrites current collection.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection|Persistence")
    bool ImportSaveData(const FString& JsonData);
};
