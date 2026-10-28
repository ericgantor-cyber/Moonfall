extends "res://scripts/native_v24_1.gd"
## v25 legacy modernization. All earlier combat/world/save behaviors inherited.
## A centralized HUD label replaces the inherited per-version string rewrites.
func _release_label() -> String:
 return "v25"
