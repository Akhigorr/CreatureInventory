#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "CreatureTypes.h"
#include "CreatureCollectionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllPartyDead);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPartyUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStorageUpdated, int32, BoxIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreatureAdded, const FCreatureInstance&, NewCreature);

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Collection|Config")
    int32 MaxPartySize = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Collection|Config")
    int32 MaxBoxes = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Collection|Config")
    int32 BoxCapacity = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Collection|Config")
    bool bLockFirstPartySlot = true;

    // --- State ---

    // Global counter for capture sorting. Saved via SaveData.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Creature Collection|State")
    int32 TotalCaptureCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Creature Collection|State")
    TArray<FCreatureInstance> Party;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Creature Collection|State")
    TMap<FName, FCreatureInstance> Storage;

    // --- Delegates ---

    // Fired when the party takes damage or dies
    UPROPERTY(BlueprintAssignable, Category = "Creature Collection|Events")
    FOnAllPartyDead OnAllPartyDead;

    // Fired whenever the Party array changes (Add, Swap, Heal, Update)
    UPROPERTY(BlueprintAssignable, Category = "Creature Collection|Events")
    FOnPartyUpdated OnPartyUpdated;

    // Fired whenever a specific storage box changes (Add, Swap)
    UPROPERTY(BlueprintAssignable, Category = "Creature Collection|Events")
    FOnStorageUpdated OnStorageUpdated;

    // Fired when a new creature is successfully caught (Party or Storage)
    UPROPERTY(BlueprintAssignable, Category = "Creature Collection|Events")
    FOnCreatureAdded OnCreatureAdded;

    // --- API ---

    /**
     * Tries to add a new creature to the collection.
     * Checks uniqueness (Species) across both Party and Storage.
     * Tries Party first, then Storage.
     * Returns true if successfully added.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    bool CallAddCreature(FCreatureInstance NewCreature);

    /**
     * Swaps a creature from Storage to the Party.
     * @param SpeciesName The ID of the creature in Storage.
     * @param PartySlotIndex The index in the Party array to swap into.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    bool CallSwapCreatureFromStorage(FName SpeciesName, int32 PartySlotIndex);

    /**
     * Sends a creature from the Party to Storage.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    bool CallSendToStorage(int32 PartySlotIndex);

    /**
     * Heals all creatures in the Party.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    void CallHealAllParty();

    /**
     * Updates a specific creature's state (e.g., after taking damage).
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    void CallUpdatePartyMemberState(int32 PartySlotIndex, float NewCurrentHP, bool bIsDead);

    /**
     * Updates a specific creature's XP and Level progress.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    void CallUpdateCreatureXP(int32 PartySlotIndex, int32 NewLevel, float NewCurrentXP, float NewXPToNext);

    /**
     * Updates the full attribute map for a creature (e.g., after leveling up).
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    void CallUpdateCreatureAttributes(int32 PartySlotIndex, const TMap<FName, float>& NewAttributes);

    /**
     * Checks if all party members are dead.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    bool CallCheckAllPartyDead() const;

    /**
     * Helper to get the correct actor class based on level.
     */
    UFUNCTION(BlueprintPure, Category = "Creature Collection")
    TSubclassOf<AActor> CallGetCreatureEvolutionClass(const FCreatureInstance& Creature) const;

    /**
     * Checks if a species is already present in the collection (Party or Storage).
     * Useful for Spawners to decide whether to spawn a pickup.
     */
    UFUNCTION(BlueprintPure, Category = "Creature Collection")
    bool CallIsSpeciesCaught(FName SpeciesName) const;

    /**
     * Returns all creatures currently stored in a specific box.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    TArray<FCreatureInstance> CallGetCreaturesInBox(int32 BoxIndex) const;

    /**
     * Returns a sorted list of ALL creatures in Storage (useful for 'All' view).
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection")
    TArray<FCreatureInstance> CallGetAllCreaturesSorted(ECreatureSortMethod Method) const;

    /**
     * Returns a struct containing the current Party and Storage data.
     * Pass this struct to your SaveGame object to save progress.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection|Persistence")
    FCreatureCollectionSaveData CallGetCollectionSaveData() const;

    /**
     * Overwrites the current Party and Storage with data from the provided struct.
     * Use this when loading from a SaveGame object.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection|Persistence")
    void CallLoadCollectionSaveData(const FCreatureCollectionSaveData& SaveData);

    /**
     * Spawns a party member into the world and initializes it with its data.
     * @param PartySlotIndex Index in the Party array.
     * @param SpawnTransform Location/Rotation to spawn at.
     * @param OutActor Returns the spawned actor (can be null if failed).
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection|Spawning")
    void CallSpawnCreatureFromParty(int32 PartySlotIndex, FTransform SpawnTransform, AActor*& OutActor);

    /**
     * Helper to possess a creature pawn.
     */
    UFUNCTION(BlueprintCallable, Category = "Creature Collection|Spawning")
    void CallPossessCreature(APlayerController* PlayerController, AActor* CreatureActor);
};
