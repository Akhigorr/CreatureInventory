#include "CreatureCollectionSubsystem.h"
#include "Algo/Sort.h"

void UCreatureCollectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Default values are set in header
}

bool UCreatureCollectionSubsystem::AddCreature(FCreatureInstance NewCreature)
{
    if (!NewCreature.CreatureDefinition)
    {
        UE_LOG(LogTemp, Warning, TEXT("AddCreature: Invalid Creature Definition."));
        return false;
    }

    FName SpeciesName = NewCreature.CreatureDefinition->SpeciesName;

    if (IsSpeciesCaught(SpeciesName))
    {
        UE_LOG(LogTemp, Warning, TEXT("AddCreature: Species %s already exists in collection."), *SpeciesName.ToString());
        return false;
    }

    // Set Capture Index
    TotalCaptureCount++;
    NewCreature.CaptureIndex = TotalCaptureCount;

    // Try Party
    if (Party.Num() < MaxPartySize)
    {
        Party.Add(NewCreature);
        UE_LOG(LogTemp, Log, TEXT("AddCreature: Added %s to Party."), *SpeciesName.ToString());
        return true;
    }

    // Try Storage (Find Box)
    // Naive search for first box with space
    int32 TargetBox = -1;
    for (int32 Box = 0; Box < MaxBoxes; ++Box)
    {
        int32 Count = 0;
        // Count items in this box
        for (const auto& Elem : Storage)
        {
            if (Elem.Value.StorageBoxIndex == Box)
            {
                Count++;
            }
        }

        if (Count < BoxCapacity)
        {
            TargetBox = Box;
            break;
        }
    }

    if (TargetBox != -1)
    {
        NewCreature.bIsDead = false;
        NewCreature.CurrentHP = NewCreature.MaxHP;
        NewCreature.StorageBoxIndex = TargetBox;

        Storage.Add(SpeciesName, NewCreature);
        UE_LOG(LogTemp, Log, TEXT("AddCreature: Party full. Added %s to Storage Box %d."), *SpeciesName.ToString(), TargetBox);
        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT("AddCreature: Storage Full (All Boxes). Cannot add %s."), *SpeciesName.ToString());
    return false;
}

bool UCreatureCollectionSubsystem::SwapCreatureFromStorage(FName SpeciesName, int32 PartySlotIndex)
{
    if (PartySlotIndex < 0 || PartySlotIndex >= MaxPartySize) return false;

    if (PartySlotIndex == 0 && bLockFirstPartySlot && Party.IsValidIndex(0))
    {
        UE_LOG(LogTemp, Warning, TEXT("SwapCreatureFromStorage: Cannot swap into locked Slot 0."));
        return false;
    }

    if (!Storage.Contains(SpeciesName)) return false;

    FCreatureInstance StorageCreature = Storage[SpeciesName];
    // Keep the box index for later if we swap back? Or clear it?
    // Party members effectively have undefined BoxIndex.

    // Case 1: Slot Empty -> Move
    if (PartySlotIndex >= Party.Num())
    {
        Party.Add(StorageCreature);
        Storage.Remove(SpeciesName);
        return true;
    }

    // Case 2: Swap
    FCreatureInstance PartyCreature = Party[PartySlotIndex];

    PartyCreature.bIsDead = false;
    PartyCreature.CurrentHP = PartyCreature.MaxHP;
    // Put Party creature into the OLD creature's box slot (swap places)
    PartyCreature.StorageBoxIndex = StorageCreature.StorageBoxIndex;

    if (PartyCreature.CreatureDefinition)
    {
        Storage.Add(PartyCreature.CreatureDefinition->SpeciesName, PartyCreature);
    }

    Storage.Remove(SpeciesName);
    Party[PartySlotIndex] = StorageCreature;

    return true;
}

