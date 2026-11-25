extends SceneTree
const GEAR=preload("res://scripts/systems/equipment_inventory.gd")
var failures:int=0

func check(condition:bool,description:String)->void:
 if condition:
  print("PASS material capacity: "+description)
 else:
  failures+=1
  push_error("FAIL material capacity: "+description)

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var state:Dictionary=GEAR.new_state()
 check(not GEAR.add_material(state,"Ironbark",0),"zero grant rejected")
 check(not GEAR.add_material(state,"Ironbark",-1),"negative grant rejected")
 check(GEAR.add_material(state,"Ironbark",999998),"grant up to cap")
 check(not GEAR.add_material(state,"Ironbark",2),"overflow rejected")
 check(int(state["materials"]["Ironbark"])==999998,"overflow leaves material balance intact")
 check(GEAR.add_material(state,"Ironbark",1),"final unit accepted")
 check(not GEAR.add_material(state,"Ironbark",1),"full stack rejects further rewards")
 check(int(state["materials"]["Ironbark"])==999999,"full stack never exceeds cap")
 check(not GEAR.add_material(state,"",1),"empty material identifier rejected")
 check(GEAR.add_material(state,"Moonsteel",4),"other materials unaffected")
 var restored:Dictionary=GEAR.normalize(state.duplicate(true))
 check(restored["materials"]==state["materials"],"save normalization preserves valid material balances")
 print("MATERIAL CAPACITY "+("PASS" if failures==0 else "FAIL"))
 quit(0 if failures==0 else 1)
