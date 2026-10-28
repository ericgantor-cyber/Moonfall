extends SceneTree
# CI-only GDScript inheritance diagnosis. No game assets modified.
func _initialize() -> void:
 var chain: Array[String] = []
 var current := "res://scripts/native_v38_0.gd"
 for index in range(85):
  if not FileAccess.file_exists(current):
   print("MOONFALL_PROBE MISSING ",current)
   break
  chain.append(current)
  var data: String = FileAccess.get_file_as_string(current)
  var first_line: String = data.split("\n")[0].strip_edges()
  if not first_line.begins_with('extends "res://scripts/'):
   break
  current = first_line.trim_prefix('extends "').trim_suffix('"')
 chain.reverse()
 print("MOONFALL_PROBE TESTING ",chain.size()," LEVELS ROOT-FIRST")
 for script_path in chain:
  print("MOONFALL_PROBE BEFORE ",script_path)
  var script = ResourceLoader.load(script_path, "GDScript", ResourceLoader.CACHE_MODE_IGNORE)
  print("MOONFALL_PROBE AFTER ",script_path," ",("LOADED" if script != null else "FAILED"))
 quit(0)
