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
