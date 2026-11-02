extends SceneTree
const R=preload("res://scripts/systems/enemy_combat_rules.gd")
var fails:int=0
func check(ok:bool,label:String)->void:
 if ok:print("PASS pause: "+label)
 else:
  fails+=1
  push_error("FAIL pause: "+label)
func _initialize()->void:call_deferred("run")
func run()->void:
 var expiry:float=R.search_pause_expiry(10.0)
 check(is_equal_approx(expiry,10.75),"arrival schedules short pause")
 check(R.search_pause_active(false,10.1,expiry),"pause holds nearby enemy briefly")
 check(not R.search_pause_active(false,11.0,expiry),"pause expires")
 check(not R.search_pause_active(true,10.1,expiry),"re-engagement interrupts pause")
 check(not R.search_pause_active(false,10.1,INF),"invalid saved pause rejected")
 check(not R.search_pause_active(false,10.1,100000.0),"excessively long saved pause rejected")
 check(R.search_pause_expiry(NAN)==0.0,"nonfinite timestamp rejected")
 check(load("res://scenes/main.tscn")!=null,"game scene loads")
 quit(1 if fails else 0)
