extends SceneTree
const RULES:=preload("res://scripts/systems/graphics_rules.gd")
var failures:int=0
func _initialize()->void:call_deferred("_run")
func check(ok:bool,message:String)->void:
 if not ok:
  failures+=1
  push_error("4K display regression: "+message)
func _run()->void:
 check("3840x2160" in RULES.OPTIONS["resolution"],"Missing 4K resolution")
 check(RULES.normalize({"resolution":"3840x2160"})["resolution"]=="3840x2160","4K setting was discarded")
 check(RULES.normalize({"resolution":"bogus"})["resolution"]==RULES.DEFAULTS["resolution"],"Invalid resolution accepted")
 check(RULES.DEFAULTS["resolution"]=="1280x800","Low-resolution default unexpectedly changed")
 check(str(ProjectSettings.get_setting("display/window/stretch/mode"))=="canvas_items","HUD scaling mode changed")
 check(str(ProjectSettings.get_setting("display/window/stretch/aspect"))=="expand","Widescreen expansion missing")
 check(RULES.normalize({"resolution":"3840x2160","render_scale":"75%","upscaler":"FSR 1.0"})["render_scale"]=="75%","Performance scaling unavailable at 4K")
 print("MOONFALL 4K DISPLAY PASS" if failures==0 else "MOONFALL 4K DISPLAY FAIL")
 quit(0 if failures==0 else 1)
