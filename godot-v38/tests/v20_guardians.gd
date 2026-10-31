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
 check(sigils.size()==8,"Five guardians and three invasion commanders have spellwheels (actual="+str(sigils.size())+")")
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
  game.call("_apply_graphics_quality","Full")
  game.call("_process",0.2)
  check(game.get("v20_animated_guardians")>0,"Boss spellwheel animates when close (quality="+str(game.get("graphics_quality"))+", alive="+str(target.get("alive"))+", count="+str(game.get("v20_animated_guardians"))+")")
  game.call("_apply_graphics_quality","Smooth")
  game.call("_process",0.2)
  check(not root_sigil.visible,"Smooth quality hides extra cinematic effects")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Z_Original.webp"),"Z texture unchanged")
 game.call("_start_game","nyra")
 check(load("res://assets/heroes/Nyra_Original.png")!=null,"Original Nyra boss artwork is available")
 if errors==0:print("MOONFALL v20 GUARDIANS PASS")
 game.queue_free()
 quit(0 if errors==0 else 1)
