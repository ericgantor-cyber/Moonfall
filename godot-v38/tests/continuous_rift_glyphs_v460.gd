extends SceneTree
## Static integration checks plus Godot scene/script parse.
const GAME=preload("res://scripts/native_v38_10.gd")
var failed:int=0
func check(ok:bool,label:String)->void:
 if ok:print("PASS glyph: "+label)
 else:
  failed+=1
  push_error("FAIL glyph: "+label)
func _initialize()->void:call_deferred("run")
func run()->void:
 var source:String=FileAccess.get_file_as_string("res://scripts/native_v38_10.gd")
 check(source.contains("func _depth_tick(delta:float)"),"live glyph labels override")
 check(source.contains("func _interact()"),"live glyph interaction override")
 check(source.contains("depth_glyph_charged[key]=true"),"glyph activation persists in existing registry")
 check(source.contains("depth_spell_charges=mini(3,depth_spell_charges+1)"),"echo charges remain capped")
 check(source.contains('active_rift!=site or bool(rifts[site].get("cleared",false))'),"inactive or sealed rifts guarded")
 check(not source.contains('int(rifts[site].get("stage",0))!=stage'),"no stage gate in new interaction handler")
 check(source.contains('super._interact()'),"other interactions preserved")
 check(source.contains("RIFT_OBJECTIVES.chamber_prompt(entry,stage)"),"all chamber labels use wave-free objectives")
 check(source.contains("func _v321_chamber_label("),"wave-free labels use cached parent hook")
 check(not source.contains("func _v321_update_chambers("),"no duplicate per-frame label writer")
 var base_labels:String=FileAccess.get_file_as_string("res://scripts/native_v32_2.gd")
 check(base_labels.contains("var next_text:String=_v321_chamber_label(entry,stage)"),"cached label path calls virtual objective hook")
 check(not source.contains("RIFT_OBJECTIVES.stage_active("),"legacy wave stage no longer gates room labels")
 check(source.contains("func _menu_tab(which:String)"),"world instructions override exists")
 check(source.contains("explore its three chambers without waves."),"legacy wave tutorial replaced")
 check(source.contains('str(entry["name"])+"  EXPLORING"'),"active rifts show exploration status")
 check(GAME!=null and load("res://scenes/main.tscn")!=null,"game script and scene parse")
 if failed==0:print("MOONFALL RIFT GLYPHS PASS")
 quit(1 if failed else 0)
