extends SceneTree
## Regression for responsive classless constellation placement.
const LAYOUT = preload("res://scripts/systems/moonfall_talent_panel_layout.gd")

func _initialize() -> void:
 var desktop: Dictionary = LAYOUT.placement(Vector2(1920, 1080))
 if not is_equal_approx(float(desktop["scale"]), 1.0):
  _fail("Large desktops must keep full-size controls")
  return
 var small: Dictionary = LAYOUT.placement(Vector2(800, 600))
 var factor: float = float(small["scale"])
 if factor <= 0.0 or factor >= 1.0:
  _fail("Small screens must scale the panel into view")
  return
 var visual: Vector2 = small["visual_size"]
 if visual.x > 768.01 or visual.y > 568.01:
  _fail("Scaled panel must stay inside the viewport margins")
  return
 if (small["offset_from_center"] + visual * 0.5).length() > 0.01:
  _fail("Scaled panel must remain centered")
  return
 var resized: Dictionary = LAYOUT.placement(Vector2(1280, 720))
 if resized["visual_size"].y > 688.01:
  _fail("Window resize must fit panel height")
  return
 var tiny: Dictionary = LAYOUT.placement(Vector2(420, 280))
 if float(tiny["scale"]) <= 0.0 or tiny["visual_size"].x > 388.01:
  _fail("Very small windows must never clip the panel by scaling")
  return
 print("MOONFALL TALENT PANEL LAYOUT PASS")
 quit(0)

func _fail(reason: String) -> void:
 push_error(reason)
 quit(1)
