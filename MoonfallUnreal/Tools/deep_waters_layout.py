"""Pure layout data in Unreal centimeters, relative to the pool center/surface."""
GROTTO_NAMES = (
    "Saltveil Hollow", "Pearlshade Arch", "Drowned Lantern",
    "Moonkelp Refuge", "Glassfin Alcove", "Brineheart Rest",
)
MIN_HALF_WIDTH = 3000.0
MIN_HALF_LENGTH = 2000.0
MIN_DEPTH = 500.0


def grotto_layout():
    return tuple(
        (name, (x, y, -180.0))
        for name, (x, y) in zip(
            GROTTO_NAMES,
            ((-1400.0, -850.0), (0.0, -850.0), (1400.0, -850.0),
             (-1400.0, 850.0), (0.0, 850.0), (1400.0, 850.0)),
        )
    )


def grotto_geometry(x, y):
    # Open-top rooms preserve a direct emergency ascent and unobstructed entry.
    return (
        ("Floor", (x, y, -350.0), (7.0, 6.0, 0.4)),
        ("West pillar", (x - 300.0, y, -200.0), (0.7, 0.7, 2.6)),
        ("East pillar", (x + 300.0, y, -200.0), (0.7, 0.7, 2.6)),
        ("Back wall", (x, y + 275.0, -200.0), (6.0, 0.6, 2.6)),
    )


# Each room gets a distinct silhouette and light color. All ornaments are
# nonblocking; the structural greybox remains the only grotto collision.
GROTTO_ART = (
    ("Salt crystal", "Cone", (0.52, 0.84, 1.0), (0.65, 0.65, 1.55)),
    ("Pearl", "Sphere", (0.85, 0.66, 1.0), (0.9, 0.9, 0.9)),
    ("Lantern", "Cylinder", (1.0, 0.62, 0.24), (0.45, 0.45, 1.25)),
    ("Kelp spire", "Cone", (0.30, 0.95, 0.58), (0.38, 0.38, 1.65)),
    ("Glassfin", "Cone", (0.32, 0.92, 0.92), (0.45, 0.70, 1.35)),
    ("Brine relic", "Sphere", (0.92, 0.34, 0.55), (0.72, 0.72, 1.05)),
)


def grotto_art(index, x, y):
    label, mesh, color, scale = GROTTO_ART[index]
    ornaments = tuple(
        (label + " " + str(i + 1), mesh, (x + dx, y + dy, -270.0),
         tuple(s * factor for s in scale))
        for i, (dx, dy, factor) in enumerate(
            ((-215.0, 150.0, 1.0), (215.0, 150.0, 0.8),
             (-220.0, -155.0, 0.55), (220.0, -155.0, 0.65))
        )
    )
    return ornaments, (x, y + 195.0, -105.0), color


def shoreline_geometry(half_width):
    # Dry deck ends at the water boundary; descending steps overlap horizontally
    # by 10 cm and differ in height by only 25 cm.
    return (
        ("Dry dock", (-half_width - 300.0, 0.0, -20.0), (6.0, 12.0, 0.4)),
        ("Shallow step", (-half_width + 95.0, 0.0, -45.0), (2.0, 12.0, 0.4)),
        ("Middle step", (-half_width + 285.0, 0.0, -70.0), (2.0, 12.0, 0.4)),
        ("Deep step", (-half_width + 475.0, 0.0, -95.0), (2.0, 12.0, 0.4)),
    )


def ruin_geometry():
    # Two broken gateway landmarks away from the center current lanes.
    return tuple(
        (f"{name} {side}", (x, y, -320.0), (0.8, 0.8, 1.8))
        for name, x in (("West drowned gate", -2200.0), ("East drowned gate", 2200.0))
        for side, y in (("north pillar", 430.0), ("south pillar", -430.0))
    )


# Open-roof wrecks, away from the grotto rooms, shore and current lanes.
WRECK_CENTERS = ((-2200.0, -1250.0, -260.0), (2200.0, 1250.0, -260.0))


