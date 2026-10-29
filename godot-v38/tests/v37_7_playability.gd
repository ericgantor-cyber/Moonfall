extends SceneTree
const RULES:=preload("res://scripts/systems/playability_v377.gd")
var failures:int=0
func _initialize()->void:call_deferred("_run")
func check(ok:bool,label:String)->void:
 if ok:print("PASS playability: "+label)
 else:
  failures+=1
  push_error("FAIL playability: "+label)
func _run()->void:
 check(RULES.rescue_available(true,false,false,2.0,1,false),"underwater emergency exit offered")
 check(not RULES.rescue_available(true,false,false,0.0,1,false),"no free surface teleport on dry land")
 check(not RULES.rescue_available(true,false,false,2.0,1,true),"no teleport during combat")
 check(not RULES.rescue_available(true,true,false,2.0,1,false),"no escape through death")
 check(not RULES.rescue_available(true,false,true,2.0,1,false),"no mounted escape")
 var regions:Array=[{"at":Vector2(40.0,12.0)}]
 var shore:Vector3=RULES.shoreline(regions,0,Vector2(18.0,14.0),0.85)
 check(shore.distance_to(Vector3(64.7,0.85,31.4))<0.01,"safe position follows region and water offset")
 check("LOW AIR" in RULES.urgent_tip(2.0,15.0,80.0,100.0,false),"urgent oxygen hint")
 check("LOW HEALTH" in RULES.urgent_tip(0.0,100.0,15.0,100.0,false),"urgent HP hint")
 check("COMBAT" in RULES.urgent_tip(0.0,100.0,80.0,100.0,true),"combat hint")
 check(RULES.format_percent(23.0,100.0)=="23%","percent formatting")
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"active scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 game.call("_menu_tab","System")
 var body:RichTextLabel=game.get("menu_body") as RichTextLabel
 check(is_instance_valid(body) and "PLAYABILITY" in body.text,"System page has help actions")
 check(not game.call("_v377_can_rescue"),"no emergency exit on title screen")
 var accurate_title:bool=false
 for node in (game.get("start_screen") as Control).find_children("*","Label",true,false):
  if "MOONFALL v37.7" in (node as Label).text or "MOONFALL v38.0" in (node as Label).text:accurate_title=true
 check(accurate_title,"title screen no longer advertises obsolete v13")
 game.call("_atlas_execute","v377_help")
 check(game.get("current_tab")=="Tips","Tips action works")
 check("RIGHT NOW" in body.text,"contextual help shown")
 game.queue_free()
 print("MOONFALL v37.7 PLAYABILITY PASS" if failures==0 else "MOONFALL v37.7 PLAYABILITY FAIL")
 quit(0 if failures==0 else 1)
