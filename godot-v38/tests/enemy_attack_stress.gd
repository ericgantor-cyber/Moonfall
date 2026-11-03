extends SceneTree
## Opt-in attack scheduling stress gate. Separate from the default regression run.
## Set MOONFALL_STRESS_CASES=11111111 for the complete deterministic sweep.
const RULES=preload("res://scripts/systems/enemy_combat_rules.gd")
const KINDS=["panther","wolf","serpent","boar","unknown"]
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
  var species_index:int=sample_index%5
  var kind:String=KINDS[species_index]
  var slots:int=5 if species_index==2 or species_index==3 else (6 if species_index==4 else 4)
  var bruiser:bool=species_index>=3
  var beast_id:int=sample_index%61-30
  var clock_tick:int=(sample_index*7)%2048
  var now:float=float(clock_tick)*0.25+0.03125
  var scenario:int=sample_index%16
  var valid_clock:bool=true
  if scenario==10:now=NAN
  elif scenario==11:now=INF
  elif scenario==12:now=-INF
  elif scenario==13:now=-1.0
  elif scenario==14:now=1.0e12+1.0
  elif scenario==15:now=1.0e15
  if scenario>=10:valid_clock=false
  var attack_slot:int=posmod(beast_id,slots)
  var selected_slot:int=posmod(clock_tick,slots)
  var expected_ready:bool=valid_clock and (selected_slot==attack_slot or (bruiser and selected_slot==posmod(attack_slot+1,slots)))
  if RULES.attack_window_count(kind)!=slots or RULES.tactical_attack_ready(kind,now,beast_id)!=expected_ready:
   failed_case=sample_index
   break
  var current:float=0.35
  var delta:float=0.016
  var expected_timer:float=0.334
  if scenario==1:
   current=0.0
   expected_timer=0.0
  elif scenario==2:
   current=-0.1
   expected_timer=0.1
  elif scenario==3:
   current=NAN
   expected_timer=0.1
  elif scenario==4:
   current=INF
   expected_timer=0.1
  elif scenario==5:
   current=60.0
   expected_timer=29.984
  elif scenario==6:
   delta=0.5
   expected_timer=0.25
  elif scenario==7:
   delta=-0.1
   expected_timer=0.35
  elif scenario==8:
   delta=NAN
   expected_timer=0.35
  elif scenario==9:
   delta=INF
   expected_timer=0.35
  elif scenario==10:
   current=0.02
   delta=0.5
   expected_timer=0.0
  elif scenario==11:
   current=0.1
   delta=0.1
   expected_timer=0.0
  elif scenario==12:
   current=0.001
   delta=0.0
   expected_timer=0.001
  elif scenario==13:
   current=-INF
   expected_timer=0.1
  elif scenario==14:
   current=0.5
   delta=-INF
   expected_timer=0.5
  elif scenario==15:
   current=0.2
   delta=1.0e10
   expected_timer=0.1
  var timer_after:float=RULES.attack_timer_after_frame(current,delta)
  if not is_finite(timer_after) or absf(timer_after-expected_timer)>0.00001 or RULES.ready_to_strike(timer_after)!=(expected_timer==0.0):
   failed_case=sample_index
   break
  var distance:float=1.75 if scenario%2==0 else 1.6
  var can_hit:bool=RULES.can_attack(distance,true,true,kind)
  var expected_hit:bool=distance<(1.8 if kind=="boar" else 1.7)
  if can_hit!=expected_hit or RULES.can_attack(distance,false,true,kind) or RULES.can_attack(distance,true,false,kind):
   failed_case=sample_index
   break
 if failed_case>=0:
  push_error("Enemy attack stress failed at sample "+str(failed_case)+" of "+str(count))
  quit(1)
  return
 print("MOONFALL ENEMY ATTACK STRESS PASS "+str(count)+"/"+str(count))
 quit(0)
