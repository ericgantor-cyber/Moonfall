extends SceneTree
const CURRENT_RELEASE = preload("res://tests/current_release.gd")
const RULES:=preload("res://scripts/systems/graphics_rules.gd")
var failed:int=0
func check(ok:bool,label:String)->void:
 if ok:print("PASS v30.9: "+label)
 else:
  failed+=1
  push_error("FAIL v30.9: "+label)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var low:Dictionary=RULES.preset(1)
 var ultra:Dictionary=RULES.preset(10)
 check(low["view_distance"]=="1" and ultra["view_distance"]=="10","presets span 1 through 10")
 check(low["ground_clutter"]=="Off" and ultra["ground_clutter"]=="Ultra","presets tune ground detail")
 check(RULES.normalize({"aa":"fake","render_scale":"invalid"})["aa"]==RULES.DEFAULTS["aa"],"invalid options fall back to safe values")
 check(RULES.normalize({"projected_telegraphs":"Off"})["projected_telegraphs"]=="Always Visible","attack warning safety cannot be disabled")
 check(RULES.cycle(RULES.DEFAULTS,"render_scale")["render_scale"] in RULES.OPTIONS["render_scale"],"render scale cycles supported choices")
 check(RULES.cycle(RULES.DEFAULTS,"projected_telegraphs")["projected_telegraphs"]=="Always Visible","warning control cannot be cycled")
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"main Godot scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(CURRENT_RELEASE.matches_project(game),"updated scene active")
 check(game.get("gfx_ready"),"graphics profile loads on startup")
 check((game.get("gfx_scatter") as Array).size()>=18,"scenery batches cached once")
 game.call("_menu_tab","System")
 var has_button:bool=false
 for button in (game.get("menu_buttons") as VBoxContainer).get_children():
  if button is Button and "ADVANCED GRAPHICS" in button.text:has_button=true
 check(has_button,"Raven Atlas System has graphics menu entry")
 game.call("_atlas_execute","gfx_open")
 check(game.get("current_tab")=="Graphics" and "MOONFALL GRAPHICS" in (game.get("menu_body") as RichTextLabel).text,"graphics UI opens")
 var presets:int=0
 for b in (game.get("menu_buttons") as VBoxContainer).get_children():
  if b is Button and b.text.begins_with("QUALITY PRESET "):presets+=1
 check(presets==10,"all ten quality preset buttons are selectable")
 game.call("_atlas_execute","gfx_preset:1")
 check(game.get("gfx_settings")["view_distance"]=="1","quality preset changes settings")
 check(get_root().get_viewport().scaling_3d_scale<1.0,"quality preset applies native 3D render scaling")
 game.call("_atlas_execute","gfx_group:Effects")
 game.call("_atlas_execute","gfx_cycle:ssao")
 check(game.get("gfx_settings")["ssao"] in RULES.OPTIONS["ssao"],"effects control cycles")
 game.call("_atlas_execute","gfx_group:World")
 game.call("_atlas_execute","gfx_cycle:shadow_quality")
 check((game.get_node("TwinMoonlight") as DirectionalLight3D).shadow_enabled or game.get("gfx_settings")["shadow_quality"]=="Off","shadow option applies")
 game.call("_atlas_execute","gfx_group:Unavailable")
 check("DLSS" in (game.get("menu_body") as RichTextLabel).text,"unavailable features identified, not faked")
 game.call("_atlas_execute","gfx_preset:10")
 check(get_root().get_viewport().scaling_3d_scale>1.0,"super-sampling applies")
 game.call("_atlas_execute","gfx_group:Advanced")
 game.call("_atlas_execute","gfx_cycle:brightness")
 check((game.get("living_environment") as Environment).adjustment_enabled,"brightness uses native environment adjustment")
 check(RULES.load_preferences()["view_distance"]=="10","graphics preferences saved outside hero checkpoint")
 # Graphics preferences are machine-specific and independent of hero saves.
 game.call("_atlas_execute","gfx_reset")
 game.call("_atlas_execute","gfx_back")
 check(game.get("current_tab")=="System","graphics menu returns to System")
 game.queue_free()
 if failed==0:print("MOONFALL v30.9 GRAPHICS PASS")
 quit(0 if failed==0 else 1)
