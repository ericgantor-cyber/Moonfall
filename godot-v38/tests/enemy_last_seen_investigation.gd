extends SceneTree
const R=preload("res://scripts/systems/enemy_combat_rules.gd")
var fails:int=0
func ck(v:bool,name:String)->void:
 if v:print("PASS investigation: "+name)
 else:
  fails+=1
  push_error("FAIL investigation: "+name)
func _initialize()->void:call_deferred("run")
func run()->void:
 var seen:Dictionary=R.remember_target(true,true,false,5.0,Vector3(8,0,0),Vector3.ZERO,0.0)
 ck(seen["position"]==Vector3(8,0,0),"remember visible hero")
 ck(is_equal_approx(float(seen["until"]),8.0),"memory expires after three seconds")
 var lost:Dictionary=R.remember_target(false,true,false,6.0,Vector3(100,0,0),seen["position"],seen["until"])
 ck(lost["position"]==Vector3(8,0,0),"lost hero does not reveal current location")
 ck(R.investigate_velocity(true,false,false,6.0,8.0,Vector3.ZERO,Vector3(8,0,0),3.0).x>0.0,"enemy searches last seen place")
 ck(R.investigate_velocity(true,false,false,9.0,8.0,Vector3.ZERO,Vector3(8,0,0),3.0)==Vector3.ZERO,"investigation expires")
 ck(R.investigate_velocity(false,false,false,6.0,8.0,Vector3.ZERO,Vector3(8,0,0),3.0)==Vector3.ZERO,"passive animals unaffected")
 ck(R.investigate_velocity(true,false,true,6.0,8.0,Vector3.ZERO,Vector3(8,0,0),3.0)==Vector3.ZERO,"boss AI unaffected")
 ck(R.investigate_velocity(true,false,false,6.0,8.0,Vector3(8,0,0),Vector3(8,0,0),3.0)==Vector3.ZERO,"search stops on arrival")
 ck(load("res://scenes/main.tscn")!=null,"full scene loads")
 quit(1 if fails else 0)
