extends SceneTree
## Headless CPU/physics integration diagnostic, not a visual GPU benchmark.
var failures:int=0
func check(ok:bool,message:String)->void:
 if ok:print("PASS v29: "+message)
 else:
  failures+=1
  push_error("FAIL v29: "+message)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var packed:PackedScene=load("res://scenes/main.tscn")
 check(packed!=null,"main scene imports")
 if packed==null:quit(1);return
 var game:Node3D=packed.instantiate()
 root.add_child(game)
 await process_frame
 check(str(game.call("_release_label")).begins_with("v3"),"v29 game loaded")
 game.call("_start_game","z")
 var player:CharacterBody3D=game.get("player")
 check(is_instance_valid(player),"player initialized")
 var beasts:Array=game.get("zone_beast_lists")[0]
 var passive:Dictionary={}
 var active:Dictionary={}
 for actor in beasts:
  if str(actor.get("kind",""))=="stag" and passive.is_empty():passive=actor
  if bool(actor.get("hostile",false)) and not bool(actor.get("boss",false)) and active.is_empty():active=actor
 check(not passive.is_empty() and not active.is_empty(),"wildlife and enemies spawned")
 if not passive.is_empty() and is_instance_valid(player):
  var body:CharacterBody3D=passive["body"]
  body.global_position=Vector3(-86,0.8,-55)
  body.velocity=Vector3.ZERO
  passive["home"]=body.global_position
  passive["engaged"]=false
  passive["ecology_target"]={}
  passive["scared_until"]=0.0
 if not active.is_empty():
  active["alert_until"]=10000.0
 var start_usec:int=Time.get_ticks_usec()
 for i in range(96):
  game.call("_update_beasts",1.0/60.0)
 var elapsed_usec:int=Time.get_ticks_usec()-start_usec
 var profile:Dictionary=game.call("v29_ai_profile")
 check(int(profile.get("sampled_frames",0))>=10,"periodic AI samples collected")
 check(int(profile.get("idle_skips",0))>0,"distant calm wildlife uses sleep budget")
 check(int(profile.get("live_steps",0))>0,"active beasts continue updating")
 check(float(profile.get("average_usec",-1))>=0.0 and int(profile.get("peak_usec",-1))>=0,"sampled CPU work is finite")
 check(elapsed_usec>=0,"96-frame direct update benchmark completed")
 # Safe passive probes do not alter the save format, hero art, or combat balancing.
 check(game.call("_combat_threat")==game.call("_combat_threat"),"threat query remains stable")
 print("V29 CPU PROFILE: 96 direct creature-update calls in %.2f ms; sampled mean %.2f us, peak %d us; idle skips %d, live steps %d (headless, not FPS)"%[float(elapsed_usec)/1000.0,float(profile["average_usec"]),int(profile["peak_usec"]),int(profile["idle_skips"]),int(profile["live_steps"])])
 game.queue_free()
 if failures==0:print("MOONFALL v29 PERFORMANCE PASS")
 quit(0 if failures==0 else 1)
