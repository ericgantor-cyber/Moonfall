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
 var beasts:Array=[{"alive":true,"hp":10.0},{"alive":false,"hp":0.0},{"alive":true,"hp":5.0}]
 ck(O.remaining_enemies([0,1,2],beasts)==2,"counts living enemies")
 ck(O.remaining_enemies([1],beasts)==0,"ignores defeated enemies")
 ck(O.remaining_enemies([-1,99],beasts)==0,"ignores missing slots")
 ck(O.remaining_enemies([],beasts)==0,"empty wave")
 ck(O.objective(1,2,false).contains("2 REMAINING"),"first room progress")
 ck(O.objective(2,1,false).contains("CHAMBER 2"),"second room objective")
 ck(O.objective(3,1,false).contains("RIFT CHAMPION"),"final champion objective")
 ck(O.objective(3,0,false).contains("COMPLETE THE RIFT"),"last enemy defeated prompt")
 ck(O.objective(1,0,false).contains("PROCEED"),"empty chamber advances")
 ck(O.objective(3,0,true).contains("REWARD CLAIMED"),"existing clear reward status")
 ck(O.stage_active({"active":true,"stage":2,"cleared":false},2),"active matching stage")
 ck(not O.stage_active({"active":true,"stage":1,"cleared":false},2),"inactive stage hidden")
 ck(not O.stage_active({"active":true,"stage":2,"cleared":true},2),"cleared stage hidden")
 ck(load("res://scenes/main.tscn")!=null,"live game scene loads")
 print("rift objective failures: %d" % failures)
 quit(1 if failures else 0)
