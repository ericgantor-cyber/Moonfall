extends SceneTree
## Godot headless integration test; NOT satisfied by offline source checks.
const RULES:=preload("res://scripts/systems/living_realm_v37.gd")
var failures:int=0
func _initialize()->void:call_deferred("_run")
func check(ok:bool,description:String)->void:
 if ok:print("PASS v37: "+description)
 else:
  failures+=1
  push_error("FAIL v37: "+description)
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"main scene imports")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check((game.get("v37_wildlife") as Array).size()==12,"twelve capped passive wildlife")
 check((game.get("v37_wakes") as Array).size()==6,"one reusable swim wake per region")
 check((game.get("v37_ward_lights") as Array).size()==6,"six regional world-state beacons")
 check((game.get("v362_water_roots") as Array).size()==6,"original six water basins")
 check((game.get("v362_water_sites") as Array).size()==18,"original 18 gathering spots")
 var regions:Array=[]
 for i in range(6):regions.append({"at":Vector2(float(i)*100.0,0.0)})
 if regions.size()>=6:
  var loc:Vector2=regions[0]["at"]
  check(RULES.swim_zone(loc.x+18.0,loc.y+14.0,regions)==0,"surface swimming at lake center")
  check(RULES.swim_zone(9999.0,9999.0,regions)==-1,"no swimming outside lakes")
 check(RULES.SWIM_SPEED_MULTIPLIER>0.0 and RULES.SWIM_SPEED_MULTIPLIER<1.0,"swimming slows third-person movement")
 check(is_equal_approx(RULES.ward_damage(100.0,true),95.0),"purified region reduces incoming damage")
 check(is_equal_approx(RULES.ward_damage(100.0,false),100.0),"corrupted region grants no bonus")
 check(RULES.wildlife_available(2,4,{}),"wildlife starts available")
 check(not RULES.wildlife_available(2,4,{"20":4}),"harvested wildlife cannot be harvested twice same day")
 check(RULES.wildlife_available(2,5,{"20":4}),"wildlife recovers the next in-game day")
 check(RULES.contract_complete(3,{"27":5}),"contract completion stored in existing checkpoint field")
 check(not RULES.contract_complete(3,{}),"new game has open contracts")
 var inventory:Dictionary={"Moonwater Fish":3,"River Reed":2}
 var cost:Dictionary=RULES.contract_cost(0)
 check(RULES.missing(cost,inventory)=="","contract validates material costs")
 RULES.pay(cost,inventory)
 check(int(inventory["Moonwater Fish"])==1,"contract deducts material cost exactly once")
 check(RULES.missing(cost,inventory)!="","contract cannot be repeated with insufficient materials")
 var original_sprite:Sprite3D=game.get("hero_sprite") as Sprite3D
 check(is_instance_valid(original_sprite) and original_sprite.texture!=null,"original hero art remains in the 3D world")
 game.queue_free()
 print("MOONFALL v37 LIVING REALM PASS" if failures==0 else "MOONFALL v37 LIVING REALM FAIL")
 quit(0 if failures==0 else 1)
