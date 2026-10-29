extends SceneTree
var failures:int=0
func verify(ok:bool,description:String)->void:
 if ok: print("PASS v31.6 second pass "+description)
 else:
  failures+=1
  push_error("FAIL v31.6 second pass "+description)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 await process_frame
 game.call("_apply_graphics_quality","Balanced")
 var hero:CharacterBody3D=game.get("player")
 var tracks:Array=game.get("v315_tracks")
 verify(tracks.size()>18,"Enough real creature models for stress test")
 # Force 24 pre-existing actors into visual range to exercise overflow.
 var prior_region:int=int(game.get("region_index"))
 var changed:Array=[]
 for i in range(mini(24,tracks.size())):
  var track:Dictionary=tracks[i]
  changed.append([track,track["zone"],(track["body"] as CharacterBody3D).global_position])
  track["zone"]=prior_region
  track["creature"]["alive"]=true
  var body:CharacterBody3D=track["body"]
  body.visible=true
  body.global_position=hero.global_position+Vector3(float(i%5)*0.2,0.0,float(i/5)*0.2)
 game.call("_v315_tick_visuals")
 var visible:int=int(game.get("v315_visible_models"))
 verify(visible<=18 and visible>0,"Detailed models have a strict 18-actor cap")
 verify(int(game.get("v316_detail_culled"))>=1,"Overflow reverts to cheap original rigs")
 verify(visible+int(game.get("v315_lod_skips"))==tracks.size(),"LOD counters include every actor")
 var report:Dictionary=game.call("_perf_report")
 verify(report.has("v316_detail_actor_budget") and int(report["v316_detail_actor_budget"])==18,"Profiler exposes model budget")
 # Only one temporary pack-broadcast deferral, no starvation or timer creep.
 var actor:Dictionary=tracks[0]["creature"]
 actor["engaged"]=true
 actor["alive"]=true
 actor["hostile"]=true
 actor["boss"]=false
 actor["v315_next_signal"]=-1.0
 game.set("v316_ai_remaining",0)
 var original:float=float(actor["v315_next_signal"])
 game.call("_update_beast_step",actor,0.016)
 verify(is_equal_approx(float(actor.get("v315_next_signal",-999.0)),original),"Deferred pack alert retains original eligibility")
 verify(int(game.get("v316_ai_deferred"))>0,"Deferred alert recorded")
 verify((game.get("region_nodes") as Array).size()==6 and (game.get("rifts") as Array).size()==3,"Connected world unchanged")
 game.queue_free()
 print("MOONFALL v31.6 SECOND PASS PASS" if failures==0 else "MOONFALL v31.6 SECOND PASS FAIL")
 quit(0 if failures==0 else 1)
