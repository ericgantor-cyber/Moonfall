extends SceneTree
## Run: godot --headless --path godot-v38 --script res://tests/aim_spell_integration.gd
var fails:int=0
func test(ok:bool,name:String)->void:
 if ok:print("PASS integration: "+name)
 else:fails+=1;push_error("FAIL integration: "+name)
func _initialize()->void:call_deferred("run_test")
func run_test()->void:
 var packed:PackedScene=load("res://scenes/main.tscn")
 if packed==null:quit(1);return
 var game:Node3D=packed.instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 var player:Node3D=game.get("player")
 var zones:Array=game.get("zone_beast_lists")
 var region:int=int(game.get("region_index"))
 var enemies:Array=zones[region]
 var target:Dictionary={}
 for b in enemies:
  if bool(b.get("hostile",false)) and bool(b.get("alive",false)):
   target=b
   break
 test(not target.is_empty(),"hostile target found in game world")
 if target.is_empty():quit(1);return
 var enemy_body:Node3D=target["body"]
 enemy_body.global_position=player.global_position+Vector3(0,0,-5)
 game.set("aim_direction",Vector3(0,0,-1))
 game.set("aim_locked_id",-1)
 var candidate:Dictionary=game.call("_aim_candidate",13.0)
 test(candidate==target,"visible forward enemy acquired")
 var old_hp:float=float(target["hp"])
 game.set("ability_cooldown",0.0)
 game.call("_spell","fire")
 test(float(target["hp"])<old_hp,"aimed flame causes real HP loss")
 game.set("aim_locked_id",-1)
 game.call("_toggle_aim_lock")
 test(int(game.get("aim_locked_id"))>=0,"target lock acquired")
 game.call("_toggle_aim_lock")
 test(int(game.get("aim_locked_id"))==-1,"target lock released")
 game.set("aim_locked_id",999999)
 candidate=game.call("_aim_candidate",13.0)
 test(int(game.get("aim_locked_id"))==-1,"stale lock recovered")
 game.queue_free()
 quit(1 if fails>0 else 0)
