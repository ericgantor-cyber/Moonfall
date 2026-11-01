extends SceneTree
const R=preload("res://scripts/systems/enemy_combat_rules.gd")
var failed:int=0
func ck(ok:bool,label:String)->void:
 if ok:print("PASS braking: "+label)
 else:
  failed+=1
  push_error("FAIL braking: "+label)
func _initialize()->void:call_deferred("run")
func run()->void:
 var fast:Vector3=R.investigate_velocity(true,false,false,1.0,4.0,Vector3.ZERO,Vector3(10,0,0),10.0)
 var slow:Vector3=R.investigate_velocity(true,false,false,1.0,4.0,Vector3.ZERO,Vector3(1.1,0,0),10.0)
 ck(fast.length()>slow.length(),"slow near remembered position")
 ck(slow.length()>0.0,"continue approaching outside arrival radius")
 ck(R.investigate_velocity(true,false,false,1.0,4.0,Vector3.ZERO,Vector3(0.8,0,0),10.0)==Vector3.ZERO,"stop within arrival radius")
 ck(R.investigate_velocity(true,false,false,5.0,4.0,Vector3.ZERO,Vector3(10,0,0),10.0)==Vector3.ZERO,"expired investigation stops")
 ck(R.investigate_velocity(true,false,false,1.0,4.0,Vector3.ZERO,Vector3(10,0,0),INF)==Vector3.ZERO,"invalid speed stops")
 var memory:Dictionary=R.remember_target(true,true,false,2.0,Vector3(NAN,0,0),Vector3(3,0,0),4.0)
 ck(memory["position"]==Vector3(3,0,0),"invalid hero sighting does not poison memory")
 ck(R.search_arrived(Vector3.ZERO,Vector3(0.5,0,0.5)),"arrival rule remains consistent")
 ck(load("res://scenes/main.tscn")!=null,"whole scene loads")
 quit(1 if failed else 0)