def wreck_geometry():
    for index, (x, y, z) in enumerate(WRECK_CENTERS):
        name = ("Blackwake Wreck", "Moonspine Wreck")[index]
        yield name + " keel", (x, y, -410.0), (6.8, 3.6, 0.4), 0.0
        for side in (-1, 1):
            yield name + " broken hull", (x-65.0, y+side*155.0, -320.0), (4.5, 0.45, 1.4), side*6.0
            yield name + " shattered bow", (x+245.0, y+side*95.0, -320.0), (1.7, 0.4, 1.25), -side*35.0
        yield name + " fallen mast", (x-150.0, y, -365.0), (2.6, 0.3, 0.3), 22.0


# Expanded basin remains opt-in; the original 60 x 40 m map stays compatible.
EXPANDED_HALF_WIDTH = 6000.0
EXPANDED_HALF_LENGTH = 4500.0
CAVE_CENTERS = ((-3800.0, 2200.0, -255.0), (3800.0, -2200.0, -255.0))
ISLAND_CENTER = (0.0, 3300.0, 80.0)


def cave_geometry():
    for i, (x, y, _) in enumerate(CAVE_CENTERS):
        name = ("Nightglass Cavern", "Silver Current Cavern")[i]
        # Two wide exits along X; the roof is elevated for capsule clearance.
        yield name + " floor", (x,y,-405.0), (10.0,6.4,0.5)
        for side in (-1,1):
            yield name + " side wall", (x,y+side*300.0,-225.0), (10.0,0.5,3.6)
        yield name + " central arch roof", (x,y,-65.0), (4.2,6.0,0.4)


def island_geometry():
    x,y,_ = ISLAND_CENTER
    # Overlapping 25 cm steps rise from the water toward a dry plateau.
    yield "Moonwatch Isle plateau", (x,y,55.0), (14.0,10.0,0.5)
    for i in range(6):
        yield "Moonwatch landing step " + str(i+1), (x,y-595.0-i*190.0,30.0-i*25.0), (6.0,2.0,0.5)
    yield "Moonwatch standing stone", (x-450.0,y+180.0,230.0), (1.2,1.2,3.0)


ISLAND_RESIDENTS = (
    ("Warden Veyra", "MoonwatchWarden", (-450.0, 3050.0, 130.0)),
    ("Cartographer Soren", "MoonwatchCartographer", (0.0, 3500.0, 130.0)),
    ("Keeper Maelis", "MoonwatchKeeper", (450.0, 3050.0, 130.0)),
    ("Trader Ilyen", "MoonwatchTrader", (450.0, 3550.0, 130.0)),
)


def settlement_geometry():
    # Three open shelters, no closed doors or walls across the central path.
    for name, _, (x,y,_) in ISLAND_RESIDENTS:
        yield name + " canopy", (x,y+80.0,365.0), (3.6,2.8,0.25)
        for side in (-1,1):
            yield name + " support", (x+side*160.0,y+190.0,215.0), (0.35,0.35,2.7)


def ocean_resources(half_width):
    # Twelve nonblocking nodes, three deposits per material type.
    return (
        (0, "Moonkelp", "Cone", (-900.0,-1600.0,-240.0), (0.5,0.5,1.2)),
        (1, "Pearl cluster", "Sphere", (900.0,-1600.0,-280.0), (0.65,0.65,0.65)),
        (2, "Crimson coral", "Cone", (-900.0,1600.0,-240.0), (0.7,0.7,1.1)),
        (3, "Sea salt", "Cylinder", (900.0,1600.0,-280.0), (0.7,0.7,0.55)),
        (4, "Moonkelp II", "Cone", (-2650.0,-600.0,-270.0), (0.5,0.5,1.2)),
        (5, "Pearl cluster II", "Sphere", (2650.0,600.0,-300.0), (0.65,0.65,0.65)),
        (6, "Crimson coral II", "Cone", (-2650.0,600.0,-270.0), (0.7,0.7,1.1)),
        (7, "Sea salt II", "Cylinder", (2650.0,-600.0,-300.0), (0.7,0.7,0.55)),
        (8, "Moonkelp III", "Cone", (-650.0,-430.0,-270.0), (0.5,0.5,1.2)),
        (9, "Pearl cluster III", "Sphere", (650.0,430.0,-300.0), (0.65,0.65,0.65)),
        (10, "Crimson coral III", "Cone", (0.0,-1750.0,-270.0), (0.7,0.7,1.1)),
        (11, "Sea salt III", "Cylinder", (0.0,1750.0,-300.0), (0.7,0.7,0.55)),
    )


