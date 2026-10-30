extends SceneTree
## v17: headless boss-interface and warning regression. Does not change original art.
var failures:=0
func check(ok:bool,label:String) -> void:
 if ok:print("PASS v17: "+label)
 else:
  failures+=1
  push_error("FAIL v17: "+label)
func _initialize() -> void:call_deferred("_run")
func _run() -> void:
 var resource:PackedScene=load("res://scenes/main.tscn")
 check(resource!=null,"Scene parses")
 if resource==null:quit(1);return
 var game:Node3D=resource.instantiate()
 root.add_child(game)
 await process_frame
 check(is_instance_valid(game.get("boss_ui")),"Guardian panel exists")
 check(not game.get("boss_ui").visible,"Boss panel hidden at start screen")
 check(game.get("zone_surface_textures").size()==6,"Existing biome textures remain")
 check(game.get("beasts").size()==74,"Existing wildlife, encounter IDs, and lunar invasion actors remain")
 game.call("_start_game","z")
 var guardian:Dictionary={}
 for b in game.get("beasts"):
  if bool(b.get("boss",false)) and int(b.get("zone",-1))==1:
   guardian=b
   break
 check(not guardian.is_empty(),"Region 1 guardian exists")
 if guardian.is_empty():game.queue_free();quit(1);return
 var body:CharacterBody3D=guardian["body"]
 var player:CharacterBody3D=game.get("player")
 player.global_position=body.global_position+Vector3(2.0,0,1.0)
 game.set("region_index",1)
 game.call("_update_region_visibility")
 game.call("_update_guardian_overlay")
 check(game.get("boss_ui").visible,"Nearby guardian shows health overlay")
 check(int(game.get("boss_health").max_value)==int(guardian["max"]),"Health bar uses real boss maximum")
 check("PHASE 1/3" in game.get("boss_title").text,"Health panel announces phase")
 guardian["windup"]=0.9
 guardian["warning_radius"]=3.7
 guardian["warning_pos"]=player.global_position
 game.call("_set_boss_telegraph",guardian,true)
 game.call("_update_guardian_overlay")
 check("INCOMING ATTACK" in game.get("boss_hint").text,"Attack countdown is shown")
 var warning:MeshInstance3D=game.get("boss_telegraphs")[int(guardian["boss_id"])]
 check(warning.visible and is_equal_approx((warning.mesh as CylinderMesh).bottom_radius,3.7),"Warning matches real hit radius")
 var pulse_before:int=game.get("boss_warning_pulses")
 guardian["windup"]=0.0
 guardian["hp"]=float(guardian["max"])*0.25
 game.call("_refresh_boss_phase",guardian)
 check(int(guardian["boss_phase"])==3,"Guardian enters phase three")
 check(game.get("boss_warning_pulses")==pulse_before+1,"Transformation has one bounded pulse")
 game.call("_update_guardian_overlay")
 check("PHASE 3/3" in game.get("boss_title").text,"Panel reports transformed phase")
 game.set("menu_open",true)
 game.call("_update_guardian_overlay")
 check(not game.get("boss_ui").visible,"Boss UI hides behind Atlas")
 game.set("menu_open",false)
 guardian["alive"]=false
 game.call("_update_guardian_overlay")
 check(not game.get("boss_ui").visible,"Boss UI hides when guardian defeated")
 game.call("_start_game","nyra")
 check(not game.get("boss_ui").visible,"New hero begins with hidden boss UI")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Nyra_Original.png"),"Original Nyra texture used")
 check(game.get("zone_surface_textures").size()==6,"Regions survive hero transition")
 if failures==0:print("MOONFALL NATIVE v17 RELEASE PASS")
 else:push_error("MOONFALL NATIVE v17 RELEASE FAILED: "+str(failures))
 game.queue_free()
 quit(0 if failures==0 else 1)
