extends SceneTree
const R=preload("res://scripts/systems/boss_telegraph_readout.gd")
var fails:int=0
func ck(ok:bool,name:String)->void:
 if ok:print("PASS boss: "+name)
 else:
  fails+=1
  push_error("FAIL boss: "+name)
func _initialize()->void:call_deferred("run")
func run()->void:
 ck(R.pattern_name(0)=="FOCUSED STRIKE","focused warning")
 ck(R.pattern_name(1)=="WIDE SWEEP","wide warning")
 ck(R.pattern_name(2)=="PRECISION STRIKE","precision warning")
 ck(R.pattern_name(3)=="DELAYED STRIKE","delayed warning")
 ck("P3" in R.warning_text(3,3),"phase label")
 var scene:PackedScene=load("res://scenes/main.tscn") as PackedScene
 ck(scene!=null,"full scene loads")
 quit(1 if fails>0 else 0)
