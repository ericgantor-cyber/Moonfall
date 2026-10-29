extends SceneTree
const RULES:=preload("res://scripts/systems/deep_water_v373.gd")
const LAKE_RULES:=preload("res://scripts/systems/living_realm_v37.gd")
var failures:int=0
func _initialize()->void:call_deferred("_run")
func check(ok:bool,label:String)->void:
 if ok:print("PASS deep water: "+label)
 else:
  failures+=1
  push_error("FAIL deep water: "+label)
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"active scene imports")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check((game.get("v373_caves") as Array).size()==6,"six caves spawned")
 check((game.get("v373_cache_positions") as Array).size()==6,"six caches spawned")
 check(is_instance_valid(game.get("v373_underwater_tint")),"underwater HUD exists")
 var depth:float=0.0
 for i in range(14):depth=RULES.advance_depth(depth,0.25,true,true,false)
 check(depth>2.0,"C descends through real Y depth")
 check(RULES.truly_submerged(depth),"oxygen uses physical depth")
 var surface:float=RULES.advance_depth(depth,0.25,true,false,true)
 check(surface<depth,"ascend key rises")
 for i in range(18):surface=RULES.advance_depth(surface,0.25,false,false,false)
 check(is_zero_approx(surface),"leaving water returns to surface")
 check(not RULES.cache_ready(0,2,{"30":2}),"cache respects day cooldown")
 check(RULES.cache_ready(0,3,{"30":2}),"cache refreshes tomorrow")
 var regions:Array=[]
 for i in range(6):regions.append({"at":Vector2(float(i)*100.0,0.0)})
 check(LAKE_RULES.swim_zone(18.0+5.4,14.0,regions,-2.5)==0,"side grotto remains swimmable")
 check(LAKE_RULES.swim_zone(18.0+5.4,14.0,regions,0.85)==-1,"cave access requires submersion")
 game.queue_free()
 print("MOONFALL v37.3 DEEP WATER PASS" if failures==0 else "MOONFALL v37.3 DEEP WATER FAIL")
 quit(0 if failures==0 else 1)
