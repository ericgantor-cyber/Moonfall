extends SceneTree
const CURRENT_RELEASE = preload("res://tests/current_release.gd")
const RULES:=preload("res://scripts/systems/graphics_rules.gd")
var failures:int=0
func check(ok:bool,label:String)->void:
 if ok:print("PASS v31.0: "+label)
 else:
  failures+=1
  push_error("FAIL v31.0: "+label)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var defaults:Dictionary=RULES.DEFAULTS
 for key in RULES.OPTIONS:
  check(defaults.has(key) and defaults.get(key) in RULES.OPTIONS[key],"Valid default: "+key)
 check(RULES.normalize({"projected_telegraphs":"Off"})["projected_telegraphs"]=="Always Visible","Safety cues are locked on")
 check(RULES.normalize({"aa":"Fake"})["aa"]==defaults["aa"],"Unknown settings rejected")
 check(RULES.normalize({"view_distance":"2"})["upscaler"]=="Bilinear","Old graphics profiles get new defaults")
 check(RULES.preset(1)["treasure_highlights"]=="Minimal" and RULES.preset(10)["treasure_highlights"]=="Full","Presets tune decorative effects")
 check(RULES.preset(1)["upscaler"]=="FSR 1.0" and RULES.preset(10)["upscaler"]=="Bilinear","Presets choose supported spatial scaler")
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"Main scene loaded")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(CURRENT_RELEASE.matches_project(game),"Current scene activated")
 game.call("_atlas_execute","gfx_open")
 game.call("_atlas_execute","gfx_group:Display")
 check("Dynamic Resolution" in (game.get("menu_body") as RichTextLabel).text,"Display controls show dynamic resolution")
 game.call("_atlas_execute","gfx_group:Advanced")
 check("Anisotropy" in (game.get("menu_body") as RichTextLabel).text,"Advanced controls show anisotropic filtering")
 var setting:Dictionary=(game.get("gfx_settings") as Dictionary).duplicate(true)
 setting["render_scale"]="67%"
 setting["upscaler"]="FSR 1.0"
 game.set("gfx_settings",setting)
 game.call("_gfx_apply",false)
 check(get_root().get_viewport().scaling_3d_mode==Viewport.SCALING_3D_MODE_FSR,"FSR 1.0 native viewport mode")
 check(is_equal_approx(get_root().get_viewport().scaling_3d_scale,0.67),"Custom render scale retained")
 setting=(game.get("gfx_settings") as Dictionary).duplicate(true)
 setting["upscaler"]="Bilinear"
 setting["render_scale"]="125%"
 game.set("gfx_settings",setting)
 game.call("_gfx_apply",false)
 check(get_root().get_viewport().scaling_3d_mode==Viewport.SCALING_3D_MODE_BILINEAR,"Bilinear supersampling remains supported")
 setting=(game.get("gfx_settings") as Dictionary).duplicate(true)
 setting["shadow_quality"]="High"
 setting["shadow_distance"]="Near"
 game.set("gfx_settings",setting)
 game.call("_gfx_apply",false)
 var light:=game.get_node("TwinMoonlight") as DirectionalLight3D
 var near_range:float=light.directional_shadow_max_distance
 setting["shadow_distance"]="Far"
 game.set("gfx_settings",setting)
 game.call("_gfx_apply",false)
 check(light.directional_shadow_max_distance>near_range,"Independent shadow distance changes light range")
 setting=(game.get("gfx_settings") as Dictionary).duplicate(true)
 setting["treasure_highlights"]="Minimal"
 game.set("gfx_settings",setting)
 game.call("_gfx_apply",false)
 var treasure:Array=game.get("v308_treasure_fx")
 check(treasure.size()==6,"Six chest FX containers intact")
 if treasure.size()>0:
  check(not (treasure[0]["halo"] as MeshInstance3D).visible,"Low cosmetic preset disables optional halo")
  check((game.get("treasure_chests") as Array).size()==6,"Chest gameplay nodes not removed")
 setting["treasure_highlights"]="Full"
 setting["particle_density"]="High"
 setting["environment_detail"]="8"
 game.set("gfx_settings",setting)
 game.call("_gfx_apply",false)
 if treasure.size()>0:check((treasure[0]["halo"] as MeshInstance3D).visible,"Treasure visual returns after quality increase")
 game.call("_atlas_execute","gfx_reset")
 check(RULES.load_preferences()["dynamic_resolution"]=="Off","Saved display settings remain independent of hero saves")
 game.queue_free()
 print("MOONFALL v31.0 GRAPHICS PASS" if failures==0 else "MOONFALL v31.0 GRAPHICS FAIL: "+str(failures))
 quit(0 if failures==0 else 1)
