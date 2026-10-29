extends SceneTree
const R:=preload("res://scripts/systems/overhaul_v380.gd")
var failures:int=0
func _initialize()->void:call_deferred("_run")
func check(ok:bool,label:String)->void:
 if ok:print("PASS v38: "+label)
 else:
  failures+=1
  push_error("FAIL v38: "+label)
func _run()->void:
 check(R.air_urgency(8.0,true)==2,"critical air severity")
 check(R.air_urgency(18.0,true)==1,"low air severity")
 check(R.air_urgency(100.0,true)==0,"safe underwater oxygen")
 check(R.air_urgency(0.0,false)==0,"no oxygen panic on land")
 check("CRITICAL AIR" in R.threat_status(5.0,true,true,10.0,100.0),"oxygen emergency prioritized")
 check("COMBAT" in R.threat_status(100.0,false,true,90.0,100.0),"combat warning")
 check("LOW HEALTH" in R.threat_status(100.0,false,false,25.0,100.0),"low health warning")
 check("AIR: 20%" in R.safe_label("Moonmist",80.0,100.0,19.5,true,2.4,false),"accessible numeric air readout")
 var packed:PackedScene=load("res://scenes/main.tscn")
 check(packed!=null,"v38 source scene loads")
 if packed==null:quit(1);return
 var game:Node3D=packed.instantiate()
 root.add_child(game)
 await process_frame
 var sprite:Sprite3D=game.get("hero_sprite") as Sprite3D
 var pose:Node3D=game.get("v374_swim_pose_anchor") as Node3D
 check(is_instance_valid(pose),"independent swim pose anchor exists")
 check(is_instance_valid(sprite) and sprite.get_parent()==pose,"existing original hero art is mounted under pose anchor")
 check(is_instance_valid(sprite) and sprite.texture!=null,"original art sprite remains textured")
 check(is_instance_valid(pose) and pose.scale.is_equal_approx(Vector3.ONE),"pose transform has no cumulative scaling")
 # Dark-fantasy rendering reuses existing graphics options and has a
 # bounded cost. Real GPU volumetrics must never run in headless CI.
 var original_graphics:Dictionary=(game.get("gfx_settings") as Dictionary).duplicate(true)
 var dark:Dictionary=original_graphics.duplicate(true)
 dark["realism_grade"]="Cinematic"
 dark["cinematic_lighting"]="Cinematic"
 dark["environment_detail"]="9"
 dark["shadow_quality"]="High"
 dark["fog"]="Normal"
 game.set("gfx_settings",dark)
 game.call("_gfx_apply",false)
 var atmosphere:Environment=game.get("living_environment") as Environment
 check(atmosphere.adjustment_saturation<=0.86,"gothic color grading is applied")
 check(atmosphere.fog_enabled,"mist respects enabled fog setting")
 check(DisplayServer.get_name()!="headless" or not atmosphere.volumetric_fog_enabled,"GPU fog is disabled for headless")
 dark["fog"]="Off"
 game.set("gfx_settings",dark)
 game.call("_gfx_apply",false)
 check(not atmosphere.fog_enabled and not atmosphere.volumetric_fog_enabled,"fog Off disables normal and volumetric mist")
 dark["realism_grade"]="Classic"
 game.set("gfx_settings",dark)
 game.call("_gfx_apply",false)
 check(is_equal_approx(atmosphere.adjustment_saturation,1.0),"Classic restores neutral color grading")
 game.set("gfx_settings",original_graphics)
 game.call("_gfx_apply",false)
 check(is_instance_valid(game.get("v380_status_panel")),"optional status panel created")
 check(not (game.get("v380_status_panel") as PanelContainer).visible,"status HUD hidden at title screen")
 var current_title:bool=false
 var release_label:String=str(game.call("_release_label"))
 var found_titles:PackedStringArray=[]
 for label_node in (game.get("start_screen") as Control).find_children("*","Label",true,false):
  var title_text:String=(label_node as Label).text
  if "MOONFALL" not in title_text.to_upper():continue
  found_titles.append(title_text)
  if "MOONFALL "+release_label in title_text:current_title=true
 if not current_title:print("MOONFALL_V38_TITLE_DIAG expected=",release_label," actual=",found_titles)
 check(current_title,"start screen presents the current project release")
 game.call("_menu_tab","System")
 var body:RichTextLabel=game.get("menu_body") as RichTextLabel
 check(is_instance_valid(body) and "F2" in body.text,"F2 discoverable inside system menu")
 game.queue_free()
 print("MOONFALL v38.0 OVERHAUL PASS" if failures==0 else "MOONFALL v38.0 OVERHAUL FAIL")
 quit(0 if failures==0 else 1)
