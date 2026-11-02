extends SceneTree
const R=preload("res://scripts/systems/enemy_combat_rules.gd")
var failed:int=0
func ck(ok:bool,label:String)->void:
 if ok:print("PASS next11: "+label)
 else:
  failed+=1
  push_error("FAIL next11: "+label)
func _initialize()->void:call_deferred("run")
func run()->void:
 ck(is_equal_approx(R.attack_cooldown("panther"),1.3),"panther cadence")
 ck(is_equal_approx(R.attack_cooldown("wolf"),1.5),"wolf cadence")
 ck(is_equal_approx(R.attack_cooldown("serpent"),1.6),"serpent cadence")
 ck(is_equal_approx(R.attack_cooldown("boar"),1.9),"boar cadence")
 ck(R.staggered_cooldown("boar",3)>2.4,"boar strike staggering")
 ck(R.staggered_cooldown("panther",3)<1.7,"panther strike staggering")
 ck(is_equal_approx(R.preferred_spacing("panther"),2.9),"panther flank space")
 ck(is_equal_approx(R.preferred_spacing("serpent"),2.0),"serpent flank space")
 ck(is_equal_approx(R.preferred_spacing("wolf"),2.6),"wolf flank space")
 ck(R.wants_flank("panther",8.1,true),"panther extended flank")
 ck(not R.wants_flank("serpent",6.1,true),"serpent short flank")
 ck(R.attack_window_count("boar")==5,"boar coordinated strike window")
 ck(R.should_circle("panther","strafe",6.1),"panther wider orbit")
 ck(not R.should_circle("serpent","strafe",5.1),"serpent closes for melee")
 ck(R.melee_engage_range("boar")>R.melee_engage_range("wolf"),"boar melee reach rule")
 ck(not R.can_attack(1.0,true,false),"walls still block hit")
 ck(load("res://scenes/main.tscn")!=null,"scene loads")
 print("Next11 failures: %d" % failed)
 quit(1 if failed else 0)
