extends SceneTree
const R:=preload("res://scripts/systems/overhaul_v380.gd")
const FUSION:=preload("res://scripts/systems/world_fusion_rules.gd")
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
 var zones:Array=[{"name":"Forest"},{"name":"Citadel"}]
 check(FUSION.next_objective(zones,[80,225],[],2,95)["name"]=="Forest","power-aware world guide selects reachable event")
 check(FUSION.next_objective(zones,[80,225],[0],2,95)["name"]=="Citadel","guide advances through unfinished events")
 check(FUSION.next_objective(zones,[80,225],[0,1],2,95)["kind"]=="Rift","world events lead to existing rifts")
 check(FUSION.event_crafting_bonus(2,{"Mining":1})["name"]=="Moon Sigil","world quests reward existing profession materials")
 check(FUSION.event_crafting_bonus(5,{"Herbalism":26})["amount"]==2,"trained professions grant a capped bonus")
 check("no region is locked" in FUSION.guidance({"name":"Citadel","kind":"World event"},30,100),"region power remains advisory")
 var packed:PackedScene=load("res://scenes/main.tscn")
 check(packed!=null,"v38 source scene loads")
 if packed==null:quit(1);return
 var game:Node3D=packed.instantiate()
 root.add_child(game)
 await process_frame
 var sprite:Sprite3D=game.get("hero_sprite") as Sprite3D
 var pose:Node3D=game.get("v374_swim_pose_anchor") as Node3D
 check(is_instance_valid(pose),"independent swim pose anchor exists")
 var rigs:Dictionary=game.get("v382_rigs") as Dictionary
 check(rigs.size()==2,"Z and Nyra each have a rig scene")
 var z_rig:Node3D=rigs.get("z") as Node3D
 var nyra_rig:Node3D=rigs.get("nyra") as Node3D
 check(is_instance_valid(z_rig) and is_instance_valid(nyra_rig),"both rigs instantiate")
 check(is_instance_valid(sprite) and sprite.get_parent().name=="ArtMotionAnchor" and sprite.get_parent().get_parent()==z_rig and z_rig.get_parent()==pose,"original hero art remains under independent swim and animation anchors")
 var z_skeleton:Skeleton3D=z_rig.get_node_or_null("ArtMotionAnchor/HeroSkeleton") as Skeleton3D
 var nyra_skeleton:Skeleton3D=nyra_rig.get_node_or_null("ArtMotionAnchor/HeroSkeleton") as Skeleton3D
 check(is_instance_valid(z_skeleton) and z_skeleton.get_bone_count()==23 and is_instance_valid(nyra_skeleton) and nyra_skeleton.get_bone_count()==23,"both rigs have 17 core joints and six secondary hair/cloth joints")
 check(is_instance_valid(z_rig.call("get_socket","WeaponSocket")) and is_instance_valid(nyra_rig.call("get_socket","AuraSocket")),"character attachment sockets exist")
 check((z_rig.get_node("AnimationPlayer") as AnimationPlayer).has_animation("attack") and (nyra_rig.get_node("AnimationPlayer") as AnimationPlayer).has_animation("cast"),"both rigs have Godot animation clips")
 # Synthetic image proves cutout import, movement and fail-safe fallback.
 var demo_image:Image=Image.create(8,8,false,Image.FORMAT_RGBA8)
 demo_image.fill(Color.WHITE)
 var demo_tex:ImageTexture=ImageTexture.create_from_image(demo_image)
 var demo_sprite:=Sprite3D.new()
 demo_sprite.texture=demo_tex
 (z_rig.call("get_art_pivot") as Node3D).add_child(demo_sprite)
 var test_layers:Dictionary={}
 for part in (z_rig.call("required_cutout_parts") as PackedStringArray):test_layers[part]=demo_tex
 check(not z_rig.call("install_cutouts",demo_sprite,test_layers,false) and demo_sprite.visible,"unreviewed layers never hide original art")
 check(z_rig.call("install_cutouts",demo_sprite,test_layers,true),"complete approved cutout layers bind")
 check(z_rig.call("is_cutout_active") and not demo_sprite.visible,"approved layers can move separately")
 z_rig.call("play_state","run")
 z_rig.call("step_bones",0.12)
 var hip_index:int=z_skeleton.find_bone("Hip_L")
 check(not z_skeleton.get_bone_pose_rotation(hip_index).is_equal_approx(Quaternion.IDENTITY),"articulated hips move independently")
 z_rig.call("restore_original")
 check(demo_sprite.visible and not z_rig.call("is_cutout_active"),"original source restored by one call")
 demo_sprite.queue_free()
 game.call("_start_game","z")
 check(sprite.texture==load("res://assets/heroes/Z_Original.webp"),"Z original texture is unchanged")
 game.call("_attack")
 check(z_rig.get("current_state")=="attack","Z attack triggers rig motion")
 game.call("_start_game","nyra")
 check(sprite.texture==load("res://assets/heroes/Nyra_Original.png") and sprite.get_parent().get_parent()==nyra_rig,"Nyra uses unedited original art on her own rig")
 game.call("_spell","nova")
 check(nyra_rig.get("current_state")=="cast","Nyra spell triggers rig motion")
 game.call("_start_game","z")
 check(sprite.texture==load("res://assets/heroes/Z_Original.webp") and sprite.get_parent().get_parent()==z_rig,"Z and Nyra swap rigs without replacing art")
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
 # Witchwood only: actual Godot batch count, parent culling and quality sweep.
 var slice:Dictionary=game.get("v384_slice") as Dictionary
 check(not slice.is_empty() and int(slice.get("tree_count",0))==48,"Witchwood builds 48 deterministic forest trees")
 if not slice.is_empty():
  var forest_root:Node3D=slice["root"] as Node3D
  var regions:Array=game.get("region_nodes") as Array
  check(forest_root.get_parent()==regions[1],"forest visuals are culled with Witchwood rather than a new world")
  check(forest_root.find_children("*","CollisionObject3D",true,false).is_empty(),"cinematic scenery adds no collisions or combat blockers")
  var trunks:MultiMeshInstance3D=slice["trunks"] as MultiMeshInstance3D
  var canopies:MultiMeshInstance3D=slice["crowns"] as MultiMeshInstance3D
  var motes:MultiMeshInstance3D=slice["motes"] as MultiMeshInstance3D
  check(trunks.multimesh.instance_count==48 and canopies.multimesh.instance_count==96 and motes.multimesh.instance_count==28,"trees, canopy and fireflies use fixed GPU batches")
  var light:OmniLight3D=slice["light"] as OmniLight3D
  check(is_instance_valid(light) and not light.shadow_enabled,"moonwood gate uses bounded, unshadowed accent light")
  var low:Dictionary=original_graphics.duplicate(true)
  low["environment_detail"]="2"
  low["foliage_quality"]="Off"
  low["ground_clutter"]="Off"
  game.set("gfx_settings",low)
  game.call("_gfx_apply",false)
  check(not trunks.visible and not motes.visible and not light.visible,"low settings cull expensive scene details")
  var high:Dictionary=original_graphics.duplicate(true)
  high["environment_detail"]="9"
  high["foliage_quality"]="High"
  high["ground_clutter"]="High"
  high["shadow_quality"]="High"
  high["particle_density"]="High"
  game.set("gfx_settings",high)
  game.call("_gfx_apply",false)
  check(trunks.visible and trunks.multimesh.visible_instance_count==48 and motes.visible,"high settings restore detailed Witchwood scenery")
  game.set("region_index",1)
  game.call("_v384_update_hero_light")
  var hero_fill:OmniLight3D=game.get("v384_hero_fill") as OmniLight3D
  check(is_instance_valid(hero_fill) and hero_fill.get_parent()==game.get("player") and hero_fill.visible,"Witchwood hero fill follows untouched original character")
  game.set("region_index",0)
  game.call("_v384_update_hero_light")
  check(not hero_fill.visible,"hero fill automatically switches off outside Witchwood")
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
 game.call("_menu_tab","World")
 check("ONE CONNECTED REALM" in body.text and "WORLD RIFTS" in body.text,"World page unifies existing adventure systems")
 var before_material:int=int((game.get("materials") as Dictionary).get("herb",0))
 game.call("_complete_world_event",0,true)
 check(int((game.get("materials") as Dictionary).get("herb",0))>before_material,"world completion contributes to existing crafting inventory")
 var once:int=int(game.get("v383_event_rewards"))
 game.call("_complete_world_event",0,true)
 check(int(game.get("v383_event_rewards"))==once,"regional reward cannot be claimed twice")
 game.call("_atlas_execute","v383:professions")
 check(game.get("current_tab")=="Professions","World navigation reuses existing profession page")
 game.queue_free()
 print("MOONFALL v38.0 OVERHAUL PASS" if failures==0 else "MOONFALL v38.0 OVERHAUL FAIL")
 quit(0 if failures==0 else 1)
