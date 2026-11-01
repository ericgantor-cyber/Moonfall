extends SceneTree
const R=preload("res://scripts/systems/enemy_combat_rules.gd")
var failed:int=0
func ck(ok:bool,label:String)->void:
 if ok:print("PASS memory: "+label)
 else:
  failed+=1
  push_error("FAIL memory: "+label)
func _initialize()->void:call_deferred("run")
func run()->void:
 var old:Vector3=Vector3(5,0,0)
 ck(R.remember_target(false,true,false,10.0,Vector3(99,0,0),old,12.0)["position"]==old,"last seen remains fixed")
 ck(R.remember_target(false,true,false,10.0,Vector3.ZERO,old,1000.0)["until"]==0.0,"huge old expiry rejected")
 ck(R.remember_target(false,true,false,10.0,Vector3.ZERO,old,INF)["until"]==0.0,"infinite old expiry rejected")
 ck(R.remember_target(false,true,false,10.0,Vector3.ZERO,old,9.0)["until"]==0.0,"expired memory cleared")
 ck(R.remember_target(false,true,false,10.0,Vector3.ZERO,Vector3(NAN,0,0),12.0)["until"]==0.0,"invalid memory position cleared")
 ck(R.investigate_velocity(true,false,false,10.0,INF,Vector3.ZERO,old,3.0)==Vector3.ZERO,"nonfinite expiry cannot cause chase")
 ck(R.investigate_velocity(true,false,false,10.0,500.0,Vector3.ZERO,old,3.0)==Vector3.ZERO,"unbounded expiry cannot cause chase")
 ck(R.investigate_velocity(true,false,false,10.0,12.0,Vector3.ZERO,Vector3(NAN,0,0),3.0)==Vector3.ZERO,"invalid target cannot cause chase")
 ck(R.investigate_velocity(true,false,false,10.0,12.0,Vector3.ZERO,old,3.0).length()>0.0,"normal search remains functional")
 ck(load("res://scenes/main.tscn")!=null,"whole scene loads")
 quit(1 if failed else 0)
