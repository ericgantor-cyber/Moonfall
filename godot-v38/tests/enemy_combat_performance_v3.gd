extends SceneTree
const R=preload("res://scripts/systems/enemy_combat_rules.gd")
var fails:int=0
func ck(ok:bool,label:String)->void:
 if ok:print("PASS combat v3: "+label)
 else:
  fails+=1
  push_error("FAIL combat v3: "+label)
func _initialize()->void:call_deferred("run")
func run()->void:
 ck(R.can_attack(1.0,true,true),"ready adjacent attack eligible")
 ck(not R.can_attack(1.0,false,true),"cooldown rejects attack")
 ck(not R.can_attack(1.0,true,false),"wall rejects attack")
 ck(not R.can_attack(9.0,true,true),"distant enemy rejects attack")
 ck(R.attack_timer_after_frame(1.0,NAN)==1.0,"NaN frame does not corrupt timer")
 ck(R.attack_timer_after_frame(1.0,INF)==1.0,"infinite frame does not corrupt timer")
 ck(R.attack_timer_after_frame(NAN,0.016)==0.0,"NaN saved cooldown resets")
 var source:String=FileAccess.get_file_as_string("res://scripts/native_v11.gd")
 ck("can_attack(distance,float(beast[\"attack\"])<=0.0,true) and _has_clear_sight(body,player)" in source,"melee range and readiness checked before costly sight ray")
 ck(load("res://scenes/main.tscn")!=null,"full scene loads")
 quit(1 if fails else 0)
