#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CreatureTypes.h"
#include "CreatureCollectionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllPartyDead);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CREATURESYSTEM_API UCreatureCollectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UCreatureCollectionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
    // --- Configuration ---

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Collection|Config")
    int32 MaxPartySize;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Collection|Config")
    int32 MaxStorageSize;

    // If true, the creature at Party Slot 0 cannot be moved to storage or swapped out (unless by specific game logic override not implemented here).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Collection|Config")
    bool bLockFirstPartySlot;

    // --- State ---

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Creature Collection|State")
    TArray<FCreatureInstance> Party;

    // Key is the SpeciesName from the CreatureDefinition
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
     * Returns true if successfully added to either.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    bool AddCreature(FCreatureInstance NewCreature);

    /**
     * Swaps a creature from Storage to the Party.
     * @param SpeciesName The ID of the creature in Storage.
     * @param PartySlotIndex The index in the Party array to swap into (0 to MaxPartySize-1).
     * @return True if swap was successful.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    bool SwapCreatureFromStorage(FName SpeciesName, int32 PartySlotIndex);

    /**
     * Sends a creature from the Party to Storage.
     * @param PartySlotIndex The index of the party member to send.
     * @return True if successful.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    bool SendToStorage(int32 PartySlotIndex);

    /**
     * Heals all creatures in the Party (Reset HP to Max, clear Death).
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    void HealAllParty();

    /**
     * Updates a specific creature's state (e.g., after taking damage).
     * Automatically checks if all party members are dead.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    void UpdatePartyMemberState(int32 PartySlotIndex, float NewCurrentHP, bool bIsDead);

    /**
     * Checks if all party members are dead.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    bool CheckAllPartyDead() const;

    /**
     * Helper to get the correct actor class for a creature instance based on its level.
     */
    UFUNCTION(BlueprintPure, Category = "Creature Collection")
    TSubclassOf<AActor> GetCreatureEvolutionClass(const FCreatureInstance& Creature) const;

private:
    // Helper to check if we already own this species
    bool HasSpecies(FName SpeciesName) const;
};
