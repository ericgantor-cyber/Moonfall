extends SceneTree
const CURRENT_RELEASE = preload("res://tests/current_release.gd")
const RULES:=preload("res://scripts/systems/graphics_rules.gd")
var fails:int=0
func check(ok:bool,reason:String)->void:
 if ok:print("PASS v31.1: "+reason)
 else:
  fails+=1
  push_error("FAIL v31.1: "+reason)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 check(RULES.DEFAULTS["terrain_surface"]=="PBR Detail","PBR default")
 check(RULES.normalize({"realism_grade":"Invalid"})["realism_grade"]=="Cinematic","invalid grading rejected")
 check(RULES.preset(1)["terrain_surface"]=="Off" and RULES.preset(10)["terrain_surface"]=="PBR Detail","quality presets scale terrain detail")
 check(RULES.normalize({"projected_telegraphs":"Off"})["projected_telegraphs"]=="Always Visible","combat cue protected")
 var packed:PackedScene=load("res://scenes/main.tscn")
 check(packed!=null,"main scene loads")
 if packed==null:quit(1);return
 var game:Node3D=packed.instantiate()
 root.add_child(game)
 await process_frame
 check(CURRENT_RELEASE.matches_project(game),"new scene active")
 var mats:Array=game.get("realism_materials")
 var meshes:Array=game.get("realism_meshes")
 print("REALISM COUNT mats=",mats.size()," meshes=",meshes.size()," worldchildren=",(game.get("world") as Node3D).get_child_count())
 check(mats.size()==7 and meshes.size()>100,"six regions plus shared road surfaces")
 check((mats[0] as StandardMaterial3D).normal_enabled,"normal maps active on high detail")
 check((mats[0] as StandardMaterial3D).albedo_texture!=null,"new procedural albedo loaded")
 var settings:Dictionary=(game.get("gfx_settings") as Dictionary).duplicate(true)
 settings["realism_grade"]="Cinematic"
 settings["terrain_surface"]="PBR Detail"
 settings["soft_shadows"]="On"
 game.set("gfx_settings",settings)
 game.call("_gfx_apply",false)
 check((game.get("living_environment") as Environment).tonemap_mode==Environment.TONE_MAPPER_AGX,"cinematic grading works")
 check((game.get_node("TwinMoonlight") as DirectionalLight3D).light_angular_distance>0.0,"soft moon shadows enabled")
 check((meshes[0] as MeshInstance3D).material_override==mats[0],"PBR region material assigned")
 settings["terrain_surface"]="Color Detail"
 game.set("gfx_settings",settings)
 game.call("_gfx_apply",false)
 check(not (mats[0] as StandardMaterial3D).normal_enabled,"medium detail removes normal work")
 settings["terrain_surface"]="Off"
 game.set("gfx_settings",settings)
 game.call("_gfx_apply",false)
 check((meshes[0] as MeshInstance3D).material_override!=(mats[0] as Material),"lowest detail restores original ground")
 settings["terrain_surface"]="PBR Detail"
 settings["wet_surfaces"]="On"
 game.set("gfx_settings",settings)
 game.set("weather_name","Rain")
 game.call("_gfx_apply",false)
 check((mats[0] as StandardMaterial3D).roughness<0.75,"rain reduces surface roughness")
 game.set("weather_name","Clear")
 game.call("_refresh_world_lighting")
 check((mats[0] as StandardMaterial3D).roughness>0.80,"weather clears back to dry material")
 game.call("_atlas_execute","gfx_open")
 game.call("_atlas_execute","gfx_group:World")
 check("Terrain Surface" in (game.get("menu_body") as RichTextLabel).text and "Realism Grade" in (game.get("menu_body") as RichTextLabel).text,"realistic graphics controls visible")
 game.queue_free()
 print("MOONFALL v31.1 REALISM PASS" if fails==0 else "MOONFALL v31.1 REALISM FAIL")
 quit(0 if fails==0 else 1)
