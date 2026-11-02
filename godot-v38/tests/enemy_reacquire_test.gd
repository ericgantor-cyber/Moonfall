extends SceneTree
const R=preload("res://scripts/systems/enemy_combat_rules.gd")
var fails:int=0
func ck(ok:bool,name:String)->void:
 if ok:print("PASS reacquire: "+name)
 else:
  fails+=1
  push_error("FAIL reacquire: "+name)
func _initialize()->void:call_deferred("run")
func run()->void:
 var first:Dictionary=R.remember_target(true,true,false,5.0,Vector3(4,0,0),Vector3.ZERO,0.0)
 ck(first["position"]==Vector3(4,0,0),"initial sight acquired")
 var lost:Dictionary=R.remember_target(false,true,false,6.0,Vector3(100,0,0),first["position"],first["until"])
 ck(lost["position"]==Vector3(4,0,0),"no tracking through wall")
 var seen_again:Dictionary=R.remember_target(true,true,false,7.0,Vector3(8,0,0),lost["position"],lost["until"])
 ck(seen_again["position"]==Vector3(8,0,0),"reacquired visible location")
 ck(is_equal_approx(seen_again["until"],10.0),"reacquisition refreshes three seconds")
 ck(R.investigate_velocity(true,true,false,7.0,10.0,Vector3.ZERO,Vector3(8,0,0),4.0)==Vector3.ZERO,"direct engagement supersedes searching")
 ck(R.investigate_velocity(true,false,false,8.0,10.0,Vector3.ZERO,Vector3(8,0,0),4.0).length()>0.0,"second search uses new location")
 ck(load("res://scenes/main.tscn")!=null,"main scene still loads")
 quit(1 if fails else 0)
