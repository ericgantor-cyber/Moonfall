# MOONFALL: The Six Roads Chronicle

## A large, connected narrative foundation

**Six acts, seven authored chapters per act, 42 unique chapters total.**
The existing six contiguous zones and their five physical dungeons per zone provide the spatial backbone. Moonfall's ruined moon has left six surviving regional records. The player, a wanderer rather than a playable Z or Nyra, uncovers testimony through actual exploration and boss victories. None of the records alone answers who caused the catastrophe.

**Z and Nyra are gods and important potential divine encounters.** Their identities and exact original artwork must remain unchanged. These narrative notes do not turn them into selectable heroes, assert that a particular god is guilty, or claim their climactic boss content has been implemented.

### In-game content in this change

- A self-contained C++17 catalog at `Source/MoonfallUnreal/MoonfallChronicle.h` contains all 42 names and authored lore passages. These are original story materials, not copies of another game's lore.
- J opens the existing Atlas. Its **seven numbered Chronicle tabs** now let
  players revisit each chapter of the selected act, with distinct
  **LOCKED / DISCOVERED / CLEARED** states and automatic focus on the next
  discovered, unbeaten chapter. The active act, progress and readable
  multi-line passages appear in the bottom map reader.
- Chapter 1 is the publicly available regional prologue. Chapters 2–6 become visible only when their matching dungeon entrance is actually discovered; the corresponding chapter is considered cleared when that dungeon boss is truly recorded as defeated. Chapter 7, the regional epilogue, appears only when all five regional bosses are defeated.
- Dungeons may be visited out of order. The chronicle features an incomplete discovered chapter first, then the prelude, or the unlocked epilogue once the region is cleared.
- The system reads `DiscoveredDungeonMask` and `DefeatedDungeonBossMask` from existing saves. It never grants XP, salvage, equipment, reputation or dungeon discoveries. It never scans actors each frame or creates world geometry.
- Undiscovered narrative chapter titles and prose stay concealed in the Atlas even though the authored catalog exists in source control.
- **Mouse:** click one of the seven numbered chapter tabs; clicking a
  locked chapter shows a spoiler-free discovery hint, never its title or
  full text. Click **READ** to enter reading mode or **MAP** to leave it.
  The reader's hit-testing exactly matches the HUD so chapter clicks
  never pan the underlying map.
- **Keyboard/controller:** while the Atlas is open, **Tab** or **R3**
  (controller right-stick press) toggles reading focus. In reading mode,
  left/right arrows or D-pad select chapters; outside reading mode they
  retain the existing map pan. Up/down still select regions.
  J/Esc still close the Atlas, and mouse-wheel zoom is unchanged.
  Q stays Dodge outside the Atlas. R3 remains Guard during gameplay,
  with its normal Atlas guard lock preventing interference.
- **Compact screens:** a normal Atlas card shows a short preview. Reading
  mode expands it into a larger, bounded passage view with up to five
  lines, allowing full chapter text at 640x480. At 720p and larger,
  a three-line reader presents the full authored passage. There are no
  extra save fields, queue, storyline checkpoint, cutscene, animation
  asset, or new NPC conversation in this pass.

## Six acts and 42 chapters

### Act 1. The Broken Hearth | Ashen Wilds

The sanctuary's broken oath, ashbound witnesses, and the first five shattered wardens.

**1. Ash Oath** (Always visible, regional prologue)

The Shattered Sanctuary shelters travelers who remember the moon before it broke. Their maps mark six roads, but no one agrees what struck the sky. A damaged vow is all that remains.

**2. Emberfall Crypt** (Unlock: discover dungeon 1; clear: defeat its boss)

Below the embers, burial tablets name the first guardians who refused to abandon their people. The oldest tablet is cracked exactly where a witness should have signed.

**3. Cinderwatch Bastion** (Unlock: discover dungeon 2; clear: defeat its boss)

Cinderwatch once kept warning fires across the wilds. Its abandoned signal mirrors still point toward a night when every watchtower saw a different moon.

**4. Ashen Reliquary** (Unlock: discover dungeon 3; clear: defeat its boss)

The reliquary's empty cradle suggests someone removed the flame-seal before the catastrophe. Ash-stained notes ask whether protecting a secret can become a betrayal.

**5. Pyreborn Hollow** (Unlock: discover dungeon 4; clear: defeat its boss)

