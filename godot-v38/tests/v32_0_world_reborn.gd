extends SceneTree
const CURRENT_RELEASE = preload("res://tests/current_release.gd")
var failures:int=0
func check(ok:bool,message:String)->void:
 if ok:print("PASS v32 world "+message)
 else:
  failures+=1
  push_error("FAIL v32 world "+message)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 check(CURRENT_RELEASE.matches_project(game),"New scene and release label")
 check((game.get("region_nodes") as Array).size()==6,"All six connected regions")
 check((game.get("rifts") as Array).size()==3,"Three existing connected rifts")
 check((game.get("v320_decor") as Array).size()==6,"Six instanced scenery batches")
 check((game.get("v320_wardens") as Array).size()==6,"Six regional NPCs")
 check((game.get("v320_impacts") as Array).size()==8,"Reusable combat impact pool")
 check((game.get("v320_boss_cues") as Array).size()>0,"Boss windup indicators attached")
 var batches:Array=game.get("v320_decor")
 for batch in batches:
  check((batch as MultiMeshInstance3D).multimesh.instance_count==42,"Instanced scenery batch size")
 game.call("_start_game","z")
 await process_frame
 var actor:CharacterBody3D=game.get("player")
 var npc:Node3D=(game.get("v320_wardens") as Array)[0]["node"]
 actor.global_position=npc.global_position+Vector3(1.0,0.85,1.0)
 game.call("_v320_stream_update")
 check((game.get("v320_stream_visible") as int)>=1,"Near scenery streamed into view")
 check(not bool((game.get("v320_decor") as Array)[5].visible),"Distant scenery hidden")
 check((game.get("v320_wardens") as Array)[0]["node"].visible,"Warden visible near hero")
 # Avoid interfering with older save data: test writes to isolated user-data root.
 game.set("contract_zone",-1)
 game.set("contracts_done",[])
 game.call("_interact")
 check(int(game.get("contract_zone"))==0,"Warden uses original persistent region contract")
 check(int(game.get("v320_warden_interactions"))==1,"Warden interaction counted")
 var report:Dictionary=game.call("_perf_report")
 check(str(report.get("version","")).begins_with("v3") and report.has("world_stream_peak_usec"),"F9 profiler shows World Reborn counters")
 check(report["regional_npc_wardens"]==6,"NPC profiling count")
 game.queue_free()
 print("MOONFALL v32 WORLD REBORN PASS" if failures==0 else "MOONFALL v32 WORLD REBORN FAIL")
 quit(0 if failures==0 else 1)
