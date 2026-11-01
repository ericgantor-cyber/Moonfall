extends SceneTree
const R=preload("res://scripts/systems/enemy_combat_rules.gd")
const I=preload("res://scripts/systems/procedural_variants_v382.gd")
var failures:int=0
func ck(ok:bool,msg:String)->void:
 if ok:print("PASS: "+msg)
 else:
  failures+=1
  push_error("FAIL: "+msg)
func _initialize()->void:call_deferred("run")
func run()->void:
 ck(I.combat_intent(R.role_for("wolf"),2.2,1.0,false,true)=="strafe","wolf circles while recovering")
 ck(I.combat_intent(R.role_for("serpent"),1.9,1.0,false,true)=="strafe","serpent circles during melee cooldown")
 ck(I.combat_intent(R.role_for("serpent"),1.0,1.0,true,true)=="attack","serpent enters actual melee range")
 ck(I.combat_intent(R.role_for("wolf"),1.0,0.1,true,true)=="retreat","injured wolf can retreat")
 ck(I.combat_intent(R.role_for("boar"),1.0,1.0,false,true)=="guard","boar holds direct fighting position")
 ck(R.can_attack(1.0,true,true) and not R.can_attack(1.8,true,true),"melee attack range enforced")
 ck(not R.can_attack(1.0,true,false),"no attacking through walls")
 ck(R.orbit_direction(2)==1.0 and R.orbit_direction(3)==-1.0,"deterministic opposing flank sides")
 ck(load("res://scenes/main.tscn")!=null,"full Moonfall scene loads")
 quit(0 if failures==0 else 1)
