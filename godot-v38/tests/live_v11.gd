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
 check(game.get("beasts").size() == 62, "Wildlife actor list stable")
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
 check(game.get("hero") == "nyra" and game.get("gold") == 0, "New hero starts with reset state")
 game.call("_load_game")
 check(game.get("hero") == "z", "Loading checkpoint restores correct hero")
 check(game.get("world_day") == 3 and game.get("weather_name") == "Rain", "Loading restores world clock and weather")
 check(0 in game.get("events_completed"), "Loading restores event progress")
 check(game.get("hero_level") >= 2 and game.get("gold") >= 527, "Loading restores XP progression and currency")
 check(game.get("hero_sprite").texture != null, "Original hero art remains available after switching and loading")
 for frame in range(60):
  await physics_frame
 check(game.get("hero_playing"), "Loaded hero runs for 60 more physics frames")
 if failures == 0: print("MOONFALL NATIVE v11 LIVE PASS")
 else: push_error("MOONFALL NATIVE v11 LIVE FAILED " + str(failures) + " checks")
 game.queue_free()
 quit(1 if failures > 0 else 0)
