extends "res://scripts/native_v18.gd"
## Release hardening: restore a valid interrupted checkpoint as a last resort,
## and cap Full graphics FPS to avoid unrestricted CPU/GPU load.
## No art files, world physics, or save schema changes.

var recovered_pending_save := false

func _read_save() -> Dictionary:
 var data:Dictionary = super._read_save()
 if not data.is_empty():return data
 var pending:Dictionary = _decode_save(SAVE_STAGING)
 if not pending.is_empty():
  recovered_pending_save = true
  save_health = "PENDING RECOVERED"
  return pending
 return {}

func _apply_graphics_quality(mode:String) -> void:
 super._apply_graphics_quality(mode)
 # Unlimited FPS wastes power, increases fan noise, and skews runtime checks.
 Engine.max_fps = 60 if graphics_quality == "Smooth" else 120
