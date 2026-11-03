extends SceneTree
## Opt-in, deterministic combat stress gate. Never part of default quick tests.
## MOONFALL_STRESS_CASES=11111111 enables the complete requested sweep.
const COMBAT=preload("res://scripts/systems/combat_rules.gd")
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
  var mark:Vector3=Vector3(float(sample_index%97)*0.1,0.0,-2.0)
  var hero:Vector3=mark+Vector3(0.8,0.0,0.0)
  var dodge_remaining:float=0.0
  var clear_sight:bool=true
  if scenario==1:hero=mark+Vector3(2.5,0.0,0.0)
  elif scenario==2 or scenario==3:dodge_remaining=0.25
  elif scenario==4:hero.x=NAN
  elif scenario==5:mark.y=INF
  elif scenario==6:dodge_remaining=-0.1
  elif scenario==7:dodge_remaining=NAN
  elif scenario==8:dodge_remaining=INF
  elif scenario==9:dodge_remaining=-INF
  elif scenario==10:dodge_remaining=1.0
  elif scenario==11:hero=mark+Vector3(1.89,0.0,0.0)
  elif scenario==12:hero=mark+Vector3(1.91,0.0,0.0)
  elif scenario==13:hero.y=100.0
  elif scenario==14:
   mark=Vector3(1.0e19,0.0,0.0)
   hero=Vector3(-1.0e19,0.0,0.0)
  elif scenario==15:
   hero=mark
   dodge_remaining=0.55
  if scenario==3:clear_sight=false
  var expected_reach:bool=scenario==0 or scenario==2 or scenario==3 or scenario==6 or scenario==7 or scenario==8 or scenario==9 or scenario==10 or scenario==11 or scenario==13 or scenario==15
  var expected_miss:bool=not expected_reach or scenario==2 or scenario==3 or scenario==7 or scenario==8 or scenario==9 or scenario==10 or scenario==15
  var expected_credit:bool=scenario==2 or scenario==15
  if COMBAT.elite_in_hit_radius(mark,hero)!=expected_reach or COMBAT.elite_missed(mark,hero,dodge_remaining)!=expected_miss or COMBAT.elite_dodge_credit(mark,hero,dodge_remaining,clear_sight)!=expected_credit:
   failed_case=sample_index
   break
  var timer:float=0.55
  var delta:float=0.016
  var expected_timer:float=0.534
  if scenario==4:
   timer=NAN
   expected_timer=0.0
  elif scenario==5:
   timer=0.7
   expected_timer=0.0
  elif scenario==6:
   timer=-0.1
   expected_timer=0.0
  elif scenario==7:
   delta=-0.1
   expected_timer=0.55
  elif scenario==8:
   delta=NAN
   expected_timer=0.55
  elif scenario==9:
   delta=INF
   expected_timer=0.55
  elif scenario==10:
   delta=1.0
   expected_timer=0.0
  elif scenario==11:
   timer=0.0
   expected_timer=0.0
  elif scenario==12:
   timer=0.001
   expected_timer=0.0
  elif scenario==13:
   timer=0.25
   expected_timer=0.234
  elif scenario==14:
   timer=INF
   expected_timer=0.0
  elif scenario==15:
   timer=0.5
   delta=0.0
   expected_timer=0.5
  var actual_timer:float=COMBAT.dodge_timer_after_frame(timer,delta)
  if not is_finite(actual_timer) or absf(actual_timer-expected_timer)>0.00001:
   failed_case=sample_index
   break
 if failed_case>=0:
  push_error("Combat stress failed at sample "+str(failed_case)+" of "+str(count))
  quit(1)
  return
 print("MOONFALL COMBAT STRESS PASS "+str(count)+"/"+str(count))
 quit(0)
