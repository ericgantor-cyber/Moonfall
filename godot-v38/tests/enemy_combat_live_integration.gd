extends SceneTree
const R=preload("res://scripts/systems/enemy_combat_rules.gd")
const I=preload("res://scripts/systems/procedural_variants_v382.gd")
var failed:int=0
func ck(ok:bool,label:String)->void:
 if ok:print("PASS enemy combat: "+label)
 else:
  failed+=1
  push_error("FAIL enemy combat: "+label)
func _initialize()->void:call_deferred("run")
func run()->void:
 ck(R.role_for("wolf")=="ambusher" and R.role_for("panther")=="ambusher","wolf and panther use melee ambush roles")
 ck(R.role_for("boar")=="bruiser" and R.role_for("serpent")=="ambusher","boar bruiser and serpent melee ambusher roles")
 ck(I.combat_intent(R.role_for("wolf"),2.0,0.1,true,true)=="retreat","injured wolf withdraws")
 ck(R.should_circle("wolf","strafe",3.0) and R.should_circle("serpent","strafe",3.0),"wolf and serpent circle during cooldown")
 ck(not R.should_circle("boar","strafe",3.0),"boar does not orbit")
 ck(R.attack_cooldown("panther")<R.attack_cooldown("boar"),"fast predator recovers earlier than boar")
 ck(is_equal_approx(R.attack_timer_after_frame(1.0,2.0),0.9),"stall-safe cooldown step")
 ck(R.attack_timer_after_frame(0.0,0.016)==0.0,"cooldown cannot turn negative")
 ck(load("res://scenes/main.tscn")!=null,"whole Moonfall scene loads")
 quit(1 if failed else 0)
