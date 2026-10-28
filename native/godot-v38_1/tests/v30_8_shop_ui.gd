extends SceneTree
const CURRENT_RELEASE = preload("res://tests/current_release.gd")
const FORGE=preload("res://scripts/systems/forge_rules.gd")
const MONEY=preload("res://scripts/systems/money_rules.gd")
var failed:int=0
func check(ok:bool,description:String)->void:
 if ok:print("PASS v30.8: "+description)
 else:
  failed+=1
  push_error("FAIL v30.8: "+description)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var profile:String=OS.get_environment("MOONFALL_TEST_PROFILE")
 if profile.is_empty() or not ProjectSettings.globalize_path("user://").begins_with(profile):
  check(false,"isolated test-save profile required")
  quit(1)
  return
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(CURRENT_RELEASE.matches_project(game),"current version label")
 var chest_fx:Array=game.get("v308_treasure_fx")
 check(chest_fx.size()==6,"six treasure decorations")
 check(game.get("treasure_chests").size()==6,"all original chests remain")
 for chest in chest_fx:
  check(not (chest["halo"] as MeshInstance3D).get_parent() is CollisionObject3D,"decorations are non-colliding")
 game.call("_apply_graphics_quality","Smooth")
 check(not (chest_fx[0]["halo"] as MeshInstance3D).visible,"optional effects hidden in Smooth mode")
 game.call("_apply_graphics_quality","Full")
 check((chest_fx[0]["halo"] as MeshInstance3D).visible,"effects visible in Full mode")
 game.call("_start_game","z")
 for beast in game.get("beasts"):beast["hostile"]=false
 var first:Dictionary=game.get("treasure_chests")[0]
 game.get("player").global_position=(first["node"] as Node3D).global_position+Vector3(1,0.85,0)
 check(game.call("_open_treasure_chest",0),"original cache interaction works")
 check(not (chest_fx[0]["halo"] as MeshInstance3D).visible,"opened cache loses glow")
 game.call("_save_game")
 var checkpoint:Dictionary=game.call("_read_save")
 game.call("_restore_data",checkpoint)
 check(game.get("opened_treasure_chests")==[0] and not (chest_fx[0]["halo"] as MeshInstance3D).visible,"cache visuals restore from checkpoint")
 var shrine:Node3D=game.get("region_shrines")[0]
 game.get("player").global_position=shrine.global_position+Vector3(0,0.85,3)
 game.call("_equip",0)
 check(not game.get("equipped").is_empty(),"gear equipped for test")
 var equipped:Dictionary=game.get("equipped")
 equipped["durability"]=68
 game.call("_sync_equipped_to_bag")
 game.set("gold",1000)
 game.set("silver",0)
 game.set("copper",0)
 game.call("_menu_tab","Wallet")
 var buttons:Node=game.get("menu_buttons")
 var repair_buttons:int=0
 for node in buttons.get_children():
  if node is Button and node.text.begins_with("REPAIR EQUIPPED"):repair_buttons+=1
 check(repair_buttons==1,"blacksmith displays clickable repair control")
 var repair_cost:int=FORGE.repair_cost(game.get("equipped"))
 var wallet_before:int=MONEY.total(game.get("gold"),game.get("silver"),game.get("copper"))
 for node in buttons.get_children():
  if node is Button and node.text.begins_with("REPAIR EQUIPPED") and not node.is_queued_for_deletion():
   node.pressed.emit()
   break
 check(int(game.get("equipped").get("durability",0))==100,"Atlas action actually repairs gear")
 check(MONEY.total(game.get("gold"),game.get("silver"),game.get("copper"))==wallet_before-repair_cost,"Atlas repair charges exact amount")
 # Two damaged items, one mirrored by equipped: should charge no duplicate.
 game.get("equipped")["durability"]=45
 game.call("_sync_equipped_to_bag")
 var bag: Array=game.get("gear_items")
 var second:Dictionary={"name":"Second Test Weapon","power":18,"rarity":"Rare","durability":30,"uid":"test-secondary"}
 bag.append(second)
 var total_expected:int=FORGE.repair_cost(game.get("equipped"))+FORGE.repair_cost(second)
 check(game.call("_bag_repair_cost")==total_expected,"bulk cost counts equipped gear once")
 wallet_before=MONEY.total(game.get("gold"),game.get("silver"),game.get("copper"))
 game.call("_atlas_execute","forge_repair_all")
 check(int(game.get("equipped")["durability"])==100 and int(bag[1]["durability"])==100,"bulk repair restores all condition")
 check(MONEY.total(game.get("gold"),game.get("silver"),game.get("copper"))==wallet_before-total_expected,"bulk repair makes one exact charge")
 game.call("_save_game")
 checkpoint=game.call("_read_save")
 game.call("_restore_data",checkpoint)
 check(int(game.get("equipped").get("durability",0))==100,"bulk repair survives save/load")
 game.get("equipped")["durability"]=1
 game.call("_sync_equipped_to_bag")
 game.set("gold",0)
 game.set("silver",0)
 game.set("copper",0)
 game.call("_menu_tab","Wallet")
 var disabled:bool=false
 for node in buttons.get_children():
  if node is Button and not node.is_queued_for_deletion() and node.text.begins_with("REPAIR EQUIPPED"):
   disabled=node.disabled
 check(disabled,"unaffordable repair button disabled")
 var condition_before:int=int(game.get("equipped")["durability"])
 check(not game.call("_forge_action","forge_repair_all") and int(game.get("equipped")["durability"])==condition_before,"failed repair doesn't mutate gear")
 game.queue_free()
 print("MOONFALL v30.8 SHOP UI "+("PASS" if failed==0 else "FAIL "+str(failed)))
 quit(0 if failed==0 else 1)
