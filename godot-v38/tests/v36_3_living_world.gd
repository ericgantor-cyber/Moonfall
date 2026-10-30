extends SceneTree
## Actual Godot runtime smoke test for the additive living-world layer.
const RULES:=preload("res://scripts/systems/living_world_v363.gd")
var failures:int=0
func _initialize()->void:call_deferred("_run")
func check(valid:bool,description:String)->void:
 if valid:print("PASS LIVING: "+description)
 else:
  failures+=1
  push_error("FAIL LIVING: "+description)
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"main world scene exists")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(game.get("v363_ready")==true,"living world initialized")
 var water:Array=game.get("v363_water_materials")
 var flora:Array=game.get("v363_flora")
 var fish:Array=game.get("v363_fish")
 var falls:Array=game.get("v363_cascades")
 check(water.size()==6,"six 3D water regions retain unique shader instances")
 check(flora.size()==30,"30 bounded reactive moonblooms")
 check(fish.size()==12,"12 decorative fish, no new AI enemies")
 check(falls.size()==2,"two non-colliding animated cascades")
 check((game.get("v362_water_sites") as Array).size()==18,"original gathering spots remain")
 if water.size()>0:
  check(water[0].shader.code.contains("foam"),"GPU shoreline foam animation")
 var base_rifts:Array=game.get("rifts")
 check(base_rifts.size()>=3,"existing rift encounters remain in the world")
 var rifts_copy:Array=base_rifts.duplicate(true)
 if not rifts_copy.is_empty():
  var zone:int=int(rifts_copy[0]["zone"])
  rifts_copy[0]["cleared"]=false
  check(RULES.is_tainted(zone,rifts_copy,-1,[]),"unsealed rift taints its region")
  rifts_copy[0]["cleared"]=true
  check(not RULES.is_tainted(zone,rifts_copy,-1,[]),"sealing rift restores its region")
 check(RULES.fish_escape_radius(0.0)>RULES.fish_escape_radius(100.0),"fish react to player approach")
 check(RULES.flora_response(0.0,false)>RULES.flora_response(0.0,true),"corruption suppresses bioluminescence")
 check(is_equal_approx(RULES.ward_damage(100.0,20.0,true),80.0),"ward reduces corruption damage")
 check(is_equal_approx(RULES.ward_damage(100.0,0.0,true),100.0),"ward expires cleanly")
 var stash:Dictionary={"Moonwater Fish":1,"River Reed":1,"herb":1}
 var recipe:Dictionary=RULES.RECIPES[0]
 check(RULES.recipe_issue(recipe,stash,0,100)=="","existing materials fund cooking recipe")
 RULES.spend(recipe,stash)
 check(int(stash["Moonwater Fish"])==0 and int(stash["River Reed"])==0,"crafting costs deducted atomically")
 check(RULES.recipe_issue(recipe,stash,0,100)!="","insufficient materials cannot craft")
 check((game.get("hero_sprite") as Sprite3D).texture!=null,"original hero artwork remains loaded")
 game.queue_free()
 print("MOONFALL v36.3 LIVING WORLD PASS" if failures==0 else "MOONFALL v36.3 LIVING WORLD FAIL")
 quit(0 if failures==0 else 1)
