extends SceneTree
## Godot 4.7.2 runtime combat target selection smoke test.
var failed:int=0
func check(ok:bool,label:String)->void:
 if ok:print("PASS simulation: "+label)
 else:
  failed+=1
  push_error("FAIL simulation: "+label)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 var hero:CharacterBody3D=game.get("player")
 check(is_instance_valid(hero),"player available")
 if not is_instance_valid(hero):quit(1);return
 var near:CharacterBody3D=CharacterBody3D.new()
 near.position=hero.global_position+Vector3(0,0,-4)
 game.add_child(near)
 var off:CharacterBody3D=CharacterBody3D.new()
 off.position=hero.global_position+Vector3(4,0,0)
 game.add_child(off)
 var zones:Array=game.get("zone_beast_lists")
 var region:int=int(game.get("region_index"))
 if region<0:
  region=0
  game.set("region_index",0)
 var a:Dictionary={"alive":true,"hostile":true,"body":near}
 var b:Dictionary={"alive":true,"hostile":true,"body":off}
 zones[region]=[a,b]
 game.set("zone_beast_lists",zones)
 game.set("aim_direction",Vector3(0,0,-1))
 game.set("aim_locked_id",-1)
 var target:Dictionary=game.call("_aim_candidate",13.0)
 check(target==a,"forward target selected")
 game.set("aim_locked_id",99)
 target=game.call("_aim_candidate",13.0)
 check(target==a and int(game.get("aim_locked_id"))==-1,"stale lock cleared")
 a["alive"]=false
 target=game.call("_aim_candidate",13.0)
 check(target.is_empty(),"dead target excluded and off-axis ignored")
 game.queue_free()
 quit(1 if failed>0 else 0)
