extends SceneTree
## Headless regression of v15 animation, materials, coronas and cosmetic budgets.
var failed := 0
func check(ok:bool, label:String) -> void:
 if ok:print("PASS v15: "+label)
 else:
  failed += 1
  push_error("FAIL v15: "+label)
func _initialize() -> void:
 call_deferred("_run")
func _run() -> void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"v15 main scene parses")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 var beasts:Array=game.get("beasts")
 var tracks:Array=game.get("motion_tracks")
 var coats:Dictionary=game.get("pelt_textures")
 var coronas:Dictionary=game.get("guardian_coronas")
 var wisps:Array=game.get("regional_wisps")
 check(tracks.size()==beasts.size() and tracks.size()==74,"All 74 living actors get bounded motion tracks")
 check(coats.size()>=8,"Procedural coat textures generated for all eight species")
 var boss_count:int=0
 for beast in beasts:
  if bool(beast.get("boss",false)):
   boss_count+=1
   check(coronas.has(int(beast["boss_id"])),"Boss "+str(beast["boss_id"])+" has a phase corona")
 check(boss_count==8 and coronas.size()==boss_count,"Five guardians and three invasion commanders have phase coronas")
 check(wisps.size()==30,"Five ambient wisps per region, six regions")
 var unique_patterns:=true
 for entry in tracks:
  var rig:Node3D=entry["rig"]
  if not is_instance_valid(rig):unique_patterns=false
  elif (rig.get_child(0) as MeshInstance3D).material_override is StandardMaterial3D:
   var m:StandardMaterial3D=(rig.get_child(0) as MeshInstance3D).material_override
   if m.albedo_texture==null:unique_patterns=false
  else:unique_patterns=false
 check(unique_patterns,"Every creature has a painted torso or scales")
 game.call("_start_game","z")
 var starter:Dictionary=beasts[0]
 var actor:CharacterBody3D=starter["body"]
 actor.position=(game.get("player") as Node3D).position+Vector3(3,0,0)
 actor.velocity=Vector3(2.3,0,0)
 game.set("map_time",1.4)
 game.call("_process",0.12)
 check(int(game.get("animated_this_tick"))>0,"Nearby animals animate")
 var moved:=false
 for part in tracks[0]["legs"]:
  if absf((part["mesh"] as MeshInstance3D).rotation.z)>0.001:moved=true
 check(moved,"Quadruped legs step with motion")
 var owl_index:=-1
 for i in range(beasts.size()):
  if beasts[i]["kind"]=="owl" and int(beasts[i]["zone"])==0:owl_index=i;break
 check(owl_index>=0,"Original six owl slots available")
 if owl_index>=0:
  var owl:CharacterBody3D=beasts[owl_index]["body"]
  owl.position=(game.get("player") as Node3D).position+Vector3(4,0,0)
  owl.velocity=Vector3(2,0,0)
  game.set("map_time",2.1)
  game.call("_process",0.12)
  var wings:Array=tracks[owl_index]["wings"]
  check(wings.size()==2 and absf((wings[0] as MeshInstance3D).rotation.x)>0.01,"Owls flap their wings")
 var boss_index:=-1
 for i in range(beasts.size()):
  if bool(beasts[i]["boss"]):boss_index=i;break
 check(boss_index>=0,"Original roaming guardians still present")
 if boss_index>=0:
  var boss:Dictionary=beasts[boss_index]
  boss["hp"]=float(boss["max"])*0.62
  game.call("_refresh_boss_phase",boss)
  check(int(boss["boss_phase"])==2 and (coronas[int(boss["boss_id"])] as Node3D).visible,"Guardian aura activates at phase 2")
  boss["hp"]=float(boss["max"])*0.22
  game.call("_refresh_boss_phase",boss)
  check(int(boss["boss_phase"])==3,"Guardian phase 3 remains functional")
 game.call("_apply_graphics_quality","Smooth")
 check(not (wisps[0]["node"] as Node3D).get_parent().visible,"Smooth mode hides decorative wisps")
 game.call("_apply_graphics_quality","Full")
 check((wisps[0]["node"] as Node3D).get_parent().visible,"Full quality re-enables decorative wisps")
 actor.position=Vector3(400,0,400)
 game.set("map_time",3.3)
 game.call("_process",0.12)
 check(int(game.get("far_animation_skips"))>0,"Far creatures are culled from cosmetic updates")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Z_Original.webp"),"Z's original texture is kept intact")
 game.call("_start_game","nyra")
 check(load("res://assets/heroes/Nyra_Original.png")!=null,"Nyra boss texture asset remains available")
 if failed==0:print("MOONFALL NATIVE v15 LIVING ART PASS")
 else:push_error("MOONFALL NATIVE v15 FAILED: "+str(failed))
 game.queue_free()
 quit(0 if failed==0 else 1)
