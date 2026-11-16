"""Moonfall's deterministic visual direction for AI-generated fantasy creature sprites.

This module is deliberately offline. Same subject ID always yields a stable,
specific art brief, so separate assets share a coherent camera/style language.
"""
STYLE_VERSION = "moonfall-2_5d-v2"

FAMILY_DIRECTIONS = {
    "Moonfang": ("violet moonlight and silver", "crescent fangs and lunar rune fur"),
    "Ashen Crown": ("charcoal and ember orange", "cinder mane and broken crown"),
    "Venomveil": ("jade green and smoky black", "translucent poison markings and thorn scales"),
    "Duskthorn": ("wine red and dusky gold", "bramble horns and dusk-rose armor"),
    "Frostbound": ("icy blue and pale ivory", "glacial antlers and frost mist"),
    "Sunscorch": ("burnished gold and hot crimson", "solar sigils and scorched armor"),
    "Stormcaller": ("electric blue and slate", "wind-swept fur and lightning-veined features"),
    "Ironhide": ("gunmetal and copper", "natural plated hide and ancient rivets"),
    "Hollowroot": ("moss green and weathered brown", "ancient bark growth and root-like limbs"),
    "Bloodmoon": ("deep ruby and dark violet", "eclipse runes and bloody-moon glow"),
    "Starfall": ("midnight indigo and starlight silver", "comet markings and celestial fragments"),
}
RANK_FEATURES = {
    "Sentinel": "lean vigilant scout, simple natural markings, compact silhouette",
    "Reaver": "aggressive athletic build, heavier claws, battle-worn hide",
    "Warden": "sturdy defensive body, layered natural armor, broad protective stance",
    "Champion": "heroic boss-scale anatomy, luminous details, unmistakable power",
    "Sovereign": "grand apex-boss scale, elaborate natural crown, signature silhouette",
}
TRAIT_EFFECTS = {
    "Moonbound": "lunar halo and cool violet warning runes",
    "Emberborn": "burning-orange ember energy and flamelike markings",
    "Venomheart": "emerald venom energy with precise serpentine patterning",
    "Eclipsed": "contrasting shadow-and-magenta eclipse sigils",
}
HABITAT_DIRECTIONS = {
    "Grasslands and tundra": "temperate prairie or arctic adaptations, natural hide or plumage",
    "Temperate and cloud forests": "forest adaptations, leaf-touched shading without a scenery background",
    "Wetlands rivers and coasts": "water and marsh adaptations, realistic aquatic anatomy",
    "Mountains and alpine tundra": "mountain-adapted proportions, dense fur or suitable feathering",
    "Deserts and scrublands": "arid-habitat adaptations, sandy muted tones and sun-tolerant skin",
    "Tropical rainforests": "humid tropical rainforest adaptations and species-appropriate natural coloration",
}
TAXON_INSTRUCTIONS = {
    "M": "Correct mammalian limbs, ears, tail, fur and hooves/paws for this exact species",
    "B": "Correct beak shape, wing placement, tail feathers, feet and feather pattern for this exact bird",
    "R": "Correct scaly skin, tail, body profile and limbs for this exact reptile",
    "A": "Correct amphibian skin, limbs and face for this exact frog, salamander or toad",
}


GAME_ROLE_BRIEFS = {
    "regions": "A majestic, readable location mural for a shrine marker in the open-world region, with layered landscape and distinct landmarks. No characters.",
    "chambers": "A compact relief mural for a dungeon chamber. Gothic stone architecture, a unique recognizable stage motif, no doors or barriers that imply actual collision.",
    "glyphs": "A sharp, legible fantasy UI glyph with a genuinely transparent backdrop, bold shape and limited palette. It must remain readable at 32 pixels.",
    "spells": "A clear magic or action icon on transparent alpha for a small in-game skill HUD. Use a distinctive one-colour key symbol with painterly energy.",
    "gear": "One isolated RPG loot or equipment icon, no characters or scene, clear in a small inventory tile. Show a single recognizable object.",
    "scenery": "One isolated environmental prop ornament for Godot 3D placement, no ground plane or opaque sky, clean 2.5D perspective and readable mass.",
}


