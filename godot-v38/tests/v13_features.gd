extends SceneTree
## Run with a temporary XDG_DATA_HOME: --headless --path PROJECT --script res://tests/v13_features.gd
var failures := 0
func check(ok:bool, name:String) -> void:
 if ok:print("PASS v13: " + name)
 else:
  push_error("FAIL v13: " + name)
  failures+=1
func _initialize() -> void:
 call_deferred("_run")
func _run() -> void:
 # Never run checkpoint-writing tests against someone's real saved game.
 var sandbox_path: String=OS.get_environment("MOONFALL_TEST_PROFILE")
 var game_data_path: String=OS.get_user_data_dir()
 print("SAVE TEST SANDBOX: ",sandbox_path.replace("\\","/"))
 print("GODOT USER DATA: ",game_data_path.replace("\\","/"))
 if sandbox_path.is_empty() or not game_data_path.to_lower().begins_with(sandbox_path.to_lower()):
  push_error("Unsafe test save location. Run through tools/run_tests.py or the isolated Windows build script.")
  quit(2)
  return
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"Main scene loads")
 if scene==null:
  quit(1)
  return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 var mini:Control=game.get("minimap")
 check(is_instance_valid(mini) and mini.visible,"Circular minimap is visible in play")
 check(mini.name=="LiveCircularMinimap" and mini.get_script()!=null,"Dedicated minimap script attached")
 game.call("_toggle_minimap")
 check(not mini.visible,"Minimap can be hidden")
 game.call("_toggle_minimap")
 check(mini.visible,"Minimap can be shown")
 check(game.call("_recommended_power",0)<game.call("_recommended_power",5),"Six world regions have ascending power guidance")
 check(game.call("_current_power")>=11,"Base power includes level")
 game.set("hero_level",1)
 var weak:float=game.call("_power_modifier",5)
 check(weak>=0.70 and weak<1.0,"High-tier world is harder for a new hero")
 game.set("hero_level",100)
 var strong:float=game.call("_power_modifier",0)
 check(strong<=1.22 and strong>1.0,"Hero power bonus has safe cap")
 game.set("hero_level",1)
 check(game.get("companion_stance")=="Guard","New game defaults to Guard")
 game.call("_cycle_companion_stance")
 check(game.get("companion_stance")=="Follow","Guard advances to Follow")
 game.call("_cycle_companion_stance")
 check(game.get("companion_stance")=="Passive","Follow advances to Passive")
 game.call("_cycle_companion_stance")
 check(game.get("companion_stance")=="Guard","Passive cycles back to Guard")
 # Simulated controller input: no physical controller needed for button dispatch test.
 var press:=InputEventJoypadButton.new()
 press.pressed=true
 press.button_index=JOY_BUTTON_DPAD_UP
 game.call("_unhandled_input",press)
 check(game.get("companion_stance")=="Follow","D-pad up changes companion stance")
 press.button_index=JOY_BUTTON_DPAD_DOWN
 game.call("_unhandled_input",press)
 check(not mini.visible,"D-pad down toggles minimap")
 press.button_index=JOY_BUTTON_START
 game.call("_unhandled_input",press)
 check(game.get("menu_open"),"Controller Start opens Atlas")
 game.call("_toggle_menu")
 game.set("companion_stance","Passive")
 game.call("_save_game")
 var saved:Dictionary=game.call("_read_save")
 check(str(saved.get("pet_stance",""))=="Passive","Companion stance persists in schema-2 checkpoint")
 game.call("_start_game","nyra")
 check(game.get("companion_stance")=="Guard","New hero resets saved companion stance")
 game.call("_load_game")
 check(game.get("hero")=="z" and game.get("companion_stance")=="Passive","Checkpoint restores hero and companion stance")
 check(is_instance_valid(game.get("minimap")) and game.get("minimap").visible==false,"Minimap visibility preference survives hero switch")
 for frame in range(180):await physics_frame
 check(game.get("hero_playing") and game.get("map_time")>1.0,"Native v13 combat simulation advances 180 frames")
 if failures==0:print("MOONFALL NATIVE v13 FEATURES PASS")
 else:push_error("MOONFALL NATIVE v13 FEATURES FAILED: " + str(failures))
 game.queue_free()
 quit(0 if failures==0 else 1)
