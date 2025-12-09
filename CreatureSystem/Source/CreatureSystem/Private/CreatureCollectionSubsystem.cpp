#include "CreatureCollectionSubsystem.h"
#include "Algo/Sort.h"
#include "CreatureVesselInterface.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

void UCreatureCollectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Default values are set in header
}

bool UCreatureCollectionSubsystem::CallAddCreature(FCreatureInstance NewCreature)
{
    if (!NewCreature.CreatureDefinition)
    {
        UE_LOG(LogTemp, Warning, TEXT("CallAddCreature: Invalid Creature Definition."));
        return false;
    }

    FName SpeciesName = NewCreature.CreatureDefinition->SpeciesName;

    if (CallIsSpeciesCaught(SpeciesName))
    {
        UE_LOG(LogTemp, Warning, TEXT("CallAddCreature: Species %s already exists in collection."), *SpeciesName.ToString());
        return false;
    }

    // Set Capture Index
    TotalCaptureCount++;
    NewCreature.CaptureIndex = TotalCaptureCount;

    // Try Party
    if (Party.Num() < MaxPartySize)
    {
        Party.Add(NewCreature);
        OnPartyUpdated.Broadcast();
        OnCreatureAdded.Broadcast(NewCreature);
        UE_LOG(LogTemp, Log, TEXT("CallAddCreature: Added %s to Party."), *SpeciesName.ToString());
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
        OnStorageUpdated.Broadcast(TargetBox);
        OnCreatureAdded.Broadcast(NewCreature);
        UE_LOG(LogTemp, Log, TEXT("CallAddCreature: Party full. Added %s to Storage Box %d."), *SpeciesName.ToString(), TargetBox);
        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT("CallAddCreature: Storage Full (All Boxes). Cannot add %s."), *SpeciesName.ToString());
    return false;
}

bool UCreatureCollectionSubsystem::CallSwapCreatureFromStorage(FName SpeciesName, int32 PartySlotIndex)
{
    if (PartySlotIndex < 0 || PartySlotIndex >= MaxPartySize) return false;

    if (PartySlotIndex == 0 && bLockFirstPartySlot && Party.IsValidIndex(0))
    {
        UE_LOG(LogTemp, Warning, TEXT("CallSwapCreatureFromStorage: Cannot swap into locked Slot 0."));
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
        OnPartyUpdated.Broadcast();
        OnStorageUpdated.Broadcast(StorageCreature.StorageBoxIndex);
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

    OnPartyUpdated.Broadcast();
    OnStorageUpdated.Broadcast(StorageCreature.StorageBoxIndex);

    return true;
}

bool UCreatureCollectionSubsystem::CallSendToStorage(int32 PartySlotIndex)
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

        OnPartyUpdated.Broadcast();
        OnStorageUpdated.Broadcast(TargetBox);
        return true;
    }

    return false;
}

void UCreatureCollectionSubsystem::CallHealAllParty()
{
    for (FCreatureInstance& Creature : Party)
    {
        Creature.CurrentHP = Creature.MaxHP;
        Creature.bIsDead = false;
    }
    OnPartyUpdated.Broadcast();
}

void UCreatureCollectionSubsystem::CallUpdatePartyMemberState(int32 PartySlotIndex, float NewCurrentHP, bool bIsDead)
{
    if (Party.IsValidIndex(PartySlotIndex))
    {
        Party[PartySlotIndex].CurrentHP = NewCurrentHP;
        Party[PartySlotIndex].bIsDead = bIsDead;

        OnPartyUpdated.Broadcast();

        if (CallCheckAllPartyDead())
        {
            OnAllPartyDead.Broadcast();
        }
    }
}

void UCreatureCollectionSubsystem::CallUpdateCreatureXP(int32 PartySlotIndex, int32 NewLevel, float NewCurrentXP, float NewXPToNext)
{
    if (Party.IsValidIndex(PartySlotIndex))
    {
        Party[PartySlotIndex].CurrentLevel = NewLevel;
        Party[PartySlotIndex].CurrentXP = NewCurrentXP;
        Party[PartySlotIndex].XPToNextLevel = NewXPToNext;
        OnPartyUpdated.Broadcast();
    }
}

void UCreatureCollectionSubsystem::CallUpdateCreatureAttributes(int32 PartySlotIndex, const TMap<FName, float>& NewAttributes)
{
    if (Party.IsValidIndex(PartySlotIndex))
    {
        Party[PartySlotIndex].Attributes = NewAttributes;
        OnPartyUpdated.Broadcast();
    }
}

bool UCreatureCollectionSubsystem::CallCheckAllPartyDead() const
{
    if (Party.Num() == 0) return true;

    for (const FCreatureInstance& Creature : Party)
    {
        if (!Creature.bIsDead) return false;
    }
    return true;
}

TSubclassOf<AActor> UCreatureCollectionSubsystem::CallGetCreatureEvolutionClass(const FCreatureInstance& Creature) const
{
    if (Creature.CreatureDefinition)
    {
        return Creature.CreatureDefinition->CallGetActorClassForLevel(Creature.CurrentLevel);
    }
    return nullptr;
}

bool UCreatureCollectionSubsystem::CallIsSpeciesCaught(FName SpeciesName) const
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