REGIONAL_HABITATS = (
    "Glacial coast", "Forest rivers", "Tundra pools", "Barrens springs",
    "Living basin", "Mountain lakes", "Sunken ruins", "Crystal waters", "Eclipse pools",
)


def regional_water_plan(province, half_width, half_length, depth):
    import math
    if type(province) is not int or not 0 <= province < 9:
        raise ValueError("Invalid province")
    if not all(math.isfinite(v) for v in (half_width, half_length, depth)):
        raise ValueError("Invalid bounds")
    if half_width < 600 or half_length < 600 or depth < 500:
        raise ValueError("Regional water requires at least 12 x 12 x 5 m")
    # West bank is continuous across both migrating school lanes.
    shore = (
        ("Dry bank", (-half_width - 300, 0, -20), (6, half_length/50, .4)),
        ("Shallow step", (-half_width+95, 0, -45), (2, half_length/50, .4)),
        ("Middle step", (-half_width+285, 0, -70), (2, half_length/50, .4)),
        ("Deep step", (-half_width+475, 0, -95), (2, half_length/50, .4)),
    )
    # East half is reserved for resources, leaving western access steps clear.
    nodes = tuple(
        (12+province*4+i, label, shape, (half_width*x, y*half_length*.4, -260), scale)
        for i, (label, shape, x, y, scale) in enumerate((
            ("Moonkelp", "Cone", .15,-1,(.5,.5,1.2)),
            ("Pearl cluster", "Sphere", .65,-1,(.65,.65,.65)),
            ("Crimson coral", "Cone", .15,1,(.7,.7,1.1)),
            ("Sea salt", "Cylinder", .65,1,(.7,.7,.55)),
        ))
    )
    return shore, nodes


# Original regional silhouettes and lighting palettes. Each uses only the
# engine primitive meshes; no imported or hero assets.
REGIONAL_ART = (
    ("Ice spire", "Cone", (.42,.76,1.0), (.55,.55,1.4), 0),
    ("River root", "Cylinder", (.35,.74,.44), (.55,.55,1.05), 3),
    ("Frost boulder", "Sphere", (.72,.89,1.0), (.85,.7,.7), 1),
    ("Bone obelisk", "Cone", (.83,.61,.32), (.45,.45,1.3), 2),
    ("Bloom coral", "Sphere", (.28,.92,.54), (.7,.7,.95), 3),
    ("Storm pillar", "Cylinder", (.45,.57,.94), (.5,.5,1.35), 2),
    ("Sunken column", "Cube", (.78,.68,.44), (.65,.65,1.2), 1),
    ("Astral crystal", "Cone", (.69,.37,1.0), (.6,.6,1.55), 4),
    ("Eclipse relic", "Sphere", (.9,.25,.46), (.75,.75,.85), 4),
)

def regional_environment_plan(province, half_width, half_length, depth):
    regional_water_plan(province, half_width, half_length, depth)
    label, shape, color, scale, profile = REGIONAL_ART[province]
    ornaments=tuple(
        (f"{label} {i+1}", shape, (half_width*x, half_length*y, -340),
         tuple(v*factor for v in scale))
        for i,(x,y,factor) in enumerate((
            (.12,-.78,1), (.62,-.78,.75), (.12,.78,.85), (.62,.78,1)
        ))
    )
    # Small pools keep gathering and swimming unobstructed; only larger
    # pools can host an optional hostile creature with its existing 270cm leash.
    wildlife = (profile, (0,0,-240)) if half_width>=1400 and half_length>=1400 else None
    return ornaments, (.35*half_width,0,-120), color, wildlife
