extends SceneTree
## Opt-in 11,111,111-case search and pursuit stress sweep.
## Run with: python tools/run_tests.py --pursuit-stress-full
const RULES=preload("res://scripts/systems/enemy_combat_rules.gd")
const DEFAULT_CASES:int=11111
const MAX_CASES:int=11111111

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var count:int=DEFAULT_CASES
 var requested:String=OS.get_environment("MOONFALL_STRESS_CASES")
 if not requested.is_empty():
  if not requested.is_valid_int():
   push_error("MOONFALL_STRESS_CASES must be an integer")
   quit(2)
   return
  count=int(requested)
 if count<1 or count>MAX_CASES:
  push_error("MOONFALL_STRESS_CASES must be between 1 and 11111111")
  quit(2)
  return
 var failure:int=-1
 for sample_index in range(count):
  var scenario:int=sample_index%16
  var body:Vector3=Vector3.ZERO
  var target:Vector3=Vector3(4.0,0.0,0.0)
  var speed:float=6.0
  var now:float=1.0+float(sample_index%97)*0.01
  var until:float=now+2.0
  var hostile:bool=true
  var engaged:bool=false
  var boss:bool=false
  var expected_speed:float=3.9
  var expected_arrival:bool=false
  if scenario==1:
   target.x=0.5
   expected_speed=0.0
   expected_arrival=true
  elif scenario==2:
   target.x=1.5
   expected_speed=1.75
  elif scenario==3:
   hostile=false
   expected_speed=0.0
  elif scenario==4:
   engaged=true
   expected_speed=0.0
  elif scenario==5:
   boss=true
   expected_speed=0.0
  elif scenario==6:
   now=NAN
   expected_speed=0.0
  elif scenario==7:
   until=now
   expected_speed=0.0
  elif scenario==8:
   until=now+3.5
   expected_speed=0.0
  elif scenario==9:
   speed=-2.0
   expected_speed=0.0
  elif scenario==10:
   speed=INF
   expected_speed=0.0
  elif scenario==11:
   body.y=NAN
   expected_speed=0.0
  elif scenario==12:
   target.y=INF
   expected_speed=0.0
  elif scenario==13:
   body=Vector3(1.0e19,0.0,0.0)
   target=Vector3(-1.0e19,0.0,0.0)
   expected_speed=0.0
  elif scenario==14:
   speed=100.0
   target.x=10.0
   expected_speed=11.7
  elif scenario==15:
   target.x=1.05
   expected_speed=0.175
  var velocity:Vector3=RULES.investigate_velocity(hostile,engaged,boss,now,until,body,target,speed)
  var arrived:bool=RULES.search_arrived(body,target)
  if not is_finite(velocity.x) or not is_finite(velocity.y) or not is_finite(velocity.z) or absf(velocity.length()-expected_speed)>0.002 or arrived!=expected_arrival:
   failure=sample_index
   break
  if expected_speed>0.0 and (velocity.x<=0.0 or not is_zero_approx(velocity.y) or not is_zero_approx(velocity.z)):
   failure=sample_index
   break
  var pause_now:float=2.0
  var expiry:float=2.75
  var pause_expected:bool=true
  if scenario==3:
   pause_now=NAN
   pause_expected=false
  elif scenario==4:
   expiry=2.0
   pause_expected=false
  elif scenario==5:
   expiry=2.9
   pause_expected=false
  elif scenario==6:
   expiry=INF
   pause_expected=false
  elif scenario==7:
   pause_now=-1.0
   pause_expected=false
  elif scenario==8:
   pause_now=1.0e12+1.0
   pause_expected=false
  elif scenario==9:
   expiry=1.0
   pause_expected=false
  var pause_active:bool=RULES.search_pause_active(false,pause_now,expiry)
  if pause_active!=pause_expected or RULES.search_pause_active(true,pause_now,expiry):
   failure=sample_index
   break
 if failure>=0:
  push_error("Pursuit stress failed at case "+str(failure)+" of "+str(count))
  quit(1)
  return
 print("MOONFALL PURSUIT STRESS PASS "+str(count)+"/"+str(count))
 quit(0)
