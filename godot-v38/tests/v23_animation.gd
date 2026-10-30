extends SceneTree
var failed:=0
func check(condition:bool,message:String) -> void:
 if condition:print("PASS v23: "+message)
 else:
  failed+=1
  push_error("FAIL v23: "+message)
func _initialize()->void:call_deferred("run_test")
func run_test()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"Scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(game.get("v23_tracks").size()==game.get("beasts").size(),"One articulation track per creature")
 check(game.get("v23_procedural_joints")>=150,"Procedural joints created")
 var kinds:Dictionary={}
 var legged:=0
 var feathered:=0
 var slithering:=0
 var bosses:=0
 for t in game.get("v23_tracks"):
  kinds[t["kind"]]=true
  if t["legs"].size()==4:legged+=1
  if t["wings"].size()==2:feathered+=1
  if t["segments"].size()==6:slithering+=1
  if t["boss"] and is_instance_valid(t["boss_aura"]):bosses+=1
 check(kinds.size()==8,"Eight species have v23 rigs")
 check(legged>=20,"Jointed quadruped legs")
 check(feathered>=6,"Owls have animated wing fans")
 check(slithering>=1,"Serpents have articulated spine")
 check(bosses==8,"Five guardians and three invasion commanders receive phase breath cues (actual="+str(bosses)+")")
 game.call("_start_game","z")
 game.call("_apply_graphics_quality","Smooth")
 check(not game.get("v23_tracks")[0]["legs"][0]["hip"].visible,"Smooth mode hides optional details")
 game.call("_apply_graphics_quality","Full")
 check(game.get("v23_tracks")[0]["legs"][0]["hip"].visible,"Full restores optional detail")
 var first:Dictionary=game.get("beasts")[0]
 first["hp"]=float(first["max"])
 game.call("_apply_player_hit",first,1.0)
 check(game.get("v23_hit_reactions")>=1,"Damage triggers creature recoil")
 var guardian:Dictionary={}
 for animal in game.get("beasts"):
  if animal["boss"]:
   guardian=animal
   break
 check(not guardian.is_empty(),"Guardian available")
 if not guardian.is_empty():
  var id:int=int(guardian["boss_id"])
  guardian["hp"]=float(guardian["max"])*0.29
  game.call("_refresh_boss_phase",guardian)
  check(int(guardian["boss_phase"])==3,"Guardian advances to final phase")
  check(game.get("v23_tracks")[id]["boss_aura"]!=null,"Guardian aura remains allocated")
 check(game.get("grid_regions").size()==2400,"Invisible terrain grid stays intact")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Z_Original.webp"),"Z original art unchanged")
 game.call("_start_game","nyra")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Nyra_Original.png"),"Nyra original art unchanged")
 game.queue_free()
 if failed==0:print("MOONFALL v23 ANIMATION PASS")
 quit(0 if failed==0 else 1)