Survivors of Pyreborn recorded an impossible heat beneath cold rock. Their testimony speaks of two distant divine presences, never of mortal rulers.

**6. Sable Furnace** (Unlock: discover dungeon 5; clear: defeat its boss)

The furnace forged chains meant to bind something larger than a kingdom. Its final ledger shows that the chains were divided between six regions.

**7. Ember Testament** (Unlock: defeat all five regional dungeon bosses)

With the five wardens silenced, the Ashen Wilds offers its fragment of the broken oath. It is evidence, not absolution: the other roads must be heard.

### Act 2. The Covenant of Roots | Moonveil Forest

The forest's living witnesses, missing covenants, and memory that outlasts written history.

**1. The Whispering Bough** (Always visible, regional prologue)

Moonveil elders say the forest kept its own record when royal archives burned. Every root remembers a name, but the living must learn to listen without forcing an answer.

**2. Briarshade Grotto** (Unlock: discover dungeon 1; clear: defeat its boss)

Briarshade's underground pools reflect branches that are no longer there. One reflection follows the same crack that crossed the moon on the final peaceful night.

**3. Moonpetal Ruins** (Unlock: discover dungeon 2; clear: defeat its boss)

A garden of stone petals marks where envoys once exchanged safe passage. The missing sixth inscription suggests the pact was already fraying before the sky fell.

**4. Weepingroot Sanctum** (Unlock: discover dungeon 3; clear: defeat its boss)

Weepingroot preserved witnesses in woven bark rather than paper. Their accounts disagree about the culprit but agree that the warning was ignored.

**5. Veilwood Depths** (Unlock: discover dungeon 4; clear: defeat its boss)

In the old veilwood, moonlight bends around an empty clearing. A trail of faded offerings points toward an oath given to gods, not to kings.

**6. Thornbound Chapel** (Unlock: discover dungeon 5; clear: defeat its boss)

The chapel's thorn gates protected a seed chest through generations of darkness. What lies inside could heal the forest, or recall the debt that wounded it.

**7. The Living Covenant** (Unlock: defeat all five regional dungeon bosses)

When Moonveil's guardians fall, the forest releases a living memory of the covenant. Its truth can travel with the wanderer, but cannot yet settle the dispute.

### Act 3. The Choir Below | Hollow Depths

Hidden choral archives beneath six lands, erased evidence, and caution about voices in the dark.

**1. The Buried Chorus** (Always visible, regional prologue)

The Hollows were not always a prison. Miners built a city beneath the roots, then sealed its doors when strange echoes began answering questions no one had asked.

**2. Blackwater Ossuary** (Unlock: discover dungeon 1; clear: defeat its boss)

Names scratched onto blackwater stones belong to people who vanished from every surface census. Their disappearances connect the sky's fracture to the deep roads.

**3. Echoing Chasm** (Unlock: discover dungeon 2; clear: defeat its boss)

Voices cross the chasm in patterns that sound like a warning repeated too late. The echoes are real, but they do not claim to know who made the first breach.

**4. Buried Choir** (Unlock: discover dungeon 3; clear: defeat its boss)

In the Buried Choir, seven silent chambers surround an eighth without a door. The missing room implies someone deliberately removed the final note.

**5. Gloamstone Vault** (Unlock: discover dungeon 4; clear: defeat its boss)

Gloamstone tablets show the six regional seals arranged around an empty center. The map can guide the investigation without revealing what waits beneath that center.

**6. Abyssal Watch** (Unlock: discover dungeon 5; clear: defeat its boss)

The Abyssal Watch guarded a passage below the deepest known floor. Its last sentries left a single instruction: do not confuse a voice with its owner.

**7. The Eighth Note** (Unlock: defeat all five regional dungeon bosses)

With the Hollows cleared, the underground choir falls quiet long enough to hear a new pattern. Five notes answer five regions; a sixth remains incomplete.

### Act 4. The Clockwork Schism | Iron Foundry

A machine network built to help people, then driven by a forged command across the regions.

**1. The Foundry's Debt** (Always visible, regional prologue)

Iron Foundry engineers promised that their machines would keep every city supplied. After the moon broke, the same machines began repairing weapons no one had ordered.

**2. Rustglass Foundry** (Unlock: discover dungeon 1; clear: defeat its boss)

Rustglass vats hold metal that reflects memories instead of faces. A worker's etched warning identifies a command that survived its original creator.

**3. Brassfang Works** (Unlock: discover dungeon 2; clear: defeat its boss)

