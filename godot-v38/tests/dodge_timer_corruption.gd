extends SceneTree
## Verify real deterministic dodge countdowns and reject corrupted timers.
const COMBAT_RULES=preload("res://scripts/systems/combat_rules.gd")
var failures:int=0

func check(ok:bool,description:String)->void:
 if ok:print("PASS dodge timer: "+description)
 else:
  failures+=1
  push_error("FAIL dodge timer: "+description)

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 check(is_equal_approx(COMBAT_RULES.dodge_timer_after_frame(0.5,0.1),0.4),"ordinary 0.5-second dodge decrements")
 check(is_equal_approx(COMBAT_RULES.dodge_timer_after_frame(0.55,0.05),0.5),"upper legal dodge window decrements")
 check(is_zero_approx(COMBAT_RULES.dodge_timer_after_frame(0.1,0.25)),"frame hitch ends dodge instead of extending it")
 check(is_zero_approx(COMBAT_RULES.dodge_timer_after_frame(NAN,0.016)),"NaN dodge clocks never grant immunity")
 check(is_zero_approx(COMBAT_RULES.dodge_timer_after_frame(INF,0.016)),"infinite dodge clocks never grant immunity")
 check(is_zero_approx(COMBAT_RULES.dodge_timer_after_frame(-INF,0.016)),"negative infinity cannot grant immunity")
 check(is_zero_approx(COMBAT_RULES.dodge_timer_after_frame(0.551,0.016)),"above maximum dodge duration rejected")
 check(is_zero_approx(COMBAT_RULES.dodge_timer_after_frame(-0.1,0.016)),"negative dodge timer rejected")
 check(is_equal_approx(COMBAT_RULES.dodge_timer_after_frame(0.5,NAN),0.5),"invalid frame delta cannot extend dodge")
 check(is_equal_approx(COMBAT_RULES.dodge_timer_after_frame(0.5,INF),0.5),"infinite frame delta cannot extend dodge")
 check(is_equal_approx(COMBAT_RULES.dodge_timer_after_frame(0.5,-0.5),0.5),"negative frame delta cannot rewind dodge")
 check(is_equal_approx(COMBAT_RULES.dodge_timer_after_frame(0.5,0.0),0.5),"zero frame delta freezes timer")
 check(is_zero_approx(COMBAT_RULES.dodge_timer_after_frame(NAN,NAN)),"invalid dodge state rejected even during invalid frames")
 var all_correct:bool=true
 for i in range(11111):
  var current:float=float(i%57)*0.01
  var delta:float=float(i%13)*0.013
  if i%19==0:current=NAN
  elif i%19==1:current=INF
  elif i%19==2:current=-INF
  elif i%19==3:current=-0.2
  elif i%19==4:current=0.56
  if i%17==0:delta=NAN
  elif i%17==1:delta=INF
  elif i%17==2:delta=-INF
  elif i%17==3:delta=-0.2
  var expected:float=0.0
  if is_finite(current) and current>=0.0 and current<=0.55:
   expected=maxf(0.0,current-delta) if is_finite(delta) and delta>0.0 else current
  var actual:float=COMBAT_RULES.dodge_timer_after_frame(current,delta)
  if not is_finite(actual) or actual<0.0 or actual>0.55 or not is_equal_approx(actual,expected):
   all_correct=false
   break
 check(all_correct,"11,111 deterministic timer cases including corrupt states and frame hitches")
 var live_script:String=FileAccess.get_file_as_string("res://scripts/native_v11.gd")
 check("dodge_time=COMBAT_RULES.dodge_timer_after_frame(dodge_time,delta)" in live_script,
  "live Godot physics loop uses the same guarded dodge clock")
 print("MOONFALL DODGE TIMER "+("PASS" if failures==0 else "FAIL"))
 quit(0 if failures==0 else 1)
