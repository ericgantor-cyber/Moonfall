extends SceneTree
const GAME=preload("res://scripts/native_v38_10.gd")
var failed:int=0
func check(value:bool,message:String)->void:
 if not value:
  failed+=1
  push_error(message)
func _initialize()->void:call_deferred("run")
func run()->void:
 var script_text:String=FileAccess.get_file_as_string("res://scripts/native_v38_10.gd")
 check(script_text.contains('marker.name="RiftHiddenInscription"'),"inscription marker exists")
 check(script_text.contains('V321_RULES.room_position(anchor,2)'),"existing second chamber location reused")
 check(script_text.contains('secret["read"]=true'),"inscription read state updated")
 check(script_text.contains('site!=active_rift'),"non-active rift cannot be read")
 check(script_text.contains('super._interact()'),"other interactions preserved")
 check(GAME!=null and load("res://scenes/main.tscn")!=null,"active Godot scene loads")
 quit(1 if failed else 0)
