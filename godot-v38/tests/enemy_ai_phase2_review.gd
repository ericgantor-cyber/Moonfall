extends SceneTree
const AI=preload("res://scripts/systems/procedural_variants_v382.gd")
var failures:int=0
func check(ok:bool,label:String)->void:
 if ok:print("PASS enemy AI: "+label)
 else:
  failures+=1
  push_error("FAIL enemy AI: "+label)
func _initialize()->void:call_deferred("run")
func run()->void:
 check(AI.combat_intent("ambusher",2.0,0.15,true,true)=="retreat","low HP ambusher retreats")
 check(AI.combat_intent("bruiser",7.0,1.0,true,true)=="pursue","distant bruiser pursues")
 check(AI.combat_intent("sniper",2.0,1.0,true,true)=="retreat","close sniper retreats")
 check(AI.combat_intent("caster",8.0,1.0,true,false)=="reposition","occluded ranged enemy repositions")
 check(AI.combat_intent("ambusher",2.0,1.0,false,true)=="strafe","ambusher moves during cooldown")
 check(load("res://scenes/main.tscn")!=null,"full Moonfall scene loads")
 quit(1 if failures else 0)
