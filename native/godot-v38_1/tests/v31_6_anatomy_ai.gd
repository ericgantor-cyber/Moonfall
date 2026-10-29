extends SceneTree
const CURRENT_RELEASE = preload("res://tests/current_release.gd")
var failures:int=0
func check(condition:bool,label:String)->void:
 if condition:print("PASS v31.6 "+label)
 else:
  failures+=1
  push_error("FAIL v31.6 "+label)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 check(CURRENT_RELEASE.matches_project(game),"new scene active")
 var tracks:Array=game.get("v315_tracks")
 check(tracks.size()>10,"Real creatures have anatomy")
 var families:Dictionary={}
 var bosses:int=0
 for track in tracks:
  var detail:Dictionary=track["v316"]
  var species:String=track["kind"]
  families[species]=true
  var joints:Array=detail["legs"]
  check(joints.size()==(0 if species=="serpent" else 2 if species=="owl" else 4),"Species-specific limbs "+species) if not families.has("checked_"+species) else null
  families["checked_"+species]=true
  if bool(track["boss"]):
   bosses+=1
   check(detail["aura"] is MeshInstance3D,"Boss aura is real mesh") if bosses==1 else null
 check(bosses>0 and families.has("serpent") and families.has("owl"),"All species and bosses present")
 var creature:Dictionary=tracks[0]["creature"]
 var actor:CharacterBody3D=tracks[0]["body"]
 var collisions:int=0
 for child in actor.get_children():
  if child is CollisionShape3D:collisions+=1
 check(collisions==1,"Existing hitbox retained")
 game.call("_start_game","z")
 await process_frame
 game.call("_apply_graphics_quality","Smooth")
 game.call("_v315_tick_visuals")
 check(int(game.get("v315_visible_models"))==0,"Smooth preset has cheap LOD")
 game.call("_apply_graphics_quality","Balanced")
 # Put the hero near one alive actor in the current region and verify real pose updates.
 var hero:CharacterBody3D=game.get("player")
 var target_track:Dictionary={}
 for entry in tracks:
  if int(entry["zone"])==int(game.get("region_index")) and bool(entry["creature"].get("alive",false)):
   target_track=entry
   break
 if not target_track.is_empty():
  hero.global_position=(target_track["body"] as CharacterBody3D).global_position+Vector3(1.0,0,1.0)
 game.call("_v315_tick_visuals")
 check(int(game.get("v316_limb_updates"))>0,"Real leg pivots animated near hero")
 var report:Dictionary=game.call("_perf_report")
 check(CURRENT_RELEASE.matches_report(report) and report.has("v316_ai_broadcasts_deferred"),"F9 profiler reports AI budget")
 check(int(report.get("v316_articulated_limb_nodes",0))>0,"Articulated 3D limbs counted")
 check((game.get("region_nodes") as Array).size()==6 and (game.get("rifts") as Array).size()==3,"Single connected world preserved")
 game.queue_free()
 print("MOONFALL v31.6 ANATOMY AND AI PASS" if failures==0 else "MOONFALL v31.6 ANATOMY AND AI FAIL")
 quit(0 if failures==0 else 1)
