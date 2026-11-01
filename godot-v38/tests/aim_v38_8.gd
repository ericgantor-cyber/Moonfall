extends SceneTree
## Headless aiming smoke checks. Run with: godot --headless --path godot-v38 --script res://tests/aim_v38_8.gd
var failures:int=0
func check(value:bool,message:String)->void:
 if value:print("PASS aim: "+message)
 else:
  failures+=1
  push_error("FAIL aim: "+message)

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var base:GDScript=load("res://scripts/native_v38_7.gd") as GDScript
 var ui:GDScript=load("res://scripts/native_v38_8.gd") as GDScript
 check(base!=null and ui!=null,"aim scripts parse and load")
 if base!=null:
  var source:String=base.source_code
  check("JOY_AXIS_RIGHT_X" in source and "JOY_AXIS_RIGHT_Y" in source,"controller right stick wired")
  check("project_ray_origin" in source and "project_ray_normal" in source,"mouse targeting uses camera ray")
  check("aim_locked_id" in source and "_toggle_aim_lock" in source,"target lock toggle exists")
  check("if which!=\"fire\":" in source and "super._spell(which)" in source,"other spells retain inherited behavior")
 if ui!=null:
  var source:String=ui.source_code
  check("AIM_PREFS_PATH" in source and "config.save(AIM_PREFS_PATH)" in source,"aim assist setting persisted")
  check("aim_input_source==\"mouse\"" in source,"mouse aiming restored with connected controller")
  check("beasts.find(target)==aim_locked_id" in source,"locked marker matches actual target")
  check("aim_options.visible=hero_playing and not menu_open and not death_active" in source,"aim controls hidden outside gameplay")
 quit(1 if failures>0 else 0)
