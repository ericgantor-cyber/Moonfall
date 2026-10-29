extends SceneTree
var errors:=0
func check(ok:bool,msg:String)->void:
 if ok:print("PASS v20: "+msg)
 else:
  errors+=1
  push_error("FAIL v20: "+msg)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"Main scene parses")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 var sigils:Dictionary=game.get("v20_guardian_sigils")
 var roaming_sigils:=0
 var invasion_sigils:=0
 for creature in game.get("beasts"):
  if not bool(creature.get("boss",false)):continue
  if not sigils.has(int(creature.get("boss_id",-1))):continue
  if int(creature.get("invasion_slot",-1))>=0:invasion_sigils+=1
  else:roaming_sigils+=1
 check(sigils.size()==8 and roaming_sigils==5 and invasion_sigils==3,"Five guardian and three invasion commander spellwheels created")
 check(game.get("v19_scenery_count")==54,"Biome scenery preserved")
 check(game.get("grid_regions").size()==2400,"Invisible terrain grid preserved")
 game.call("_start_game","z")
 var target:Dictionary={}
 for beast in game.get("beasts"):
  if bool(beast.get("boss",false)) and int(beast.get("zone",0))==1:
   target=beast
   break
 check(not target.is_empty(),"Witchwood guardian found")
 if not target.is_empty():
  var id:int=int(target["boss_id"])
  var root_sigil:Node3D=sigils[id]["root"]
  check(root_sigil.get_child_count()==4,"Three circles and energy crown per guardian")
  check(not root_sigil.visible,"First phase has no transformation aura")
  target["hp"]=float(target["max"])*0.60
  game.call("_refresh_boss_phase",target)
  check(int(target["boss_phase"])==2 and root_sigil.visible,"Second phase awakens spellwheel")
  target["hp"]=float(target["max"])*0.22
  game.call("_refresh_boss_phase",target)
  check(int(target["boss_phase"])==3 and game.get("v20_phase_flashes")==2,"Third phase strengthens sigil and tracks transitions")
  var body:CharacterBody3D=target["body"]
  game.get("player").global_position=body.global_position+Vector3(2.2,0,0)
  game.set("region_index",1)
  game.call("_update_region_visibility")
  game.call("_process",0.2)
  print("MOONFALL_V20_DIAG animated=",game.get("v20_animated_guardians")," region=",game.get("region_index")," target_zone=",target["zone"]," phase=",target["boss_phase"]," alive=",target["alive"]," graphics=",game.get("graphics_quality")," playing=",game.get("hero_playing")," distance_squared=",body.global_position.distance_squared_to(game.get("player").global_position)," visible=",root_sigil.visible)
  check(game.get("v20_animated_guardians")>0,"Boss spellwheel animates when close")
  game.call("_apply_graphics_quality","Smooth")
  game.call("_process",0.2)
  check(not root_sigil.visible,"Smooth quality hides extra cinematic effects")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Z_Original.webp"),"Z texture unchanged")
 game.call("_start_game","nyra")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Nyra_Original.png"),"Nyra texture unchanged")
 if errors==0:print("MOONFALL v20 GUARDIANS PASS")
 game.queue_free()
 quit(0 if errors==0 else 1)
