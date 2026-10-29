extends SceneTree
## v36.2 runtime smoke test. Run via Godot 4.7.2 in the CI suite.
const RULES:=preload("res://scripts/systems/water_professions_v362.gd")
var failures:int=0
func _initialize()->void:call_deferred("_run")
func check(ok:bool,description:String)->void:
 if ok:print("PASS WATER: "+description)
 else:
  failures+=1
  push_error("FAIL WATER: "+description)
func _run()->void:
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 var water:Array=game.get("v362_water_roots")
 var sites:Array=game.get("v362_water_sites")
 check(water.size()==6,"six regional water bodies")
 check(sites.size()==18,"three gathering sites per region")
 check(game.get("water_professions").size()==6,"six durable profession skills")
 check(water[0].get_child_count()>=10,"shores include stones, reeds, and resource markers")
 check((water[0].get_child(0) as MeshInstance3D).mesh is PlaneMesh,"water surface uses world geometry")
 check((water[0].get_child(0) as MeshInstance3D).material_override is ShaderMaterial,"animated water material")
 var resource_days:Dictionary={"2":3}
 check(not RULES.can_gather(2,3,resource_days),"harvest cannot repeat in same day")
 check(RULES.can_gather(2,4,resource_days),"resources replenish next in-game day")
 check(RULES.gather_yield("Fishing",75,0).get("Moonwater Fish",0)==2,"fishing skill improves yields")
 check(RULES.gather_yield("Mining",1,1).get("Ore Shard",0)==3,"mining grants ore")
 var materials:Dictionary={"Ore Shard":3,"Dusk Cinder":1}
 var forge:Dictionary=RULES.RECIPES[2]
 check(RULES.recipe_issue(forge,materials,0,100)=="","valid recipe recognized")
 RULES.consume_recipe(forge,materials)
 check(int(materials["Ore Shard"])==0 and int(materials["Dusk Cinder"])==0,"recipe consumes costs only")
 check(RULES.recipe_issue(forge,materials,0,100)!="","insufficient ingredients blocked")
 check((game.get("hero_sprite") as Sprite3D).texture!=null,"hero artwork remains intact")
 game.queue_free()
 print("MOONFALL v36.2 WATER PROFESSIONS PASS" if failures==0 else "MOONFALL v36.2 WATER PROFESSIONS FAIL")
 quit(0 if failures==0 else 1)
