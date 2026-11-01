extends SceneTree
const R=preload("res://scripts/systems/companion_combat_rules.gd")
var failures:int=0
func check(ok:bool, label:String)->void:
 if ok:print("PASS Phase 2: "+label)
 else:
  failures+=1
  push_error("FAIL Phase 2: "+label)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 check(R.target_priority(4.0,36.0,true,5.5)<R.target_priority(49.0,1.0,true,5.5),"guard prioritizes threat near low-health hero")
 check(R.target_priority(49.0,1.0,false,5.5)<R.target_priority(4.0,36.0,false,5.5),"healthy companion prefers nearest threat")
 check(R.strike_damage(1,0,false)>0.0,"minimum damage stays positive")
 check(R.strike_damage(500,500,false)<=38.0,"damage and loyalty scaling bounded")
 check(R.strike_damage(15,75,true)<R.strike_damage(15,75,false),"boss hit reduced relative to normal enemy")
 var script:GDScript=load("res://scripts/native_v38_5.gd") as GDScript
 check(script!=null and script.reload()==OK,"companion script compiles")
 var scene:PackedScene=load("res://scenes/main.tscn") as PackedScene
 check(scene!=null,"whole Moonfall scene loads")
 quit(1 if failures>0 else 0)
