extends SceneTree
const AI_BUDGET:=preload("res://scripts/systems/ai_budget.gd")
var failures:=0
func check(ok:bool,title:String)->void:
 if ok:print("PASS v28: "+title)
 else:
  failures+=1
  push_error("FAIL v28: "+title)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var idle:=AI_BUDGET.may_throttle(30.0*30.0,0.0,0.0,true,false,-1,false,false,false,false,false,false,false)
 check(idle,"distant settled ordinary wildlife may sleep")
 check(not AI_BUDGET.may_throttle(10.0,0.0,0.0,true,false,-1,false,false,false,false,false,false,false),"nearby threats never throttled")
 check(not AI_BUDGET.may_throttle(900.0,0.0,0.0,true,true,-1,false,false,false,false,false,false,false),"boss never throttled")
 check(not AI_BUDGET.may_throttle(900.0,0.0,0.0,true,false,1,false,false,false,false,false,false,false),"rift combat never throttled")
 check(not AI_BUDGET.may_throttle(900.0,0.0,0.0,true,false,-1,true,false,false,false,false,false,false),"engaged beast never throttled")
 check(not AI_BUDGET.may_throttle(900.0,0.0,0.0,true,false,-1,false,true,false,false,false,false,false),"alerted beast never throttled")
 check(not AI_BUDGET.may_throttle(900.0,0.0,0.0,true,false,-1,false,false,false,true,false,false,false),"telegraphed attacks never throttled")
 check(not AI_BUDGET.may_throttle(900.0,0.0,0.0,true,false,-1,false,false,false,false,true,false,false),"ecology pursuit never throttled")
 check(not AI_BUDGET.may_throttle(900.0,0.0,0.0,true,false,-1,false,false,false,false,false,true,false),"fleeing wildlife never throttled")
 check(not AI_BUDGET.may_throttle(900.0,1.0,0.0,true,false,-1,false,false,false,false,false,false,false),"moving wildlife never throttled")
 check(not AI_BUDGET.may_throttle(900.0,0.0,5.0,true,false,-1,false,false,false,false,false,false,false),"returning home never throttled")
 for slot in range(13):
  var count:=0
  for frame in range(1,41):
   if AI_BUDGET.should_tick(frame,slot):count+=1
  check(count==10,"slot %d gets exactly one of four ticks"%slot)
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(str(game.call("_release_label")).begins_with("v3"),"v28 release label")
 game.call("_start_game","z")
 var beast:Dictionary={}
 for candidate in game.get("zone_beast_lists")[0]:
  if str(candidate.get("kind"))=="stag":beast=candidate;break
 check(not beast.is_empty(),"has a passive stag for real-world scheduling")
 if not beast.is_empty():
  var body:CharacterBody3D=beast["body"]
  body.global_position=Vector3(-87.0,0.62,-56.0)
  body.velocity=Vector3.ZERO
  beast["home"]=body.global_position
  beast["engaged"]=false
  beast["scared_until"]=0.0
  beast["ecology_target"]={}
  var start_skips:int=game.get("v28_idle_skips")
  for i in range(16):
   game.call("_update_beasts",1.0/60.0)
  check(game.get("v28_idle_skips")>start_skips,"actual world loop sleeps distant idle wildlife")
  check(float(beast.get("v28_sleep_dt",-1.0))>=0.0,"sleep duration stays bounded")
 check(game.get("v28_ai_live_steps")>0,"nearby active AI continues updating")
 game.queue_free()
 if failures==0:print("MOONFALL v28 AI SCHEDULER PASS")
 quit(0 if failures==0 else 1)
