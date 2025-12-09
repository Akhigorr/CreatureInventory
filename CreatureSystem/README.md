# Creature System Plugin

A generic Creature Collection system for Unreal Engine 5.6+, capable of managing Party, Storage, Evolution, and Stats.

## Installation

1.  Copy the `CreatureSystem` folder into your project's `Plugins/` directory (create the folder if it doesn't exist).
2.  Regenerate Visual Studio project files.
3.  Compile the project.
4.  Enable the plugin in `Edit -> Plugins -> CreatureSystem`.

## Core Concepts

### 1. Creature Definition (`UCreatureDefinition`)
A **Data Asset** that defines a species.
*   **Species Name**: Unique ID for the species (e.g., "Katraji").
*   **Evolution Map**: Defines which Actor Class to spawn based on the creature's level.
    *   *Example*: Level 1 -> `BP_Katraji_Baby`, Level 20 -> `BP_Katraji_Adult`.

### 2. Creature Instance (`FCreatureInstance`)
A **Struct** holding the runtime data of a specific creature.
*   `CurrentLevel`, `CurrentHP`, `CurrentXP`, `Attributes` (Map), `CaptureIndex` (for sorting).
*   Holds a reference to the `CreatureDefinition`.

### 3. Collection Subsystem (`UCreatureCollectionSubsystem`)
A **Local Player Subsystem** that manages the player's inventory. Accessible globally in Blueprints via `Get CreatureCollectionSubsystem`.

---

## API Reference (Blueprints)

All functions are prefixed with `Call` for easy searching.

### Collection Management
*   **`CallAddCreature(NewCreature)`**: Adds a creature. Tries Party first, then finds an empty Storage Box. Returns `false` if species is unique and already caught.
*   **`CallSwapCreatureFromStorage(SpeciesName, PartySlotIndex)`**: Swaps a creature from the storage box into the active party slot. Handles swapping back if slot is occupied.
*   **`CallSendToStorage(PartySlotIndex)`**: Sends a party member to the first available storage box.
*   **`CallIsSpeciesCaught(SpeciesName)`**: Returns true if you own this species (in Party OR Storage). Use this to prevent spawning capture items.

### Spawning & Possession
*   **`CallSpawnCreatureFromParty(PartySlotIndex, Transform, OutActor)`**: Spawns the Actor defined in the Evolution Map for the creature's current level.
*   **`CallPossessCreature(PlayerController, CreatureActor)`**: Helper to make the Player Controller possess the spawned creature.
    *   *Note*: The spawned actor must implement `ICreatureVesselInterface` to receive stats.

### Stats & Logic
*   **`CallUpdateCreatureXP(Slot, Level, XP, XPToNext)`**: Updates progression data.
*   **`CallUpdateCreatureAttributes(Slot, AttributeMap)`**: Updates generic stats (Attack, Def, etc.).
*   **`CallHealAllParty()`**: Resets HP to MaxHP for all party members.
*   **`CallCheckAllPartyDead()`**: Returns true if all party members have `bIsDead == true`.

### UI Helpers
*   **`CallGetCreaturesInBox(BoxIndex)`**: Returns list of creatures in a specific storage box.
*   **`CallGetAllCreaturesSorted(Method)`**: Returns a sorted list of ALL storage creatures (Newest, Oldest, Level High/Low).

### Events (Delegates)
Assign these in your UI Widget Construct to update automatically:
*   `OnPartyUpdated`
*   `OnStorageUpdated(BoxIndex)`
*   `OnCreatureAdded(NewCreature)`
*   `OnAllPartyDead`

---

## How to Setup a Creature Actor

1.  Create a Character/Pawn Blueprint (e.g., `BP_Katraji`).
2.  Go to **Class Settings -> Interfaces** and add `Creature Vessel Interface`.
3.  Implement the event **`Call Initialize Creature`**:
    *   This event gives you the `FCreatureInstance` struct.
    *   Use this data to set your Max Health, Current Health, Attack Damage, etc. on spawn.

## Saving & Loading

The system uses a struct `FCreatureCollectionSaveData` compatible with `USaveGame`.

**To Save:**
1.  Get the Subsystem.
2.  Call `CallGetCollectionSaveData`.
3.  Save the returned Struct into your `USaveGame` object.
4.  Write to disk (e.g., `SaveGameToSlot`).

**To Load:**
1.  Load your `USaveGame` object.
2.  Get the Struct.
3.  Call `CallLoadCollectionSaveData` on the Subsystem.
4.  The system will automatically broadcast `OnPartyUpdated` to refresh the UI.
