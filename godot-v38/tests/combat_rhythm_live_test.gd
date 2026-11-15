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
 var source:String=FileAccess.get_file_as_string("res://scripts/native_v38_9.gd")
 assert(source.contains("func _dodge()->void:"))
 assert(source.contains("func _apply_incoming_damage(raw:float)->void:"))
 assert(source.contains("RHYTHM.perfect_dodge(elapsed,raw>0.0)"))
 assert(source.contains("RHYTHM_COUNTER_MULTIPLIER"))
 assert(source.contains("super._apply_incoming_damage(raw)"))
 assert(source.contains('_cue(player.global_position+Vector3(0.0,-0.35,0.0),"attack"'))
 assert(source.contains('_cue(player.global_position+Vector3(0.0,-0.35,0.0),"spell"'))
 assert(source.contains("BOSS POSTURE BROKEN!"))
 assert(source.contains("is_instance_valid(staggered_body)"))
 instance.free()
 print("Combat rhythm live scene wiring: assertions passed")
 quit()
