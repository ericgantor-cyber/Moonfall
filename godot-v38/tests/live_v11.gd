extends SceneTree
## Extra end-to-end headless test: actual physics ticks, XP, save/load and hero-switch recovery.
## Writes test checkpoints to user://. Run with a temporary XDG_DATA_HOME.
var failures := 0
func check(ok: bool, name: String) -> void:
 if ok: print("PASS: " + name)
 else:
  push_error("LIVE FAIL: " + name)
  failures += 1

func _initialize() -> void:
 call_deferred("_run")

func _run() -> void:
 # Never run checkpoint-writing tests against someone's real saved game.
 var sandbox_path: String=OS.get_environment("MOONFALL_TEST_PROFILE")
 var game_data_path: String=OS.get_user_data_dir()
 var sandbox_normalized:String=sandbox_path.replace("\\","/").trim_suffix("/").to_lower()
 var user_normalized:String=game_data_path.replace("\\","/").to_lower()
 if sandbox_normalized.is_empty() or not user_normalized.begins_with(sandbox_normalized+"/"):
  push_error("Unsafe test save location. Run through tools/run_tests.py or the isolated Windows build script.")
  quit(2)
  return
 var scene: PackedScene = load("res://scenes/main.tscn")
 if scene == null:
  push_error("LIVE FAIL: Scene load")
  quit(1)
  return
 var game: Node3D = scene.instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game", "z")
 var t: float = game.get("map_time")
 for frame in range(120):
  await physics_frame
 check(game.get("map_time") > t + 0.5, "Player simulation advances 120 physics frames")
 check(game.get("hero_playing"), "Hero remains active in live simulation")
 check(game.get("visible_regions") >= 1, "Region visibility remains valid")
 check(game.get("beasts").size() == 76, "74 legacy actor IDs plus two divine boss actors remain stable")
 # Divine signature attacks must not leak through a boss transformation.
 var first_z_id:int=int(game.get("divine_boss_ids").get("z",-1))
 if first_z_id>=0:
  var z_in_combat:Dictionary=game.get("beasts")[first_z_id]
  var z_marker:MeshInstance3D=z_in_combat.get("divine_disk") as MeshInstance3D
  z_in_combat["divine_warn_until"]=float(game.get("map_time"))+10.0
  if is_instance_valid(z_marker):z_marker.show()
  z_in_combat["hp"]=float(z_in_combat["max"])*0.60
  game.call("_refresh_boss_phase",z_in_combat)
  check(int(z_in_combat.get("boss_phase",1))==2, "Z transitions into phase two")
  check(is_zero_approx(float(z_in_combat.get("divine_warn_until",1.0))), "Phase change cancels pending divine strike")
  check(is_instance_valid(z_marker) and not z_marker.visible, "Phase change hides the old warning disk")
  game.call("_reset_native_beast",z_in_combat)

 for i in range(6):
  game.call("_award_experience", i % 6, "elder", false)
 check(game.get("hero_level") >= 2, "Experience grants level-up and talent points")
 game.set("gold", 527)
 game.set("weather_name", "Rain")
 game.set("world_day", 3)
 game.call("_complete_world_event", 0, true)
 game.call("_save_game")
 check(game.get("save_health") == "OK", "Real checkpoint is saved")
 var saved: Dictionary = game.call("_read_save")
 check(not saved.is_empty(), "Checkpoint parses and validates")
 check(int(saved.get("gold", 0)) >= 527, "Checkpoint preserves gold and earned event reward")
 check(saved.get("hero", "") == "z", "Checkpoint hero identity stored")
 game.call("_start_game", "nyra")
 check(game.get("hero") == "z" and game.get("gold") == 0, "New adventurer starts with reset state in legacy combat slot")
 game.call("_load_game")
 check(game.get("hero") == "z", "Loading checkpoint restores correct hero")
 check(game.get("world_day") == 3 and game.get("weather_name") == "Rain", "Loading restores world clock and weather")
 check(0 in game.get("events_completed"), "Loading restores event progress")
 check(game.get("hero_level") >= 2 and game.get("gold") >= 527, "Loading restores XP progression and currency")
 check(game.get("hero_sprite").texture != null, "Original hero art remains available after switching and loading")
 for frame in range(60):
  await physics_frame
 check(game.get("hero_playing"), "Loaded hero runs for 60 more physics frames")
 # Exercise divine save/load on the sandbox profile, not on real user files.
 var divine_ids:Dictionary=game.get("divine_boss_ids")
 check(divine_ids.has("z") and divine_ids.has("nyra"), "Stable divine boss IDs registered in live scene")
 if divine_ids.has("z") and divine_ids.has("nyra"):
  var defeats:Array=game.get("defeated_boss_ids")
  defeats.append(int(divine_ids["z"]))
  defeats.append(int(divine_ids["nyra"]))
  var expected_max:float=float(game.get("max_hp"))+30.0
  game.set("max_hp",expected_max)
  game.set("hp",expected_max)
  game.call("_save_game")
  check(game.get("save_health")=="OK", "Divine blessing checkpoint writes successfully")
  var divine_saved:Dictionary=game.call("_read_save")
  var parsed_boss_ids:Array[int]=[]
  for raw_id in divine_saved.get("bosses",[]):
   parsed_boss_ids.append(int(raw_id))
  check(parsed_boss_ids.has(int(divine_ids["z"])) and parsed_boss_ids.has(int(divine_ids["nyra"])), "Both divine victory IDs survive JSON encoding")
  game.call("_start_game","nyra")
  check(not game.call("_divine_blessing_earned","z") and not game.call("_divine_blessing_earned","nyra"), "New adventure clears permanent blessings")
  game.call("_load_game")
  check(game.call("_divine_blessing_earned","z") and game.call("_divine_blessing_earned","nyra"), "Both blessings restore from checkpoint")
  check(is_equal_approx(float(game.get("max_hp")),expected_max), "Nyra max HP restores exactly once")
  check(is_equal_approx(float(game.get("hp")),expected_max), "Nyra bonus HP is not lost on load")
  # A suspended warning must never survive a checkpoint reload.
  var z_actor:Dictionary=game.get("beasts")[int(divine_ids["z"])]
  var disk:MeshInstance3D=z_actor.get("divine_disk") as MeshInstance3D
  z_actor["divine_warn_until"]=float(game.get("map_time"))+20.0
  if is_instance_valid(disk):disk.show()
  game.call("_load_game")
  check(is_equal_approx(float(z_actor.get("divine_warn_until",1.0)),0.0), "Reload cancels pending divine strikes")
  check(is_instance_valid(disk) and not disk.visible, "Reload hides stale divine telegraphs")

 # Custom adventurers use the same schema-2 checkpoint as Z and Nyra.
 # This end-to-end test catches silent rejection by the disk validator.
 game.call("_start_game","custom")
 game.set("custom_hero_name","Moonfall Tester")
 game.set("gold",643)
 game.call("_save_game")
 check(game.get("save_health")=="OK","Custom adventurer checkpoint writes successfully")
 var custom_checkpoint:Dictionary=game.call("_read_save")
 check(str(custom_checkpoint.get("hero",""))=="custom","Custom hero survives checkpoint validation")
 game.call("_start_game","z")
 game.call("_load_game")
 check(str(game.get("hero"))=="custom","Loading restores custom adventurer identity")
 check(str(game.get("custom_hero_name"))=="Moonfall Tester","Loading restores custom adventurer name")
 check(int(game.get("gold"))==643,"Loading restores custom adventurer currency")

 if failures == 0: print("MOONFALL NATIVE v11 LIVE PASS")
 else: push_error("MOONFALL NATIVE v11 LIVE FAILED " + str(failures) + " checks")
 game.queue_free()
 quit(1 if failures > 0 else 0)
