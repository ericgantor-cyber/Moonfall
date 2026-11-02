extends SceneTree
const C = preload("res://scripts/systems/combat_rules.gd")
const F = preload("res://scripts/systems/companion_combat_rules.gd")
var failed:int=0
func ck(ok:bool, title:String)->void:
 if ok:print("PASS next11 runtime: "+title)
 else:
  failed+=1
  push_error("FAIL next11 runtime: "+title)
func _initialize()->void:
 call_deferred("run")
func run()->void:
 ck(is_equal_approx(C.outgoing_damage(10.0,false,false,0,0.0,1.0),10.0),"normal damage unchanged")
 ck(is_equal_approx(C.outgoing_damage(10.0,true,true,2,0.0,1.0),19.2),"critical execute scaling unchanged")
 ck(C.outgoing_damage(NAN,false,false,0,0.0,1.0)==0.0,"invalid base cannot damage")
 ck(C.outgoing_damage(-5.0,false,false,0,0.0,1.0)==0.0,"negative base cannot heal through damage")
 ck(is_finite(C.outgoing_damage(10.0,true,true,2147483647,INF,INF)),"extreme modifiers do not overflow")
 ck(is_equal_approx(C.outgoing_damage(10.0,false,false,0,NAN,NAN),10.0),"invalid buffs recover neutrally")
 ck(C.elite_missed(Vector3(NAN,0,0),Vector3.ZERO,0.0),"invalid strike position misses")
 ck(C.elite_missed(Vector3.ZERO,Vector3(NAN,0,0),0.0),"invalid target position misses")
 ck(C.guardian_attack_cooldown(-50)==3.0,"invalid guardian phase cannot slow attack excessively")
 ck(F.target_priority(NAN,0.0,false,5.0)>=1.0e11,"companion avoids corrupt targets")
 ck(not F.can_engage(true,true,5.0,1.0,-1.0,1.0,10.0,true,2.0),"companion rejects invalid distances")
 ck(not F.can_engage(true,true,5.0,1.0,1.0,1.0,INF,true,2.0),"companion rejects invalid leash")
 ck(F.cooldown_elapsed(NAN,0.016)>0.0,"companion corrupt cooldown recovers safely")
 ck(F.pursuit_step(INF,0.1)==0.0,"companion invalid speed cannot teleport")
 ck(load("res://scenes/main.tscn")!=null,"main game scene loads")
 print("next11 runtime check failures: %d" % failed)
 quit(1 if failed else 0)