def art_direction(item: dict) -> str:
    """Return a specific, stable brief without an image service or external files."""
    if item["category"] in GAME_ROLE_BRIEFS:
        role = item["category"]
        title = item["subject"]
        specifics = GAME_ROLE_BRIEFS[role]
        special = (
            " Preserving the existing HUD layout is mandatory: red HP, blue mana, purple XP "
            "arcing around a circular globe; use ornament only, never replace dynamic meters."
            if role == "glyphs" and item.get("slot_key") == "orb_hud"
            else ""
        )
        return f"Moonfall {role} artwork: {title}. {specifics}{special} "
    if item["category"] == "guardians":
        family, _, rank = item["subject"].rpartition(" ")
        palette, details = FAMILY_DIRECTIONS.get(family, ("muted purple and silver", "lunar sigils"))
        return (
            f"Original named boss: {item['subject']}. "
            f"Native creature anatomy: clearly a {item['species']}, never a human. "
            f"Palette: {palette}. Distinctive design: {details}. "
            f"Rank: {RANK_FEATURES.get(rank, 'readable mid-tier creature')}. "
            f"Combat signature: {TRAIT_EFFECTS.get(item['trait'], item['trait'])}. "
            "Complexity must remain legible at a small in-game sprite size."
        )
    habitat = item.get("habitat", "Americas natural habitat")
    taxon = item.get("taxon", "M")
    return (
        f"Scientifically recognizable wild {item['subject']} from the Americas. "
        f"{TAXON_INSTRUCTIONS.get(taxon, TAXON_INSTRUCTIONS['M'])}. "
        f"{HABITAT_DIRECTIONS.get(habitat, 'Authentic natural history colors and anatomy')}. "
        "Fantasy-grade painterly rendering without changing the animal's natural identifying features."
    )


def make_prompt(item: dict) -> str:
    if item["category"] in GAME_ROLE_BRIEFS:
        return (
            "Create exactly one original game-ready transparent RGBA PNG for the Moonfall 2.5D "
            "isometric action RPG. Keep the whole art element within the canvas and leave a "
            "minimum 8 percent clear alpha margin on every side. " + art_direction(item)
            + " Consistent art bible: dark fantasy, subtly eerie yet beautiful, painterly "
            "shading, luminous accents, tactile surfaces, precise silhouette and readable "
            "iconography; high contrast at 64 pixels. Transparent means real alpha, not a "
            "checkerboard or fake alpha. No words, captions, menu text, lettering, logos, "
            "watermarks or UI backgrounds. No copyrighted references. "
            "Do not modify, depict, or restyle Moonfall's existing hero artwork of Z or Nyra."
        )
    return (
        "Produce ONE original full-body PNG concept sprite with a transparent background for Moonfall, "
        "a dark-fantasy 2.5D isometric action RPG. "
        "Fixed camera: three-quarter elevated view, facing lower-left at 30 degrees. "
        "Keep the entire single subject, limbs, horns, tail, feathers and effects within "
        "an 8 percent transparent margin on all sides; centered and grounded without a floor plane. "
        "At 128 px tall it must still be identifiable. "
        + art_direction(item)
        + " Matching art bible: elegant painterly edges, tactile material shading, controlled contrast, "
        "soft moonlit rim light, intentional negative space, one clear focal point and highly readable silhouette. "
        "Transparent background means real alpha, no checkerboard, environmental scene or contact shadow. "
        "No words, captions, logos, UI, border, bases, ground props, additional subjects or weapons detached from body. "
        "Do not imitate, modify, or depict Moonfall's original heroes Z or Nyra."
    )