TArray<FCreatureInstance> UCreatureCollectionSubsystem::CallGetCreaturesInBox(int32 BoxIndex) const
{
    TArray<FCreatureInstance> Result;
    for (const auto& Elem : Storage)
    {
        if (Elem.Value.CreatureDefinition && Elem.Value.StorageBoxIndex == BoxIndex)
        {
            Result.Add(Elem.Value);
        }
    }
    return Result;
}

TArray<FCreatureInstance> UCreatureCollectionSubsystem::CallGetAllCreaturesSorted(ECreatureSortMethod Method) const
{
    TArray<FCreatureInstance> Result;
    // Generate manually to filter nulls
    for (const auto& Elem : Storage)
    {
        if (Elem.Value.CreatureDefinition)
        {
            Result.Add(Elem.Value);
        }
    }

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

FCreatureCollectionSaveData UCreatureCollectionSubsystem::CallGetCollectionSaveData() const
{
    FCreatureCollectionSaveData Data;
    Data.Party = Party;
    Data.Storage = Storage;
    Data.TotalCaptureCount = TotalCaptureCount;
    return Data;
}

void UCreatureCollectionSubsystem::CallLoadCollectionSaveData(const FCreatureCollectionSaveData& SaveData)
{
    // Sanitize Party
    Party.Empty();
    int32 RemovedParty = 0;
    for (const FCreatureInstance& Creature : SaveData.Party)
    {
        if (Creature.CreatureDefinition)
        {
            Party.Add(Creature);
        }
        else
        {
            RemovedParty++;
        }
    }

    // Sanitize Storage
    Storage.Empty();
    int32 RemovedStorage = 0;
    for (const auto& Elem : SaveData.Storage)
    {
        if (Elem.Value.CreatureDefinition)
        {
            Storage.Add(Elem.Key, Elem.Value);
        }
        else
        {
            RemovedStorage++;
        }
    }

    TotalCaptureCount = SaveData.TotalCaptureCount;

    // Broadcast updates after load so UI refreshes
    OnPartyUpdated.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("CallLoadCollectionSaveData: Loaded %d party, %d storage. Total Captures: %d. Pruned %d invalid entries."), Party.Num(), Storage.Num(), TotalCaptureCount, (RemovedParty + RemovedStorage));
}

void UCreatureCollectionSubsystem::CallSpawnCreatureFromParty(int32 PartySlotIndex, FTransform SpawnTransform, AActor*& OutActor)
{
    OutActor = nullptr;

    if (!Party.IsValidIndex(PartySlotIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("CallSpawnCreatureFromParty: Invalid Index %d"), PartySlotIndex);
        return;
    }

    const FCreatureInstance& Data = Party[PartySlotIndex];
    TSubclassOf<AActor> ActorClass = CallGetCreatureEvolutionClass(Data);

    if (!ActorClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("CallSpawnCreatureFromParty: No Class found for creature in slot %d"), PartySlotIndex);
        return;
    }

    UWorld* World = GetWorld();
    if (World)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AActor* Spawned = World->SpawnActor<AActor>(ActorClass, SpawnTransform, SpawnParams);

        if (Spawned)
        {
            // Initialize Interface if implemented
            if (Spawned->Implements<UCreatureVesselInterface>())
            {
                ICreatureVesselInterface::Execute_CallInitializeCreature(Spawned, Data);
            }
            OutActor = Spawned;
        }
    }
}

void UCreatureCollectionSubsystem::CallPossessCreature(APlayerController* PlayerController, AActor* CreatureActor)
{
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("CallPossessCreature: Invalid PlayerController"));
        return;
    }

    APawn* NewPawn = Cast<APawn>(CreatureActor);
    if (!NewPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("CallPossessCreature: Actor is not a Pawn."));
        return;
    }

    PlayerController->Possess(NewPawn);
}

void UCreatureCollectionSubsystem::CallSwitchActiveCreature(APlayerController* PlayerController, int32 NewPartySlotIndex, FTransform SpawnTransform, bool bDespawnOld)
{
    if (!PlayerController) return;

    // 1. Save Current State
    APawn* OldPawn = PlayerController->GetPawn();
    if (OldPawn && OldPawn->Implements<UCreatureVesselInterface>())
    {
        FCreatureInstance UpdatedData;
        ICreatureVesselInterface::Execute_CallGetUpdatedCreatureData(OldPawn, UpdatedData);

        // Find in Party and update
        // We assume we can match by CaptureIndex (unique ID)
        for (int32 i = 0; i < Party.Num(); ++i)
        {
            if (Party[i].CaptureIndex == UpdatedData.CaptureIndex)
            {
                Party[i] = UpdatedData;
                OnPartyUpdated.Broadcast();
                break;
            }
        }
    }

    // 2. Despawn Old
    if (bDespawnOld && OldPawn)
    {
        OldPawn->Destroy();
    }

    // 3. Spawn New
    AActor* NewActor = nullptr;
    CallSpawnCreatureFromParty(NewPartySlotIndex, SpawnTransform, NewActor);

    // 4. Possess New
    if (NewActor)
    {
        CallPossessCreature(PlayerController, NewActor);
    }
}
