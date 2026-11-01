extends SceneTree
const R=preload("res://scripts/systems/enemy_combat_rules.gd")
var errors:int=0
func ck(value:bool,description:String)->void:
 if value:print("PASS search: "+description)
 else:
  errors+=1
  push_error("FAIL search: "+description)
func _initialize()->void:call_deferred("run")
func run()->void:
 ck(R.search_arrived(Vector3.ZERO,Vector3(0.5,0,0.4)),"arrives inside search radius")
 ck(not R.search_arrived(Vector3.ZERO,Vector3(3,0,0)),"far away still investigating")
 ck(R.search_arrived(Vector3(1,5,1),Vector3(1,0,1)),"height difference ignored")
 ck(not R.search_arrived(Vector3.ZERO,Vector3(NAN,0,0)),"invalid position never counts as arrived")
 ck(R.investigate_velocity(true,false,false,2.0,5.0,Vector3.ZERO,Vector3(4,0,0),NAN)==Vector3.ZERO,"invalid movement speed stops")
 ck(R.investigate_velocity(true,false,false,2.0,5.0,Vector3.ZERO,Vector3(4,0,0),1000.0).length()<=13.01,"investigation speed capped")
 ck(R.investigate_velocity(true,false,false,6.0,5.0,Vector3.ZERO,Vector3(4,0,0),5.0)==Vector3.ZERO,"expired search stops")
 ck(load("res://scenes/main.tscn")!=null,"main scene loads")
 quit(1 if errors else 0)