Brassfang's gears still turn around an absent master key. The design was copied from a seal brought from the ashlands before anyone understood its purpose.

**4. Broken Crucible** (Unlock: discover dungeon 3; clear: defeat its boss)

The crucible shattered while forging a crown-shaped circuit. Its shards bear six symbols, but the diagram leaves no place for a seventh hand.

**5. Furnaceheart Keep** (Unlock: discover dungeon 4; clear: defeat its boss)

Deep within Furnaceheart, repair drones obey the last impossible instruction they received: prepare for a visitor who has not yet arrived.

**6. Ironwake Vault** (Unlock: discover dungeon 5; clear: defeat its boss)

The Ironwake plans reveal that machines once moved messages between shrine networks. A forged message could have carried the false summons that divided the regions.

**7. The Unmade Key** (Unlock: defeat all five regional dungeon bosses)

With the Foundry cores quiet, an unfinished key can finally be understood. It may open a path to the truth, but the wanderer must decide which witness to trust.

### Act 5. The Silent Winter | Frostbound Reach

Star charts frozen in ice, a lost coordinate, and the burden of knowing too much too early.

**1. A Promise in Ice** (Always visible, regional prologue)

Frostbound elders kept the last clear star charts beneath glacial stone. Their descendants guard them from anyone who promises a quick answer to the moon's ruin.

**2. Shiverpeak Cairn** (Unlock: discover dungeon 1; clear: defeat its boss)

A mountain cairn holds the instruments of a cartographer who mapped light as though it were a road. His final route ends in an unmarked sky.

**3. Winterglass Tomb** (Unlock: discover dungeon 2; clear: defeat its boss)

Winterglass preserves a treaty signed by travelers from every region. Frost crept across the ink on the day the signatories ceased speaking.

**4. Frostmourn Hollows** (Unlock: discover dungeon 3; clear: defeat its boss)

The hollows' ice remembers a battle never recorded in the histories. Its echoes warn that victory over a guardian is not the same as saving a land.

**5. Glacierwatch Ruins** (Unlock: discover dungeon 4; clear: defeat its boss)

From Glacierwatch, the fractured moon appears whole for a heartbeat each winter. The phenomenon hints at a missing piece rather than a destroyed world.

**6. Coldstar Bastion** (Unlock: discover dungeon 5; clear: defeat its boss)

Coldstar's gates were closed to keep something inside, not to keep invaders out. The last captain recorded a plea for patience rather than a call to arms.

**7. The Last Clear Night** (Unlock: defeat all five regional dungeon bosses)

When the frost wardens are defeated, the old star chart becomes readable. Its final coordinate points beyond the six roads toward an eclipse no one can yet prevent.

### Act 6. The Unwritten Dawn | Eclipse Wastes

The collapse of competing prophecies, the last surviving records, and the unanswered divine reckoning.

**1. At the Dark Horizon** (Always visible, regional prologue)

The Eclipse Wastes are where the six regional stories begin to disagree most sharply. Here the sky refuses a single shape, and every prophecy has an author.

**2. Umbral Scar** (Unlock: discover dungeon 1; clear: defeat its boss)

The scar marks the place where ground and shadow stopped obeying the same horizon. A surviving inscription asks who benefits when the world fears the dark.

**3. Nightfall Reliquary** (Unlock: discover dungeon 2; clear: defeat its boss)

In Nightfall's vaults, histories have been rewritten and sealed again. The corrections suggest that more than one faction attempted to claim the moon's fall.

**4. Duskspire Ruins** (Unlock: discover dungeon 3; clear: defeat its boss)

Duskspire's ruined observatory follows two divine trajectories through the fractured sky. The names Z and Nyra appear as gods in old warnings, not mortal companions.

**5. Black Sun Sanctuary** (Unlock: discover dungeon 4; clear: defeat its boss)

The Black Sun sanctuary guarded the final witness to an ancient bargain. What remains is testimony, not permission to rewrite any of the gods' identities.

**6. Eclipse Throne** (Unlock: discover dungeon 5; clear: defeat its boss)

At the Eclipse Throne, the six threads of evidence converge on a chamber still waiting for its true scene and encounter. Clearing its guardian opens the last written record.

**7. The Unwritten Dawn** (Unlock: defeat all five regional dungeon bosses)

With the six roads explored and the Eclipse Wastes quiet, the chronicle ends at a choice still unwritten. The gods remain themselves; the next confrontation deserves its own authored quest.