bool UCreatureCollectionSubsystem::SendToStorage(int32 PartySlotIndex)
{
    if (!Party.IsValidIndex(PartySlotIndex)) return false;

    if (PartySlotIndex == 0 && bLockFirstPartySlot) return false;

    // Find a box for this creature
    int32 TargetBox = -1;
    for (int32 Box = 0; Box < MaxBoxes; ++Box)
    {
        int32 Count = 0;
        for (const auto& Elem : Storage)
        {
            if (Elem.Value.StorageBoxIndex == Box) Count++;
        }
        if (Count < BoxCapacity)
        {
            TargetBox = Box;
            break;
        }
    }

    if (TargetBox == -1) return false; // Full

    FCreatureInstance MovingCreature = Party[PartySlotIndex];
    MovingCreature.bIsDead = false;
    MovingCreature.CurrentHP = MovingCreature.MaxHP;
    MovingCreature.StorageBoxIndex = TargetBox;

    if (MovingCreature.CreatureDefinition)
    {
        Storage.Add(MovingCreature.CreatureDefinition->SpeciesName, MovingCreature);
        Party.RemoveAt(PartySlotIndex);
        return true;
    }

    return false;
}

void UCreatureCollectionSubsystem::HealAllParty()
{
    for (FCreatureInstance& Creature : Party)
    {
        Creature.CurrentHP = Creature.MaxHP;
        Creature.bIsDead = false;
    }
}

void UCreatureCollectionSubsystem::UpdatePartyMemberState(int32 PartySlotIndex, float NewCurrentHP, bool bIsDead)
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

bool UCreatureCollectionSubsystem::CheckAllPartyDead() const
{
    if (Party.Num() == 0) return true;

    for (const FCreatureInstance& Creature : Party)
    {
        if (!Creature.bIsDead) return false;
    }
    return true;
}

TSubclassOf<AActor> UCreatureCollectionSubsystem::GetCreatureEvolutionClass(const FCreatureInstance& Creature) const
{
    if (Creature.CreatureDefinition)
    {
        return Creature.CreatureDefinition->GetActorClassForLevel(Creature.CurrentLevel);
    }
    return nullptr;
}

bool UCreatureCollectionSubsystem::IsSpeciesCaught(FName SpeciesName) const
{
    if (Storage.Contains(SpeciesName)) return true;

    for (const FCreatureInstance& Creature : Party)
    {
        if (Creature.CreatureDefinition && Creature.CreatureDefinition->SpeciesName == SpeciesName)
        {
            return true;
        }
    }
    return false;
}

TArray<FCreatureInstance> UCreatureCollectionSubsystem::GetCreaturesInBox(int32 BoxIndex) const
{
    TArray<FCreatureInstance> Result;
    for (const auto& Elem : Storage)
    {
        if (Elem.Value.StorageBoxIndex == BoxIndex)
        {
            Result.Add(Elem.Value);
        }
    }
    return Result;
}

TArray<FCreatureInstance> UCreatureCollectionSubsystem::GetAllCreaturesSorted(ECreatureSortMethod Method) const
{
    TArray<FCreatureInstance> Result;
    Storage.GenerateValueArray(Result);

    Algo::Sort(Result, [Method](const FCreatureInstance& A, const FCreatureInstance& B)
    {
        switch (Method)
        {
            case ECreatureSortMethod::Newest:
                return A.CaptureIndex > B.CaptureIndex; // Higher index = Newer
            case ECreatureSortMethod::Oldest:
                return A.CaptureIndex < B.CaptureIndex;
            case ECreatureSortMethod::LevelHigh:
                return A.CurrentLevel > B.CurrentLevel;
            case ECreatureSortMethod::LevelLow:
                return A.CurrentLevel < B.CurrentLevel;
            default:
                return false;
        }
    });

    return Result;
}

FCreatureCollectionSaveData UCreatureCollectionSubsystem::GetCollectionSaveData() const
{
    FCreatureCollectionSaveData Data;
    Data.Party = Party;
    Data.Storage = Storage;
    Data.TotalCaptureCount = TotalCaptureCount;
    return Data;
}

void UCreatureCollectionSubsystem::LoadCollectionSaveData(const FCreatureCollectionSaveData& SaveData)
{
    Party = SaveData.Party;
    Storage = SaveData.Storage;
    TotalCaptureCount = SaveData.TotalCaptureCount;
    UE_LOG(LogTemp, Log, TEXT("LoadCollectionSaveData: Loaded %d party, %d storage. Total Captures: %d"), Party.Num(), Storage.Num(), TotalCaptureCount);
}
