extends RefCounted
## Fits a wide constellation panel to the visible Godot viewport without
## changing the web's internal coordinate system or any save data.
const DESIGN_SIZE: Vector2 = Vector2(1100.0, 730.0)
const VIEWPORT_MARGIN: float = 16.0

static func placement(viewport_size: Vector2) -> Dictionary:
 var usable: Vector2 = Vector2(maxf(1.0, viewport_size.x - VIEWPORT_MARGIN * 2.0), maxf(1.0, viewport_size.y - VIEWPORT_MARGIN * 2.0))
 var factor: float = minf(1.0, minf(usable.x / DESIGN_SIZE.x, usable.y / DESIGN_SIZE.y))
 var visual_size: Vector2 = DESIGN_SIZE * factor
 var position: Vector2 = (viewport_size - visual_size) * 0.5
 return {"scale": factor, "position": position, "visual_size": visual_size}
