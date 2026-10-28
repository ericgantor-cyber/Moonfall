extends SceneTree
## Godot headless regression for the v14 visual systems.
var failures := 0
func check(ok: bool, name: String) -> void:
 if ok:print("PASS v14: "+name)
 else:
  push_error("FAIL v14: "+name)
  failures+=1
func _initialize() -> void:
 call_deferred("_run")
func _run() -> void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"Scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 var fauna:Array=game.get("beasts")
 var rigs:Array=game.get("visual_rigs")
 var zones:Array=game.get("decorative_groups")
 check(fauna.size()==74 and rigs.size()==fauna.size(),"All 74 beasts have unique procedural rigs")
 check(zones.size()==6,"All six zones have visual art-direction roots")
 for z in range(zones.size()):
  check(zones[z].get_child_count()>=10,"Zone "+str(z)+" has landmarks and glowing motifs")
 var kinds:Dictionary={}
 var boss_rigs:=0
 var invasion_boss_rigs:=0
 for i in range(fauna.size()):
  var beast:Dictionary=fauna[i]
  var kind:String=beast["kind"]
  kinds[kind]=true
  var body:Node3D=beast["body"]
  check(rigs[i].get_child_count()>=5,"Rig geometry for "+kind) if i < 8 else null
  var existing_meshes:=0
  var hidden_meshes:=0
  for child in body.get_children():
   if child is MeshInstance3D:
    existing_meshes+=1
    if not child.visible:hidden_meshes+=1
  # Additional v32 boss warning meshes may be visible; only legacy placeholders must stay hidden.
  if existing_meshes<2 or hidden_meshes<2:
   check(false,"Old placeholder capsules are hidden")
  if bool(beast.get("boss",false)) and bool(rigs[i].get_meta("boss_ornamented",false)):
   if int(beast.get("invasion_slot",-1))>=0:invasion_boss_rigs+=1
   else:boss_rigs+=1
 check(kinds.size()==8,"Eight unique species silhouettes")
 check(boss_rigs==5,"All five roaming guardians have ornate rigs")
 check(invasion_boss_rigs==3,"All three dormant invasion commanders have ornate rigs")
 var all_colliders_ok:=true
 for beast in fauna:
  var collider_found:=false
  for child in (beast["body"] as Node3D).get_children():
   if child is CollisionShape3D:collider_found=true
  if not collider_found:all_colliders_ok=false
 check(all_colliders_ok,"Visual overhaul does not remove any creature colliders")
 check(is_instance_valid(game.get("life_bar")) and is_instance_valid(game.get("experience_bar")),"HUD health and XP meters")
 game.call("_apply_graphics_quality","Smooth")
 check(not zones[0].visible and not zones[5].visible,"Smooth quality hides optional landmarks")
 game.call("_apply_graphics_quality","Full")
 check(zones[0].visible and zones[5].visible,"Full quality restores all landmarks")
 game.call("_start_game","z")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Z_Original.webp"),"Z retains exact original texture")
 game.call("_start_game","nyra")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Nyra_Original.png"),"Nyra retains exact original texture")
 game.call("_impact_ring",Vector3.ZERO,Color.YELLOW)
 check(game.get("hit_effects").size()==1,"Reusable bounded combat impact visual created")
 game.call("_process",0.50)
 check(game.get("hit_effects").is_empty(),"Impact effect expires and is freed")
 game.call("_create_gear_drop",Vector3.ZERO,"wolf","moon",1)
 var drops:Array=game.get("gear_drops")
 check(not drops.is_empty() and (drops[-1]["node"] as Node3D).get_child_count()>=4,"Ground loot has rarity visual markings")
 for f in range(120):await physics_frame
 check(game.get("visual_frames")>0,"Low-frequency creature animation loop progresses")
 if failures==0:print("MOONFALL NATIVE v14 VISUALS PASS")
 else:push_error("MOONFALL NATIVE v14 VISUALS FAILED: "+str(failures))
 game.queue_free()
 quit(0 if failures==0 else 1)
