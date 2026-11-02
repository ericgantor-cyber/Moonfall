extends SceneTree
const R=preload("res://scripts/systems/enemy_combat_rules.gd")
var fails:int=0
func ck(ok:bool,label:String)->void:
 if ok:print("PASS v448: "+label)
 else:
  fails+=1
  push_error("FAIL v448: "+label)
func _initialize()->void:call_deferred("run")
func run()->void:
 var start:Dictionary=R.remember_target(true,true,false,10.0,Vector3(6,0,0),Vector3.ZERO,0.0)
 ck(start["position"]==Vector3(6,0,0),"enemy acquires visible position")
 ck(is_equal_approx(start["until"],13.0),"three-second memory set")
 var lost:Dictionary=R.remember_target(false,true,false,11.0,Vector3(100,0,0),start["position"],start["until"])
 ck(lost["position"]==Vector3(6,0,0),"no tracking through walls")
 var far_velocity:Vector3=R.investigate_velocity(true,false,false,11.0,lost["until"],Vector3.ZERO,lost["position"],10.0)
 ck(far_velocity.x>0.0,"search progresses toward last seen position")
 var near_velocity:Vector3=R.investigate_velocity(true,false,false,11.0,lost["until"],Vector3(4.9,0,0),lost["position"],10.0)
 ck(near_velocity.length()<far_velocity.length(),"search brakes approaching destination")
 ck(R.search_arrived(Vector3(5.5,0,0),lost["position"]),"arrival detected")
 var pause_until:float=R.search_pause_expiry(11.5)
 ck(R.search_pause_active(false,11.6,pause_until),"look-around active")
 ck(not R.search_pause_active(true,11.6,pause_until),"new sighting interrupts search")
 ck(not R.search_pause_active(false,13.0,pause_until),"look-around expires")
 var reacquired:Dictionary=R.remember_target(true,true,false,12.0,Vector3(9,0,0),lost["position"],lost["until"])
 ck(reacquired["position"]==Vector3(9,0,0),"reacquisition moves last seen target")
 ck(R.investigate_velocity(true,false,false,20.0,reacquired["until"],Vector3.ZERO,reacquired["position"],10.0)==Vector3.ZERO,"expired chase stops")
 ck(load("res://scenes/main.tscn")!=null,"complete game scene loads")
 quit(1 if fails else 0)
