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
