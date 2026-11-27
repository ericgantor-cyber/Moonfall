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
