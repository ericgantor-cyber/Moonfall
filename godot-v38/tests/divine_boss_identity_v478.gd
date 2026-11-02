extends SceneTree
const IDENTITY=preload("res://scripts/systems/divine_boss_identity.gd")
func _initialize()->void:
 call_deferred("_run")
func _run()->void:
 for id in ["z","nyra"]:
  var entry:Dictionary=IDENTITY.boss(id)
  assert(entry.get("role")=="god_boss")
  var image_path:String=str(entry.get("art",""))
  assert(ResourceLoader.exists(image_path))
  assert(load(image_path)!=null)
 assert(not IDENTITY.is_divine_boss("player"))
 print("DIVINE BOSS IDENTITY PASS")
 quit(0)
