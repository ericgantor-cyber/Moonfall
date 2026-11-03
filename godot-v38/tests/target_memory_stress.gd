extends SceneTree
## Opt-in target-memory and search-pause regression stress suite.
## Full run: python tools/run_tests.py --memory-stress-full
const RULES=preload("res://scripts/systems/enemy_combat_rules.gd")
const DEFAULT_CASES:int=11111
const MAX_CASES:int=11111111

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var count:int=DEFAULT_CASES
 var raw:String=OS.get_environment("MOONFALL_STRESS_CASES")
 if not raw.is_empty():
  if not raw.is_valid_int():
   push_error("MOONFALL_STRESS_CASES must be an integer")
   quit(2)
   return
  count=int(raw)
 if count<1 or count>MAX_CASES:
  push_error("MOONFALL_STRESS_CASES must be between 1 and 11111111")
  quit(2)
  return
 var failed_case:int=-1
 for sample_index in range(count):
  var scenario:int=sample_index%16
  var now:float=100.0+float(sample_index%101)*0.01
  var hero:Vector3=Vector3(8.0,0.0,2.0)
  var old:Vector3=Vector3(4.0,0.0,3.0)
  var old_until:float=now+2.0
  var seen:bool=false
  var hostile:bool=true
  var boss:bool=false
  var expected_position:Vector3=old
  var expected_until:float=old_until
  if scenario==0 or scenario==14:
   seen=true
   expected_position=hero
   expected_until=now+3.0
  elif scenario==2:
   old_until=now
   expected_until=0.0
  elif scenario==3:
   old_until=now+3.5
   expected_until=0.0
  elif scenario==4:
   seen=true
   hero.x=NAN
   expected_position=Vector3.ZERO
   expected_until=0.0
  elif scenario==5:
   seen=true
   hero.y=INF
   expected_position=Vector3.ZERO
   expected_until=0.0
  elif scenario==6:
   old.x=NAN
   expected_position=Vector3.ZERO
   expected_until=0.0
  elif scenario==7:
   hostile=false
   expected_position=Vector3.ZERO
   expected_until=0.0
  elif scenario==8:
   boss=true
   expected_position=Vector3.ZERO
   expected_until=0.0
  elif scenario==9:
   now=NAN
   expected_position=Vector3.ZERO
   expected_until=0.0
  elif scenario==10:
   now=-1.0
   expected_position=Vector3.ZERO
   expected_until=0.0
  elif scenario==11:
   now=1.0e12+1.0
   expected_position=Vector3.ZERO
   expected_until=0.0
  elif scenario==12:
   now=1.0e12-2.0
   seen=true
   expected_position=Vector3.ZERO
   expected_until=0.0
  elif scenario==13:
   old_until=INF
   expected_until=0.0
  elif scenario==15:
   old.y=INF
   expected_position=Vector3.ZERO
   expected_until=0.0
  if scenario==14:
   hero.y=120.0
   expected_position=hero
  var result:Dictionary=RULES.remember_target(seen,hostile,boss,now,hero,old,old_until)
  var actual_pos:Vector3=result.get("position",Vector3.INF)
  var actual_until:float=float(result.get("until",-1.0))
  if actual_pos!=expected_position or not is_finite(actual_until) or absf(actual_until-expected_until)>0.00001:
   failed_case=sample_index
   break
  var pause_now:float=100.0+float(sample_index%101)*0.01
  if scenario==9:pause_now=NAN
  elif scenario==10:pause_now=-1.0
  elif scenario==11:pause_now=1.0e12+1.0
  elif scenario==12:pause_now=1.0e12-0.5
  var should_expire:bool=scenario!=9 and scenario!=10 and scenario!=11 and scenario!=12
  var pause_until:float=RULES.search_pause_expiry(pause_now)
  var expected_pause_until:float=pause_now+0.75 if should_expire else 0.0
  if not is_finite(pause_until) or absf(pause_until-expected_pause_until)>0.00001:
   failed_case=sample_index
   break
  if RULES.search_pause_active(false,pause_now,pause_until)!=should_expire or RULES.search_pause_active(true,pause_now,pause_until):
   failed_case=sample_index
   break
  if should_expire and RULES.search_pause_active(false,pause_until,pause_until):
   failed_case=sample_index
   break
 if failed_case>=0:
  push_error("Memory stress failed at sample "+str(failed_case)+" of "+str(count))
  quit(1)
  return
 print("MOONFALL MEMORY STRESS PASS "+str(count)+"/"+str(count))
 quit(0)
