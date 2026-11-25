"""Nine Northern Expanse zones, two unique shrines each (18 total).

Historical filename retained for compatibility with existing UE5 editor tools.
Coordinates mirror MoonfallNorthContinentLayout.h province centers; every
shrine remains on collision-backed original province pad geometry.
Pure Python so Windows/Linux CI can verify before native Unreal playtesting.
"""
import math

ZONE_NAMES = (
    "Shiverwake Coast", "Wolfsong Timberlands", "Palehorn Tundra",
    "Gravewind Barrens", "Verdant Rift", "Skyshard Summits",
    "The Sunken Crown", "Starveil Vale", "Nightspire Dominion",
)
ZONE_CENTERS = (
    (0, 0), (-14000, 13000), (0, 13000),
    (0, 26000), (-14000, 26000), (-14000, 39000),
    (0, 39000), (-14000, 52000), (0, 52000),
)
SHRINES_PER_ZONE = 2
EXPECTED_NORTHERN_SHRINES = len(ZONE_NAMES) * SHRINES_PER_ZONE
PROVINCE_PAD_HALF_SIZE_CM = 1500

# Preserve two original Shiverwake shrine identities and positions so the
# earlier island-only preview can be migrated without disturbing their IDs.
ZONE_SHRINE_LABELS = (
    (("shiverwake_landing", "Shiverwake Landing Shrine"),
     ("tideglass_watch", "Tideglass Watch Shrine")),
    (("emberpine_lodge", "Emberpine Lodge Shrine"),
     ("rootsong_clearing", "Rootsong Clearing Shrine")),
    (("greyhorn_outpost", "Greyhorn Outpost Shrine"),
     ("frosthorn_cairn", "Frosthorn Cairn Shrine")),
    (("lantern_rest", "Lantern Rest Shrine"),
     ("hollow_bell", "Hollow Bell Shrine")),
    (("bloomwatch_refuge", "Bloomwatch Refuge Shrine"),
     ("heartseed_grove", "Heartseed Grove Shrine")),
    (("cloudbreak_hold", "Cloudbreak Hold Shrine"),
     ("skybreaker_beacon", "Skybreaker Beacon Shrine")),
    (("lastlight_enclave", "Lastlight Enclave Shrine"),
     ("rusted_crown", "Rusted Crown Shrine")),
    (("prismwatch", "Prismwatch Shrine"),
     ("mirrorlight", "Mirrorlight Shrine")),
    (("dawns_edge", "Dawn's Edge Shrine"),
     ("nightspire_vigil", "Nightspire Vigil Shrine")),
)
DEFAULT_OFFSETS = ((650, -600), (-650, 600))
SHRINE_SITES = tuple(
    (shrine_id, label, (float(cx + dx), float(cy + dy), 0.0))
    for zone, ((cx, cy), labels) in enumerate(zip(ZONE_CENTERS, ZONE_SHRINE_LABELS))
    for (shrine_id, label), (dx, dy) in zip(
        labels, ((675, -850), (225, -850)) if zone == 0 else DEFAULT_OFFSETS)
)
SHRINE_ZONE_BY_ID = {
    shrine_id: zone
    for zone, pair in enumerate(ZONE_SHRINE_LABELS)
    for shrine_id, _ in pair
}

