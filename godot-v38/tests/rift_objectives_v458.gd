extends SceneTree
const O=preload("res://scripts/systems/rift_objective_rules.gd")
var failures:int=0
func ck(ok:bool,label:String)->void:
 if ok:print("PASS rift objectives: "+label)
 else:
  failures+=1
  push_error("FAIL rift objectives: "+label)
func _initialize()->void:call_deferred("run")
func run()->void:
 ck(O.objective(1,false).contains("EXPLORE"),"first room exploration")
 ck(O.objective(2,false).contains("FIND THE WAY"),"second room exploration")
 ck(O.objective(3,false).contains("RIFT CHAMPION"),"final boss objective")
 ck(O.objective(3,true).contains("REWARD UNLOCKED"),"cleared reward status")
 ck(not O.objective(1,false).contains("REMAINING"),"no enemy counter")
 ck(not O.objective(2,false).contains("WAVE"),"no wave objective")
 ck(O.stage_active({"active":true,"stage":2,"cleared":false},2),"active matching stage")
 ck(not O.stage_active({"active":true,"stage":1,"cleared":false},2),"inactive stage hidden")
 ck(not O.stage_active({"active":true,"stage":2,"cleared":true},2),"cleared stage hidden")
 ck(load("res://scenes/main.tscn")!=null,"live game scene loads")
 quit(1 if failures else 0)
