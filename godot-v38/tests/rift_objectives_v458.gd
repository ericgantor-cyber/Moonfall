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
 var exploring:Dictionary={"active":true,"stage":3,"cleared":false}
 ck(O.chamber_prompt(exploring,1)==O.objective(1,false),"first chamber available despite legacy stage three")
 ck(O.chamber_prompt(exploring,2)==O.objective(2,false),"second chamber available despite legacy stage three")
 ck(O.chamber_prompt(exploring,3)==O.objective(3,false),"guardian chamber available alongside other rooms")
 ck(O.chamber_prompt({"active":true,"stage":1,"cleared":false},2)==O.objective(2,false),"legacy stage one cannot hide second chamber")
 ck(O.chamber_prompt({"active":false,"stage":3,"cleared":false},1).is_empty(),"inactive rift does not show active objective")
 ck(O.chamber_prompt({"active":true,"stage":3,"cleared":true},2).is_empty(),"sealed rift keeps inherited sealed label")
 ck(O.chamber_prompt(exploring,0).is_empty() and O.chamber_prompt(exploring,4).is_empty(),"invalid chamber indices ignored")
 ck(load("res://scenes/main.tscn")!=null,"live game scene loads")
 if failures==0:print("MOONFALL RIFT OBJECTIVES PASS")
 quit(1 if failures else 0)
