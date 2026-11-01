extends SceneTree
const R=preload("res://scripts/systems/guardian_attack_rules.gd")
var failures:int=0
func check(ok:bool,label:String)->void:
 if ok:print("PASS guardian safety: "+label)
 else:failures+=1;push_error("FAIL guardian safety: "+label)
func _initialize()->void:call_deferred("run")
func run()->void:
 check(R.safe_phase(-20)==1 and R.safe_phase(999)==3,"old save phase bounded")
 check(is_equal_approx(R.safe_radius(-50.0),0.5) and is_equal_approx(R.safe_radius(900.0),4.0),"attack radius bounded")
 check(is_equal_approx(R.safe_radius(NAN),2.8) and is_equal_approx(R.safe_radius(INF),2.8),"nonfinite radius safely reset")
 check(R.warning_hit(Vector3.ZERO,Vector3(1,0,0),2.0),"warning inside hits")
 check(not R.warning_hit(Vector3.ZERO,Vector3(4,0,0),2.0),"warning outside misses")
 check(R.boss_damage(-5)==26.0 and R.boss_damage(900)==40.0,"boss phase damage bounded")
 var scene:PackedScene=load("res://scenes/main.tscn") as PackedScene
 check(scene!=null,"active game scene loads")
 quit(1 if failures>0 else 0)