# Previously generated islands contained 18 tagged shrine actors. The
# updater migrates the 16 old tagged sites to the other eight provinces,
# but refuses to modify any customized, moved or untagged authored actor.
LEGACY_SHIVERWAKE_SITES = (
    ("shiverwake_landing", "Shiverwake Landing Shrine", (675.0, -850.0, 0.0)),
    ("tideglass_watch", "Tideglass Watch Shrine", (225.0, -850.0, 0.0)),
    ("saltwind_beacon", "Saltwind Beacon Shrine", (-225.0, -850.0, 0.0)),
    ("ravenrest", "Ravenrest Shrine", (-675.0, -850.0, 0.0)),
    ("frosttide_hollow", "Frosttide Hollow Shrine", (-1125.0, -850.0, 0.0)),
    ("dawnwatch_bluff", "Dawnwatch Bluff Shrine", (1125.0, -850.0, 0.0)),
    ("moonshard_cairn", "Moonshard Cairn Shrine", (-1125.0, -125.0, 0.0)),
    ("whispering_ice", "Whispering Ice Shrine", (-675.0, -125.0, 0.0)),
    ("blackwater_vigil", "Blackwater Vigil Shrine", (-225.0, -125.0, 0.0)),
    ("greywake_spire", "Greywake Spire Shrine", (225.0, -125.0, 0.0)),
    ("silverfall_refuge", "Silverfall Refuge Shrine", (675.0, -125.0, 0.0)),
    ("northwind_rise", "Northwind Rise Shrine", (1125.0, -125.0, 0.0)),
    ("frostglass_chapel", "Frostglass Chapel Shrine", (-1125.0, 600.0, 0.0)),
    ("stormbreak_cairn", "Stormbreak Cairn Shrine", (-675.0, 600.0, 0.0)),
    ("pale_aurora", "Pale Aurora Shrine", (-225.0, 600.0, 0.0)),
    ("seaborn_sanctuary", "Seaborn Sanctuary Shrine", (225.0, 600.0, 0.0)),
    ("winterthorn", "Winterthorn Shrine", (675.0, 600.0, 0.0)),
    ("lastlight_beacon", "Lastlight Beacon Shrine", (1125.0, 600.0, 0.0)),
)


def validate_shrine_sites(sites=SHRINE_SITES):
    if len(ZONE_NAMES) != 9 or len(ZONE_CENTERS) != 9:
        raise ValueError("Expected nine Northern Expanse zones")
    if len(ZONE_SHRINE_LABELS) != 9 or any(len(pair) != 2 for pair in ZONE_SHRINE_LABELS):
        raise ValueError("Two named shrines must be defined per zone")
    if len(sites) != EXPECTED_NORTHERN_SHRINES:
        raise ValueError("Exactly 18 Northern Expanse shrines are required")
    seen_ids, seen_names = set(), set()
    per_zone = [0] * len(ZONE_NAMES)
    for shrine_id, display, xyz in sites:
        if (not shrine_id or shrine_id in seen_ids or shrine_id not in SHRINE_ZONE_BY_ID
                or not display.endswith(" Shrine") or display in seen_names
                or len(display) > 36 or len(xyz) != 3
                or not all(math.isfinite(v) for v in xyz)):
            raise ValueError("Invalid or duplicate Northern shrine")
        zone = SHRINE_ZONE_BY_ID[shrine_id]
        if dict(ZONE_SHRINE_LABELS[zone])[shrine_id] != display:
            raise ValueError("Shrine ID/name does not match zone catalog")
        cx, cy = ZONE_CENTERS[zone]
        x, y, z = xyz
        if (not (-1200 <= x-cx <= 1200 and -1000 <= y-cy <= 900)
                or z != 0.0):
            raise ValueError("Shrine is not on its zone's walkable province pad")
        if zone == 0 and (x*x + (y+1200)**2 < 300**2 or
                          (x-650)**2 + (y+1200)**2 < 300**2):
            raise ValueError("Shrine overlaps Shiverwake ferry landing")
        seen_ids.add(shrine_id)
        seen_names.add(display)
        per_zone[zone] += 1
    if per_zone != [SHRINES_PER_ZONE] * 9:
        raise ValueError("Exactly two shrines must exist in EACH zone")
    for i, (_, _, a) in enumerate(sites):
        for _, _, b in sites[i+1:]:
            if (a[0]-b[0])**2 + (a[1]-b[1])**2 < 440**2:
                raise ValueError("220-cm shrine discovery triggers must not overlap")
    return True


validate_shrine_sites()
