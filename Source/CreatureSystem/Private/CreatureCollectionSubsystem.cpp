#include "CreatureCollectionSubsystem.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"

void UCreatureCollectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Default values are set in header, or can be loaded from config here
}

bool UCreatureCollectionSubsystem::AddCreature(FCreatureInstance NewCreature)
{
    if (!NewCreature.CreatureDefinition)
    {
        UE_LOG(LogTemp, Warning, TEXT("AddCreature: Invalid Creature Definition."));
        return false;
    }

    FName SpeciesName = NewCreature.CreatureDefinition->SpeciesName;

    // 1. Check Uniqueness
    if (IsSpeciesCaught(SpeciesName))
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
        // Auto-heal for storage
        NewCreature.bIsDead = false;
        NewCreature.CurrentHP = NewCreature.MaxHP;

        Storage.Add(SpeciesName, NewCreature);
        UE_LOG(LogTemp, Log, TEXT("AddCreature: Party full. Added %s to Storage."), *SpeciesName.ToString());
        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT("AddCreature: Collection Full. Cannot add %s."), *SpeciesName.ToString());
    return false;
}

bool UCreatureCollectionSubsystem::SwapCreatureFromStorage(FName SpeciesName, int32 PartySlotIndex)
{
    if (PartySlotIndex < 0 || PartySlotIndex >= MaxPartySize) return false;

    // Lock Check
    if (PartySlotIndex == 0 && bLockFirstPartySlot && Party.IsValidIndex(0))
    {
        UE_LOG(LogTemp, Warning, TEXT("SwapCreatureFromStorage: Cannot swap into locked Slot 0."));
        return false;
    }

    if (!Storage.Contains(SpeciesName)) return false;

    FCreatureInstance StorageCreature = Storage[SpeciesName];

    // Case 1: Slot Empty -> Move
    if (PartySlotIndex >= Party.Num())
    {
        Party.Add(StorageCreature);
        Storage.Remove(SpeciesName);
        return true;
    }

    // Case 2: Swap
    FCreatureInstance PartyCreature = Party[PartySlotIndex];

    // Heal Party Creature for Storage
    PartyCreature.bIsDead = false;
    PartyCreature.CurrentHP = PartyCreature.MaxHP;

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

    if (Storage.Num() >= MaxStorageSize) return false;

    FCreatureInstance MovingCreature = Party[PartySlotIndex];

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

// --- Helper for Json Serialization ---

// Manually serialize because UObject* (Definition) doesn't serialize nicely with FJsonObjectConverter by default without custom handling
TSharedPtr<FJsonObject> SerializeCreatureInstance(const FCreatureInstance& Instance)
{
    TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);

    if (Instance.CreatureDefinition)
    {
        JsonObj->SetStringField("DefinitionPath", Instance.CreatureDefinition->GetPathName());
    }
    else
    {
        JsonObj->SetStringField("DefinitionPath", "");
    }

    JsonObj->SetNumberField("CurrentLevel", Instance.CurrentLevel);
    JsonObj->SetNumberField("CurrentHP", Instance.CurrentHP);
    JsonObj->SetNumberField("MaxHP", Instance.MaxHP);
    JsonObj->SetBoolField("bIsDead", Instance.bIsDead);

    return JsonObj;
}

FCreatureInstance DeserializeCreatureInstance(const TSharedPtr<FJsonObject>& JsonObj)
{
    FCreatureInstance Instance;

    FString Path = JsonObj->GetStringField("DefinitionPath");
    if (!Path.IsEmpty())
    {
        // Load the asset
        Instance.CreatureDefinition = LoadObject<UCreatureDefinition>(nullptr, *Path);
    }

    Instance.CurrentLevel = JsonObj->GetNumberField("CurrentLevel");
    Instance.CurrentHP = JsonObj->GetNumberField("CurrentHP");
    Instance.MaxHP = JsonObj->GetNumberField("MaxHP");
    Instance.bIsDead = JsonObj->GetBoolField("bIsDead");

    return Instance;
}

FString UCreatureCollectionSubsystem::ExportSaveData()
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);

    // 1. Serialize Party
    TArray<TSharedPtr<FJsonValue>> PartyArray;
    for (const FCreatureInstance& Creature : Party)
    {
        TSharedPtr<FJsonObject> CreatureJson = SerializeCreatureInstance(Creature);
        PartyArray.Add(MakeShareable(new FJsonValueObject(CreatureJson)));
    }
    RootObject->SetArrayField("Party", PartyArray);

    // 2. Serialize Storage (Array of Objects)
    TArray<TSharedPtr<FJsonValue>> StorageArray;
    for (const auto& Elem : Storage)
    {
        TSharedPtr<FJsonObject> CreatureJson = SerializeCreatureInstance(Elem.Value);
        // We don't need to explicitly save the Key (SpeciesName) because it's derived from the Definition inside the struct.
        StorageArray.Add(MakeShareable(new FJsonValueObject(CreatureJson)));
    }
    RootObject->SetArrayField("Storage", StorageArray);

    // 3. Convert to String
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    return OutputString;
}

bool UCreatureCollectionSubsystem::ImportSaveData(const FString& JsonData)
{
    if (JsonData.IsEmpty()) return false;

    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonData);

    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("ImportSaveData: Failed to parse JSON."));
        return false;
    }

    // Clear current data
    Party.Empty();
    Storage.Empty();

    // 1. Import Party
    const TArray<TSharedPtr<FJsonValue>>* PartyArray;
    if (RootObject->TryGetArrayField("Party", PartyArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *PartyArray)
        {
            TSharedPtr<FJsonObject> CreatureObj = Val->AsObject();
            if (CreatureObj.IsValid())
            {
                FCreatureInstance Inst = DeserializeCreatureInstance(CreatureObj);
                if (Inst.CreatureDefinition)
                {
                    // Basic validation, maybe verify MaxPartySize logic here or just force load?
                    // Typically Load overrides rules, so we just add.
                    Party.Add(Inst);
                }
            }
        }
    }

    // 2. Import Storage
    const TArray<TSharedPtr<FJsonValue>>* StorageArray;
    if (RootObject->TryGetArrayField("Storage", StorageArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *StorageArray)
        {
            TSharedPtr<FJsonObject> CreatureObj = Val->AsObject();
            if (CreatureObj.IsValid())
            {
                FCreatureInstance Inst = DeserializeCreatureInstance(CreatureObj);
                if (Inst.CreatureDefinition)
                {
                    Storage.Add(Inst.CreatureDefinition->SpeciesName, Inst);
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("ImportSaveData: Successfully loaded %d party members and %d storage members."), Party.Num(), Storage.Num());
    return true;
}
