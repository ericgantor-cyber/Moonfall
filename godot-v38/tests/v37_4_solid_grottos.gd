extends SceneTree
var failures:int=0
func _initialize()->void:call_deferred("_run")
func check(ok:bool,label:String)->void:
 if ok:print("PASS solid grottos: "+label)
 else:
  failures+=1
  push_error("FAIL solid grottos: "+label)
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"active scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check((game.get("v373_caves") as Array).size()==6,"six underwater grottos intact")
 var bodies:Array=game.get("v374_collision_bodies")
 check(bodies.size()==78,"13 persistent solid bodies per grotto")
 var valid:bool=true
 for body in bodies:
  if not (body is StaticBody3D):valid=false;break
  if body.collision_layer!=1 or body.get_child_count()<1:valid=false;break
  if not (body.get_child(0) is CollisionShape3D):valid=false;break
 check(valid,"boundaries have layer-1 Godot collision shapes")
 var visible_walls:int=0
 for body in bodies:
  if body.has_node("VisibleCaveStone"):visible_walls+=1
 check(visible_walls==18,"three visible solid cave walls per region")
 # These engine-level queries verify actual physics registration, not just nodes.
 await physics_frame
 var cache:Vector3=(game.get("v373_cache_positions") as Array)[0]
 var state:PhysicsDirectSpaceState3D=game.get_world_3d().direct_space_state
 var side_query:=PhysicsRayQueryParameters3D.create(cache+Vector3(-0.45,0.0,0.0),cache+Vector3(-0.45,0.0,2.5))
 side_query.collision_mask=1
 var side_hit:Dictionary=state.intersect_ray(side_query)
 check(not side_hit.is_empty() and (side_hit.get("collider") as Node).name=="SolidSideWall_1","physics ray strikes the grotto's visible side wall")
 var floor_query:=PhysicsRayQueryParameters3D.create(cache+Vector3(-0.45,0.0,0.0),cache+Vector3(-0.45,-2.0,0.0))
 floor_query.collision_mask=1
 var floor_hit:Dictionary=state.intersect_ray(floor_query)
 check(not floor_hit.is_empty() and (floor_hit.get("collider") as Node).name=="SolidLakeBed","physics ray strikes the submerged floor")
 check(game.get("v373_original_collision_mask")!=0,"player keeps original collision mask")
 check(is_instance_valid(game.get("hero_sprite")),"original hero billboard remains")
 game.queue_free()
 print("MOONFALL v37.4 SOLID GROTTOS PASS" if failures==0 else "MOONFALL v37.4 SOLID GROTTOS FAIL")
 quit(0 if failures==0 else 1)
