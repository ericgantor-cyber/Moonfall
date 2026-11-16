"""Optional whole-game Moonfall artwork slots with stable filenames.

Only catalogues legitimate scenery/loot/UI artwork. Heroes Z and Nyra are
deliberately excluded. This file does not contact image services or import Godot.
"""
REGIONS = [
    "Hearthstone Outpost", "Witchwood Wilds", "Moonmist Marsh",
    "Highcrest Quarry", "Shattered Ruins", "Eclipse Citadel",
]
CHAMBERS = [
    "Dungeon Threshold", "Outer Sanctum", "Guardian Crossing",
    "Moonlit Archive", "Cinder Reliquary", "Raven Watch",
    "Lost Observatory", "Whispering Vault", "Twilight Garden",
    "Ember Crucible", "Fallen Library", "Astral Gallery",
    "Hollow Shrine", "Eclipse Threshold",
]
GLYPHS = {
    "rift_warning": "Arcane rift danger sigil",
    "moon_surge": "Moon surge status glyph",
    "treasure_cache": "Treasure relic cache sigil",
    "treasure_bearer": "Treasure carrier emblem",
    "boss_phase": "Guardian combat phase emblem",
    "forge": "Hearthstone forge emblem",
    "rare_loot": "Legendary loot emblem",
    "dungeon_seal": "Eclipse dungeon entrance seal",
    "orb_hud": "Red health blue mana purple experience orb HUD ornament",
}
SPELLS = {
    "nova": "Lunar nova spell icon",
    "dash": "Arcane dash spell icon",
    "fire": "Dragonfire spell icon",
    "moonbound": "Moonbound guardian telegraph emblem",
    "emberborn": "Emberborn guardian telegraph emblem",
    "venomheart": "Venomheart guardian telegraph emblem",
    "eclipsed": "Eclipsed guardian telegraph emblem",
    "dodge": "Dodge and evade icon",
    "parry": "Parry and riposte icon",
    "block": "Shield block icon",
}
GEAR = {
    "charm": "Moonstone charm equipment icon",
    "band": "Arcane silver band ring icon",
    "cowl": "Ranger hood and cowl equipment icon",
    "boots": "Enchanted travel boots equipment icon",
    "mantle": "Mythic mantle equipment icon",
    "crest": "Legendary dungeon guardian crest icon",
    "relic": "Eclipse legendary relic icon",
    "weapon": "Fantasy blade and staff weapon icon",
    "potion": "Healing draught item icon",
    "herb": "Moonlit herbal material icon",
    "ore": "Foundry ore and mechanical crafting material icon",
    "food": "Wildlife cooking and healing food icon",
}
SCENERY = {
    "shrine": "Ancient waystone shrine ornament",
    "moon_tree": "Moonlit forest tree ornament",
    "stone_pillar": "Runic stone pillar ornament",
    "forge_anvil": "Mech foundry forging anvil ornament",
    "marsh_reeds": "Mystical marsh reeds ornament",
    "ruin_gate": "Broken stone gate ornament",
    "citadel_banner": "Eclipse citadel banner ornament",
    "campfire": "Traveler campfire ornament",
    "crystal": "Enchanted crystal mineral ornament",
    "library_books": "Lost archive books ornament",
}

SLOTS = {
    "regions": {f"{i:02d}": name for i, name in enumerate(REGIONS)},
    "chambers": {f"{i:02d}": name for i, name in enumerate(CHAMBERS, 1)},
    "glyphs": GLYPHS,
    "spells": SPELLS,
    "gear": GEAR,
    "scenery": SCENERY,
}


def game_profile(category: str, key: str) -> dict:
    """Map a registry entry to the safe, approved-image naming convention."""
    if category not in SLOTS or key not in SLOTS[category]:
        raise ValueError("Unknown game art slot. Use --list-game-art to see available names.")
    name = SLOTS[category][key]
    return {
        "subject": name, "species": "", "trait": "original game art",
        "category": category, "asset_id": f"{category[:-1] if category.endswith('s') else category}_{key}",
        "slot_key": key,
    }


def parse_slot(value: str) -> dict:
    category, sep, key = value.partition(":")
    if not sep:
        raise ValueError("Use --game-art CATEGORY:KEY (example glyphs:orb_hud).")
    return game_profile(category, key)


def all_game_items() -> list[dict]:
    return [game_profile(category, key) for category, slots in SLOTS.items() for key in slots]


def count_game_items() -> int:
    return sum(len(slots) for slots in SLOTS.values())
