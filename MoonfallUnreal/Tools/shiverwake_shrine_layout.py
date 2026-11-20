"""Shared 18-shrine placement manifest for the isolated Shiverwake preview.

Coordinates are on the existing Shiverwake Coast province pad (center 0,0,
half-width 1500 cm), NOT new zones or extra land beyond authored collision.
No Unreal imports so ordinary Linux/Windows CI can validate the exact layout.
"""
import math

EXPECTED_SHIVERWAKE_SHRINES = 18
SHRINE_SITES = (
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
    if len(sites) != EXPECTED_SHIVERWAKE_SHRINES:
        raise ValueError("Exactly 18 Shiverwake shrines are required")
    ids = set()
    names = set()
    for shrine_id, display, xyz in sites:
        if not shrine_id or shrine_id in ids or not shrine_id.startswith((
                "shiverwake_", "tideglass_", "saltwind_", "ravenrest",
                "frosttide_", "dawnwatch_", "moonshard_", "whispering_",
                "blackwater_", "greywake_", "silverfall_", "northwind_",
                "frostglass_", "stormbreak_", "pale_", "seaborn_",
                "winterthorn", "lastlight_")):
            raise ValueError("Missing, duplicate or invalid shrine ID")
        if not display.endswith(" Shrine") or display in names or len(display) > 36:
            raise ValueError("Shrine display name must be unique and readable")
        ids.add(shrine_id)
        names.add(display)
        x, y, z = xyz
        if not all(math.isfinite(p) for p in xyz) or z != 0.0:
            raise ValueError("Shrine positions must be finite and at floor Z=0")
        if not (-1200.0 <= x <= 1200.0 and -1000.0 <= y <= 900.0):
            raise ValueError("Shrine must stay on the actual Shiverwake pad")
        if (x * x + (y + 1200.0) ** 2 < 300.0 ** 2 or
                (x - 650.0) ** 2 + (y + 1200.0) ** 2 < 300.0 ** 2):
            raise ValueError("Shrine must not auto-trigger on ferry arrival/boarding")
    for i, (_, _, a) in enumerate(sites):
        for _, _, b in sites[i + 1:]:
            if (a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 < 440.0 ** 2:
                raise ValueError("Shrines must not overlap discovery radii")
    return True


validate_shrine_sites()
