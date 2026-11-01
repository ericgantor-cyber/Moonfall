extends SceneTree
const R=preload("res://scripts/systems/enemy_combat_rules.gd")
var failures:int=0
func ck(ok:bool,name:String)->void:
 if ok:print("PASS combat safety: "+name)
 else:
  failures+=1
  push_error("FAIL combat safety: "+name)
func _initialize()->void:call_deferred("run")
func run()->void:
 ck(R.can_attack(1.0,true,true),"valid melee strike")
 ck(not R.can_attack(1.0,false,true),"cooldown blocks strike")
 ck(not R.can_attack(1.0,true,false),"wall blocks strike")
 ck(not R.can_attack(2.0,true,true),"out of range blocks strike")
 ck(not R.can_attack(NAN,true,true),"nonfinite range rejected")
 ck(not R.can_attack(INF,true,true),"infinite range rejected")
 ck(R.should_circle("wolf","strafe",3.0),"wolf circles only in range")
 ck(load("res://scenes/main.tscn")!=null,"whole scene loads")
 quit(1 if failures else 0)
