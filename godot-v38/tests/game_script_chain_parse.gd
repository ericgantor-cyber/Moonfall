extends SceneTree
## Compile the actual game's full script inheritance chain, not just source text.
## This catches missing ancestor methods before the longer Windows export job.

func _initialize() -> void:
 var game_script:GDScript=load("res://scripts/native_v38_9.gd") as GDScript
 if game_script==null:
  push_error("Moonfall game script failed to load")
  quit(1)
  return
 var result:Error=game_script.reload()
 if result!=OK or not game_script.can_instantiate():
  push_error("Moonfall game script inheritance chain did not compile")
  quit(1)
  return
 print("MOONFALL GAME SCRIPT CHAIN PARSE PASS")
 quit(0)
