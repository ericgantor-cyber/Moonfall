extends SceneTree
## Structural smoke test for the live combat wiring; no game instance required.
func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 assert(scene!=null)
 var live:Script=load("res://scripts/native_v38_9.gd") as Script
 assert(live!=null)
 assert(live.get_base_script()!=null)
 var rules:Script=load("res://scripts/systems/combat_rhythm.gd") as Script
 assert(rules!=null)
 var instance:Node=scene.instantiate()
 assert(instance.get_script()==live)
 instance.free()
 print("Combat rhythm live scene wiring: assertions passed")
 quit()
