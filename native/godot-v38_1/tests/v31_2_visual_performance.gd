extends SceneTree
const CURRENT_RELEASE = preload("res://tests/current_release.gd")
const RULES:=preload("res://scripts/systems/graphics_rules.gd")
var failures:int=0
func check(ok:bool,label:String)->void:
 if ok:print("PASS v31.2: "+label)
 else:
  failures+=1
  push_error("FAIL v31.2: "+label)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var packed:PackedScene=load("res://scenes/main.tscn")
 check(packed!=null,"Main scene loads")
 if packed==null:quit(1);return
 var game:Node3D=packed.instantiate()
 root.add_child(game)
 await process_frame
 check(CURRENT_RELEASE.matches_project(game),"v31.2 is active")
 var flora:Array=game.get("v312_foliage")
 var effects:Array=game.get("v312_hits")
 check(flora.size()==6,"Exactly six scenery batches")
 check(effects.size()==8,"Impact effects pooled once")
 check(int((game.get("v312_visual_stats") as Dictionary).get("max_instances",0))==576,"576 optional ground blades batched")
 var settings:Dictionary=(game.get("gfx_settings") as Dictionary).duplicate(true)
 settings["foliage_quality"]="Off"
 settings["combat_effects"]="Off"
 game.set("gfx_settings",settings)
 game.call("_gfx_apply",false)
 check(not (flora[0] as MultiMeshInstance3D).visible,"Off foliage hides cosmetic instances")
 game.call("_v312_impact",Vector3.ZERO)
 check(effects.filter(func(e):return e.visible).size()==0,"Off effects never allocate impact geometry")
 settings["foliage_quality"]="High"
 settings["combat_effects"]="High"
 settings["scenery_lod"]="Far"
 game.set("gfx_settings",settings)
 game.call("_gfx_apply",false)
 check((flora[0] as MultiMeshInstance3D).visible,"Foliage restored after quality change")
 check((flora[0] as MultiMeshInstance3D).multimesh.visible_instance_count==96,"High displays full per-region batch")
 check((flora[0] as MultiMeshInstance3D).visibility_range_end>30.0,"Distance culling configured")
 var pos:Vector3=(game.get("player") as CharacterBody3D).global_position
 game.call("_v312_impact",pos)
 check(effects.filter(func(e):return e.visible).size()==1,"Hit pool shows short impact flash")
 check((game.get("v312_hit_cursor") as int)==1,"Hit mesh is recycled")
 game.call("_atlas_execute","gfx_open")
 game.call("_atlas_execute","gfx_group:World")
 check("Foliage Quality" in (game.get("menu_body") as RichTextLabel).text,"New world quality control shown")
 game.call("_atlas_execute","gfx_group:Effects")
 check("Combat Effects" in (game.get("menu_body") as RichTextLabel).text,"New combat effects control shown")
 check(RULES.normalize({"projected_telegraphs":"Off"})["projected_telegraphs"]=="Always Visible","Telegraphs protected")
 game.queue_free()
 print("MOONFALL v31.2 VISUAL PERFORMANCE PASS" if failures==0 else "MOONFALL v31.2 VISUAL PERFORMANCE FAIL")
 quit(0 if failures==0 else 1)
