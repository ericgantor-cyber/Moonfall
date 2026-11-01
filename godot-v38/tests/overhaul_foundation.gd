extends SceneTree
## Whole-game overhaul quality gate: core scene, controls, combat and loot.
var failed:int=0
func check(ok:bool,label:String)->void:
 if ok:print("PASS overhaul: "+label)
 else:
  failed+=1
  push_error("FAIL overhaul: "+label)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn") as PackedScene
 check(scene!=null,"active RPG scene loads")
 if scene==null:quit(1);return
 var game:Node=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(game.has_method("_cycle_aim_target"),"Tab target cycling is wired")
 check(game.has_method("_toggle_atlas"),"M map action is available")
 check(game.has_method("_spell") and game.has_method("_attack"),"combat attack and spell pathways available")
 check(game.has_method("_create_gear_drop"),"loot drop integration available")
 check(game.has_method("_defeat_beast"),"enemy defeat integration available")
 check(game.has_method("_checkpoint_extra_data"),"save extension available")
 var target_code:String=FileAccess.get_file_as_string("res://scripts/native_v38_9.gd")
 var map_code:String=FileAccess.get_file_as_string("res://scripts/native_v38_2.gd")
 check("event.keycode==KEY_TAB" in target_code and "event.keycode==KEY_F6" not in target_code,"Tab replaces F6")
 check("event.keycode==KEY_M" in map_code,"M remains the map key")
 var follower:String=FileAccess.get_file_as_string("res://scripts/native_v38_5.gd")
 check("realm_combat_attack_timer" in follower and "var companion_attack_timer" not in follower,"inherited companion timer collision fixed")
 var sprite=game.get("hero_sprite") as Sprite3D
 check(is_instance_valid(sprite) and sprite.texture!=null,"original hero sprite texture present")
 game.queue_free()
 quit(1 if failed>0 else 0)