## Gothic Isle story bridge: The Seventh Witness

Gothic Isle now has a **separate seven-chapter narrative companion** to
the six mainland acts. It does **not** change the original 6 x 7 = 42
mainland chapters, overwrite dungeon progression, modify Z/Nyra, or
change the save format. The original island residents and shrines in the
expanded world supply its existing, persistent progression evidence.

| Chapter | Title | Unlock |
| --- | --- | --- |
| 1 | The Nightferry's Wake | Discover Raven's Landing or Blackbell shrine |
| 2 | Mara's Missing Tide | Accept Mara Vesper's existing contract |
| 3 | The Blackbell Testament | Accept Brother Corvin's contract |
| 4 | Names Beneath the Ash | Accept Veyra Ash's contract |
| 5 | The Last Lantern | Accept Rook Thorn's contract |
| 6 | Roots of the Unsaid | Accept Elias Briar's contract |
| 7 | The Seventh Witness | Discover Blackbell shrine, complete all five contracts, and finish all six mainland acts |

Resident chapters change from DISCOVERED to CLEARED when their
existing contracts are completed. The final chapter is evidence for a
future divine main-story confrontation, not a claim that a final boss
scene, island dungeon, or cinematic ending already exists.

Island residents and their five factions are registered in both the live
quest/reputation validators and the save/load allowlists, so accepted
contracts, bond progress and faction rewards can persist with the
existing save format. Unknown IDs are still rejected.

The **J Atlas** sidebar shows a read-only `ISLE x/7` progress line
(and completed contract count on noncompact layouts), using the
existing saved shrine and contract states. The island's full prose
exists in `MoonfallIsleChronicle.h` and is spoiler-gated by a
native-free access API; **a separate in-game seven-tab island reader
has not yet been wired into the Atlas**. The existing six-act reader,
its mouse/controller controls, and all mainland lore stay unchanged.
Source tests cover every valid five-contract state combination, fresh
save spoiler protection, and preservation of the 42-chapter contract.
Native Unreal Windows gameplay testing is still required.

## Gameplay and narrative direction after this pass

The seven chapters of an act are not seven new quests: they are **42 lore beats tied to existing progression**, so the player discovers the story while enjoying the connected world. A future full narrative implementation can connect them to voiced NPC scenes, unique encounters, branching faction responses, in-world artifacts, journal history and authored cinematic transitions.

The overarching mystery is intentionally not resolved yet. The six epilogues reveal complementary evidence, not a prewritten punishment for Z or Nyra. A later divine storyline should use the original gods and their art, with bespoke mechanics, narrative choices and clear authorial direction.

### Native verification before merge

1. Build on a real Windows 10/11 UE5 toolchain, then run in Editor and packaged mode.
2. Visit J Atlas at 720p, 1080p, 4K, and compact 640x480. Confirm the
   seven tabs and READ/MAP toggle align with their mouse hitboxes,
   compact reading mode shows the passage without overflow, and the
   card doesn't leave permanent map markers or break map panning.
3. Confirm each region shows one prelude on a fresh save. Click
   every locked chapter; it must show only UNDISCOVERED and a generic
   hint, never its title or story paragraph. Check grey/amber/green
   labels and color-independent status text.
4. Discover dungeon 5 before dungeon 1 and confirm only dungeon 5's chapter becomes available.
5. Defeat a real tagged dungeon boss and confirm that chapter becomes cleared. Defeating an unrelated enemy must not count.
6. Defeat all five regional bosses and confirm the regional epilogue appears once and remains available after save and reload.
7. Load a corrupted/out-of-range progress mask. The chronicle should refuse unsafe disclosure and must not crash or create rewards.
8. Test mouse chapter tabs, READ/MAP toggle, Tab and controller R3 focus,
   left/right chapter navigation, Up/Down region selection, map click
   and mouse-wheel zoom, J / Escape, and gameplay Guard/Dodge after closing
   the Atlas. Hover tooltips must not draw on top of the chapter reader.
9. Verify the original artwork, animations and divine identities of Z and Nyra have not changed.

Automated C++17 and Python tests confirm catalog size, unique IDs, exact dungeon titles, mask gating, 6x7 progression, exact shared HUD/pointer geometry, the 7-tab reader, compact full-text mode, and read-only source integration. **These checks do not prove native UE5 compilation or packaged gameplay.**
