extends SceneTree
## Regression coverage for eleven live enemy encounter balance changes.
const R = preload("res://scripts/systems/enemy_combat_rules.gd")
var failures:int = 0
func ck(ok:bool, label:String)->void:
 if ok:print("PASS encounters: "+label)
 else:
  failures+=1
  push_error("FAIL encounters: "+label)
func _initialize()->void:
 call_deferred("run")
func run()->void:
 ck(is_equal_approx(R.attack_cooldown("panther"),1.3),"panther attack recovery")
 ck(is_equal_approx(R.attack_cooldown("wolf"),1.5),"wolf attack recovery")
 ck(is_equal_approx(R.attack_cooldown("serpent"),1.6),"serpent attack recovery")
 ck(is_equal_approx(R.attack_cooldown("boar"),1.9),"boar attack recovery")
 ck(is_equal_approx(R.preferred_spacing("panther"),2.9),"panther wider flank")
 ck(is_equal_approx(R.preferred_spacing("serpent"),2.0),"serpent tighter flank")
 ck(is_equal_approx(R.preferred_spacing("wolf"),2.6),"wolf flank spacing")
 ck(R.wants_flank("panther",7.5,true) and not R.wants_flank("wolf",7.5,true),"panther longer flank reach")
 ck(R.attack_window_count("serpent")==5 and R.attack_window_count("wolf")==4,"serpent longer pack window")
 ck(R.should_circle("panther","strafe",5.8) and not R.should_circle("wolf","strafe",5.8),"panther longer strafe")
 ck(R.staggered_cooldown("boar",3)>R.attack_cooldown("boar")+0.5,"boar wider stagger")
 ck(not R.can_attack(1.0,true,false) and not R.can_attack(1.8,true,true),"line of sight and range still protected")
 ck(R.role_for("wolf")=="ambusher" and R.role_for("boar")=="bruiser","enemy roles preserved")
 ck(load("res://scenes/main.tscn")!=null,"main scene loads")
 print("encounter regression failures: %d" % failures)
 quit(1 if failures else 0)
