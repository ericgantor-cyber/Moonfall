extends SceneTree
const CURRENT_RELEASE = preload("res://tests/current_release.gd")
var errors:int=0
func check(ok:bool,label:String)->void:
 if ok:print("PASS BESTIARY "+label)
 else:
  errors+=1
  push_error("FAIL BESTIARY "+label)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 for key in ["wolf","stag","boar","fox","serpent","panther","elk","owl","boss_0","boss_1","boss_2","boss_3","region_0","region_1","region_2","region_3","region_4","region_5"]:
  var image:Texture2D=load("res://assets/v31_4_bestiary/"+key+".png")
  check(image!=null and image.get_width()==256 and image.get_height()==256,"Asset "+key)
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 check(CURRENT_RELEASE.matches_project(game),"Release activated")
 check((game.get("v314_region_signs") as Array).size()==6,"Six region signs")
 var badges:Array=game.get("v314_portraits")
 check(badges.size()==(game.get("beasts") as Array).size(),"Every creature has a portrait")
 var boss_total:int=0
 for entry in badges:
  var art:Sprite3D=entry["node"]
  check(art!=null and art.texture!=null and art.name=="V314_BestiaryPortrait","Portrait texture attached") if entry==badges[0] else null
  if bool(entry["boss"]):boss_total+=1
 check(boss_total>=1,"Boss portraits attached")
 game.call("_start_game","z")
 await process_frame
 game.call("_v314_update_visuals")
 check(int(game.get("v314_animated_count"))+int(game.get("v314_culled_count"))==badges.size(),"Every portrait accounted for by LOD")
 var p:Dictionary=game.call("_perf_report")
 check(p.get("version")in ["v31.4","v31.5","v31.6","v32.0","v32.1"] and p.has("bestiary_visual_update_usec"),"F9 profiler fields")
 check((game.get("rifts") as Array).size()==3 and (game.get("v312_foliage") as Array).size()==6,"World and scenery retained")
 game.queue_free()
 print("MOONFALL v31.4 BESTIARY PASS" if errors==0 else "MOONFALL v31.4 BESTIARY FAIL")
 quit(0 if errors==0 else 1)
