extends SceneTree
## Invisible-grid tests deliberately verify gameplay state, not a grid visual.
var failures := 0
func check(condition:bool,why:String) -> void:
 if condition:print("PASS v18: "+why)
 else:
  failures+=1
  push_error("FAIL v18: "+why)
func _initialize() -> void:call_deferred("_run")
func _run() -> void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"Scene parses")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 var regions:PackedByteArray=game.get("grid_regions")
 var blocked:PackedByteArray=game.get("grid_blocked")
 check(regions.size()==2400 and blocked.size()==2400,"60 x 40 logical grid is allocated")
 check(game.get("grid_blocked_count")>0 and game.get("grid_blocked_count")<800,"Obstacles indexed without blocking the entire world")
 check(game.get_node_or_null("TerrainGrid")==null,"No visible terrain-grid mesh or tile nodes")
 for z in range(2):
  for x in range(3):
   var at:=Vector3(-60.0+60.0*x,0.85,-30.0+60.0*z)
   check(game.call("_grid_region_at",at)==z*3+x,"Cell lookup correctly maps world region "+str(z*3+x))
 check(game.call("_grid_region_at",Vector3(150,0,0))==-1,"Out-of-map cell rejected")
 check(not game.call("_grid_is_open",Vector3(150,0,0)),"Outside-world area cannot be traversed")
 check(game.call("_grid_is_open",Vector3(-60,0,-30)),"Starting outpost area stays open")
 game.call("_start_game","z")
 check(game.call("_region_here")==0,"Player spawn uses grid to identify outpost")
 var nearby:Dictionary={}
 for creature in game.get("beasts"):
  if int(creature.get("zone",-1))==0 and bool(creature.get("hostile",false)) and not bool(creature.get("boss",false)):
   nearby=creature
   break
 check(not nearby.is_empty(),"Outpost enemy available to test navigation")
 if not nearby.is_empty():
  var creature_body:CharacterBody3D=nearby["body"]
  game.get("player").global_position=creature_body.global_position+Vector3(2.5,0,0)
  game.set("region_index",0)
  var before:int=game.get("grid_queries")
  game.call("_physics_process",0.016)
  check(game.get("grid_queries")>before,"Live enemy simulation calls grid obstacle steering")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Z_Original.webp"),"Z original artwork retained")
 game.call("_start_game","nyra")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Nyra_Original.png"),"Nyra original artwork retained")
 check(game.get("grid_regions").size()==2400,"Grid remains stable after hero switching")
 if failures==0:print("MOONFALL NATIVE v18 GRID PASS")
 else:push_error("MOONFALL NATIVE v18 GRID FAILED: "+str(failures))
 game.queue_free()
 quit(0 if failures==0 else 1)
