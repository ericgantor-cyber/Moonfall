extends SceneTree
const GAME=preload("res://scripts/native_v38_10.gd")
var failed:int=0
func check(ok:bool,label:String)->void:
 if not ok:
  failed+=1
  push_error(label)
func _initialize()->void:call_deferred("run")
func run()->void:
 var source:String=FileAccess.get_file_as_string("res://scripts/native_v38_10.gd")
 check(source.contains("_depth_attunement_count(site)"),"glyph count derives from existing registry")
 check(source.contains('depth_glyph_charged.has(key)'),"individual chamber glyph state shown")
 check(source.contains('MOON GLYPH • '),"glyph sign text exists")
 check(source.contains('if prompt.is_empty():return super._v321_chamber_label(entry,stage)'),"inactive and cleared signs preserved")
 check(GAME!=null and load("res://scenes/main.tscn")!=null,"live scene parses")
 quit(1 if failed else 0)
