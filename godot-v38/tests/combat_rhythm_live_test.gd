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
 assert(source.contains("func _rhythm_confirmed_enemy_dodge()->void:"))
 assert(source.contains("RHYTHM.perfect_dodge(elapsed,true)"))
 assert(not source.contains("func _apply_incoming_damage(raw:float)->void:"))
 var boss_source:String=FileAccess.get_file_as_string("res://scripts/native_v38_8.gd")
 assert(boss_source.contains("_rhythm_confirmed_enemy_dodge()"))
 assert(source.contains("RHYTHM_COUNTER_MULTIPLIER"))
 assert(source.contains("rhythm_counter_until=map_time+RHYTHM_COUNTER_SECONDS"))
 assert(source.contains('_cue(player.global_position+Vector3(0.0,-0.35,0.0),"attack"'))
 assert(source.contains('_cue(player.global_position+Vector3(0.0,-0.35,0.0),"spell"'))
 assert(source.contains("BOSS POSTURE BROKEN!"))
 assert(source.contains("is_instance_valid(staggered_body)"))
 # Lifecycle must clear transient combo and dodge reward states.
 for entry in ["func _start_game(","func _restore_data(","func _respawn_player(","func _death_return_to_title("]:
  assert(source.contains(entry))
 assert(source.contains("func _rhythm_reset_transient_combat()->void:"))
 assert(source.contains("rhythm_counter_until=-999.0"))
 assert(source.contains("rhythm_dodge_rewarded=false"))
 assert(source.contains("rhythm_chain=0"))
 instance.free()
 print("Combat rhythm live scene wiring: assertions passed")
 quit()
