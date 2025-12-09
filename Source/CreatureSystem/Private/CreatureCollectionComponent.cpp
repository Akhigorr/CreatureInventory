#include "CreatureCollectionComponent.h"

// Sets default values
UCreatureCollectionComponent::UCreatureCollectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

    MaxPartySize = 6;
    MaxStorageSize = 100;
    bLockFirstPartySlot = true;
}


// Called when the game starts
void UCreatureCollectionComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UCreatureCollectionComponent::AddCreature(FCreatureInstance NewCreature)
{
    if (!NewCreature.CreatureDefinition)
    {
        UE_LOG(LogTemp, Warning, TEXT("AddCreature: Invalid Creature Definition."));
        return false;
    }

    FName SpeciesName = NewCreature.CreatureDefinition->SpeciesName;

    // 1. Check Uniqueness
    if (HasSpecies(SpeciesName))
    {
        UE_LOG(LogTemp, Warning, TEXT("AddCreature: Species %s already exists in collection."), *SpeciesName.ToString());
        return false;
    }

    // 2. Try adding to Party
    if (Party.Num() < MaxPartySize)
    {
        Party.Add(NewCreature);
        UE_LOG(LogTemp, Log, TEXT("AddCreature: Added %s to Party."), *SpeciesName.ToString());
        return true;
    }

    // 3. Try adding to Storage
    if (Storage.Num() < MaxStorageSize)
    {
        // Ensure storage creatures are "alive" as per requirements
        NewCreature.bIsDead = false;
        NewCreature.CurrentHP = NewCreature.MaxHP; // Full heal on storage entry? User said "auto set to alive"

        Storage.Add(SpeciesName, NewCreature);
        UE_LOG(LogTemp, Log, TEXT("AddCreature: Party full. Added %s to Storage."), *SpeciesName.ToString());
        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT("AddCreature: Collection Full. Cannot add %s."), *SpeciesName.ToString());
    return false;
}

bool UCreatureCollectionComponent::SwapCreatureFromStorage(FName SpeciesName, int32 PartySlotIndex)
{
    // Validate Index
    if (PartySlotIndex < 0 || PartySlotIndex >= MaxPartySize)
    {
        UE_LOG(LogTemp, Warning, TEXT("SwapCreatureFromStorage: Invalid Party Index %d."), PartySlotIndex);
        return false;
    }

    // Check Lock
    if (PartySlotIndex == 0 && bLockFirstPartySlot && Party.IsValidIndex(0))
    {
        UE_LOG(LogTemp, Warning, TEXT("SwapCreatureFromStorage: Cannot swap into locked Slot 0."));
        return false;
    }

    // Find in Storage
    if (!Storage.Contains(SpeciesName))
    {
        UE_LOG(LogTemp, Warning, TEXT("SwapCreatureFromStorage: Creature %s not found in Storage."), *SpeciesName.ToString());
        return false;
    }

    FCreatureInstance StorageCreature = Storage[SpeciesName];

    // Case 1: Slot is Empty -> Just Move
    if (PartySlotIndex >= Party.Num())
    {
        // NOTE: TArray doesn't support sparse indices easily.
        // Logic: If user requests index 5 but array size is 2, we can't just put it at 5.
        // We will assume "PartySlotIndex" maps to the current array size if we are just "Adding".
        // BUT, the requirement says "swap".
        // If the array is full or we are targeting an existing index, we swap.
        // If the array is smaller than MaxPartySize, typically we just Add().
        // However, the user asked for "swap from storage to party" with an index.
        // If the index is out of bounds of the CURRENT Party array (but within Max), we should just Add().

        Party.Add(StorageCreature);
        Storage.Remove(SpeciesName);
        return true;
    }

    // Case 2: Slot is Occupied -> Swap
    FCreatureInstance PartyCreature = Party[PartySlotIndex];

    // Move Party Creature to Storage
    // Ensure it is healed/alive when moving to storage? User said "one in storage are auto set to alive".
    PartyCreature.bIsDead = false;
    PartyCreature.CurrentHP = PartyCreature.MaxHP;

    // We use its SpeciesName as key
    if (PartyCreature.CreatureDefinition)
    {
        Storage.Add(PartyCreature.CreatureDefinition->SpeciesName, PartyCreature);
    }

    // Remove from Storage first (clean up old key)
    Storage.Remove(SpeciesName);

    // Place new creature in party
    Party[PartySlotIndex] = StorageCreature;

    return true;
}

bool UCreatureCollectionComponent::SendToStorage(int32 PartySlotIndex)
{
    if (!Party.IsValidIndex(PartySlotIndex))
    {
         UE_LOG(LogTemp, Warning, TEXT("SendToStorage: Invalid Party Index."));
         return false;
    }

    if (PartySlotIndex == 0 && bLockFirstPartySlot)
    {
        UE_LOG(LogTemp, Warning, TEXT("SendToStorage: Cannot send locked Slot 0 to Storage."));
        return false;
    }

    if (Storage.Num() >= MaxStorageSize)
    {
        UE_LOG(LogTemp, Warning, TEXT("SendToStorage: Storage is full."));
        return false;
    }

    FCreatureInstance MovingCreature = Party[PartySlotIndex];

    // Heal/Resurrect for storage
    MovingCreature.bIsDead = false;
    MovingCreature.CurrentHP = MovingCreature.MaxHP;

    if (MovingCreature.CreatureDefinition)
    {
        Storage.Add(MovingCreature.CreatureDefinition->SpeciesName, MovingCreature);
        Party.RemoveAt(PartySlotIndex);
        return true;
    }

    return false;
}

void UCreatureCollectionComponent::HealAllParty()
{
    for (FCreatureInstance& Creature : Party)
    {
        Creature.CurrentHP = Creature.MaxHP;
        Creature.bIsDead = false;
    }
}

void UCreatureCollectionComponent::UpdatePartyMemberState(int32 PartySlotIndex, float NewCurrentHP, bool bIsDead)
{
    if (Party.IsValidIndex(PartySlotIndex))
    {
        Party[PartySlotIndex].CurrentHP = NewCurrentHP;
        Party[PartySlotIndex].bIsDead = bIsDead;

        if (CheckAllPartyDead())
        {
            OnAllPartyDead.Broadcast();
        }
    }
}

bool UCreatureCollectionComponent::CheckAllPartyDead() const
{
    if (Party.Num() == 0) return true; // Empty party considered dead? Or safe? Usually "Loss" condition implies you have creatures but they are dead. But let's assume true.

    for (const FCreatureInstance& Creature : Party)
    {
        if (!Creature.bIsDead)
        {
            return false; // Found one survivor
        }
    }
    return true;
}

TSubclassOf<AActor> UCreatureCollectionComponent::GetCreatureEvolutionClass(const FCreatureInstance& Creature) const
{
    if (Creature.CreatureDefinition)
    {
        return Creature.CreatureDefinition->GetActorClassForLevel(Creature.CurrentLevel);
    }
    return nullptr;
}

bool UCreatureCollectionComponent::HasSpecies(FName SpeciesName) const
{
    // Check Storage
    if (Storage.Contains(SpeciesName)) return true;

    // Check Party
    for (const FCreatureInstance& Creature : Party)
    {
        if (Creature.CreatureDefinition && Creature.CreatureDefinition->SpeciesName == SpeciesName)
        {
            return true;
        }
    }

    return false;
}
