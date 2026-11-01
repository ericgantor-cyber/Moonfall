extends SceneTree
const R=preload("res://scripts/systems/companion_combat_rules.gd")
var fails:int=0
func ck(v:bool,name:String)->void:
 if v:print("PASS hardening: "+name)
 else:fails+=1;push_error("FAIL hardening: "+name)
func _initialize()->void:call_deferred("run")
func run()->void:
 ck(R.can_engage(true,true,11.0,10.0,9.0,16.0,12.0,true,10.0),"alerted enemy eligible")
 ck(not R.can_engage(true,false,11.0,10.0,9.0,16.0,12.0,true,10.0),"passive enemy excluded")
 ck(not R.can_engage(false,true,11.0,10.0,9.0,16.0,12.0,true,10.0),"dead enemy excluded")
 ck(not R.can_engage(true,true,9.0,10.0,9.0,16.0,12.0,true,10.0),"expired alert excluded")
 ck(not R.can_engage(true,true,11.0,10.0,9.0,16.0,12.0,false,10.0),"wall blocks attack")
 ck(not R.can_engage(true,true,11.0,10.0,200.0,16.0,12.0,true,10.0),"leash enforced")
 ck(not R.can_engage(true,true,11.0,10.0,9.0,120.0,12.0,true,10.0),"companion range enforced")
 ck(is_equal_approx(R.cooldown_elapsed(1.0,3.0),0.9),"cooldown clamps stalled frame")
 ck(is_equal_approx(R.pursuit_step(5.0,3.0),0.5),"movement clamps stalled frame")
 var scene:PackedScene=load("res://scenes/main.tscn") as PackedScene
 ck(scene!=null,"full scene loads")
 quit(1 if fails>0 else 0)
