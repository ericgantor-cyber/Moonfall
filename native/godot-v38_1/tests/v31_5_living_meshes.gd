extends SceneTree
const CURRENT_RELEASE = preload("res://tests/current_release.gd")
var failures:int=0
func check(ok:bool,what:String)->void:
 if ok:print("PASS v31.5 "+what)
 else:
  failures+=1
  push_error("FAIL v31.5 "+what)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var build:Script=load("res://scripts/systems/creature_mesh_v315.gd")
 check(build!=null,"3D procedural mesh builder loads")
 for kind in ["wolf","stag","boar","fox","serpent","panther","elk","owl"]:
  var mesh:ArrayMesh=build.sculpt_mesh(kind,"body")
  check(mesh!=null and mesh.get_surface_count()>0 and mesh.surface_get_array_len(0)>100,"Actual 3D mesh "+kind)
  check(build.sculpt_mesh(kind,"body")==mesh,"Shared geometry cache "+kind)
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 check(CURRENT_RELEASE.matches_project(game),"v31.5 is active")
 var tracks:Array=game.get("v315_tracks")
 var beasts:Array=game.get("beasts")
 check(tracks.size()==beasts.size() and tracks.size()>10,"Models attached to real enemy actors")
 var boss_count:int=0
 var diverse:Dictionary={}
 for track in tracks:
  var geo:Dictionary=track["geometry"]
  var root3d:Node3D=geo["root"]
  check(root3d.get_child_count()>=3 and (geo["body"] as MeshInstance3D).mesh is ArrayMesh,"Organic model hierarchy") if track==tracks[0] else null
  diverse[track["kind"]]=true
  if bool(track["boss"]):
   boss_count+=1
   check((geo["crowns"] as Array).size()==2 and geo["weakpoint"]!=null,"Multihead boss and 3D weakpoint") if boss_count==1 else null
 check(diverse.size()==8,"All eight model species integrated")
 check(boss_count>=1,"Bosses have procedural models")
 # The 3D mesh adds no physics shapes: existing character collision stays exactly once.
 var actor:CharacterBody3D=tracks[0]["body"]
 var physics:int=0
 for node in actor.get_children():
  if node is CollisionShape3D:physics+=1
 check(physics==1,"Original creature collision unchanged")
 game.call("_start_game","z")
 await process_frame
 game.call("_v315_tick_visuals")
 var near_count:int=game.get("v315_visible_models")
 check(near_count>=0 and near_count+int(game.get("v315_lod_skips"))==tracks.size(),"LOD accounts for all models")
 game.call("_apply_graphics_quality","Smooth")
 game.call("_v315_tick_visuals")
 check(int(game.get("v315_visible_models"))==0,"Low graphics uses original lightweight rigs")
 game.call("_apply_graphics_quality","Balanced")
 game.call("_v315_tick_visuals")
 var report:Dictionary=game.call("_perf_report")
 check(CURRENT_RELEASE.matches_report(report) and report.has("v315_visual_peak_usec"),"F9 profiling includes v31.5")
 check((game.get("rifts") as Array).size()==3 and (game.get("region_nodes") as Array).size()==6,"Connected world intact")
 game.queue_free()
 print("MOONFALL v31.5 LIVING MESH PASS" if failures==0 else "MOONFALL v31.5 LIVING MESH FAIL")
 quit(0 if failures==0 else 1)
