extends SceneTree
const CURRENT_RELEASE = preload("res://tests/current_release.gd")
const RULES:=preload("res://scripts/systems/inventory_rules.gd")
var errors:int=0
func check(condition:bool,title:String)->void:
 if condition:print("PASS v30.5: "+title)
 else:
  errors+=1
  push_error("FAIL v30.5: "+title)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 check(RULES.BAG_START_CAPACITY==100 and RULES.BAG_MAX_CAPACITY==500 and RULES.VAULT_LIMIT==1000,"100-slot bag with 500 cap and 1000-slot vault")
 check(RULES.valid_capacity(24)==100 and RULES.valid_capacity(999)==500,"capacity migration clamps safely")
 check(RULES.can_store_gear(99) and not RULES.can_store_gear(100),"default bag starts at 100")
 check(RULES.can_store_gear(499,500) and not RULES.can_store_gear(500,500),"upgraded bag respects 500 cap")
 check(RULES.material_stack_counts(2005)==[999,999,7],"material stacks split at 999")
 check(RULES.material_stack_counts(0).is_empty(),"empty material requires no stacks")
 var bag:Array=[{"name":"Wolf Charm","power":10,"rarity":"Rare"},{"name":"Elder Mantle","power":50,"rarity":"Legendary"}]
 var vault:Array=[]
 check(not RULES.transfer_gear(bag,vault,100,1000),"cannot transfer out-of-range index")
 check(RULES.transfer_gear(bag,vault,0,1000) and bag.size()==1 and vault.size()==1,"deposit into vault moves item exactly once")
 check(RULES.transfer_gear(vault,bag,0,100) and bag.size()==2 and vault.is_empty(),"withdraw restores item")
 check(not RULES.transfer_gear(vault,bag,0,100),"cannot duplicate item by repeated withdrawal")
 var gear:Array=[{"name":"Sun Ring","power":5,"rarity":"Uncommon"},{"name":"Moon Crown","power":60,"rarity":"Legendary"},{"name":"Moon Ring","power":25,"rarity":"Rare"}]
 check(RULES.matching_gear_indices(gear,"moon","All","Power")==[1,2],"search and power sort retain original indices")
 check(RULES.matching_gear_indices(gear,"","Rare","Default")==[2],"rarity filter works")
 check(RULES.matching_gear_indices(gear,"","All","Name")==[1,2,0],"name sort works")
 check(RULES.page_count(0)==1 and RULES.page_count(101)==6,"pagination handles empty and large bags")
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"v30.5 game scene loads")
 if scene!=null:
  var game:Node3D=scene.instantiate()
  root.add_child(game)
  await process_frame
  check(game.get("bag_capacity")==100 and game.get("vault_items").is_empty(),"fresh game starts with expanded capacity")
  check(CURRENT_RELEASE.matches_project(game),"v30.5 entry-point version active")
  var profile:String=OS.get_environment("MOONFALL_TEST_PROFILE")
  var profile_normalized:String=profile.replace("\\","/").trim_suffix("/").to_lower()
  var data_normalized:String=ProjectSettings.globalize_path("user://").replace("\\","/").to_lower()
  if profile_normalized.is_empty() or not data_normalized.begins_with(profile_normalized+"/"):
   check(false,"integration requires isolated test profile")
  else:
   game.call("_start_game","z")
   game.set("gold",5000)
   game.call("_inventory_action","inventory_upgrade")
   check(game.get("bag_capacity")==150 and int(game.get("gold"))==4900,"bag upgrade costs gold and adds 50")
   var oldbag:Array=game.get("gear_items")
   oldbag.append({"name":"Keep Me","power":21,"rarity":"Rare"})
   game.call("_inventory_action","inventory_deposit:0")
   check(game.get("gear_items").is_empty() and game.get("vault_items").size()==1,"Atlas vault action moves an actual item")
   var saved:Dictionary=game.call("_read_save")
   check(int(saved.get("bag_capacity",0))==150 and (saved.get("vault",[]) as Array).size()==1,"bag capacity and vault persisted in schema-2 checkpoint")
   game.call("_inventory_action","inventory_withdraw:0")
   check(game.get("vault_items").is_empty() and game.get("gear_items").size()==1,"Atlas withdrawal retains item")
   game.call("_restore_data",saved)
   check(game.get("bag_capacity")==150 and game.get("gear_items").is_empty() and game.get("vault_items").size()==1,"checkpoint restores upgraded bag and stored gear")
   var legacy:Dictionary=saved.duplicate(true)
   legacy.erase("bag_capacity")
   legacy.erase("vault")
   legacy["gear"]=[{"name":"Old Item","power":11,"rarity":"Rare"}]
   game.call("_restore_data",legacy)
   check(game.get("bag_capacity")==100 and game.get("vault_items").is_empty() and game.get("gear_items").size()==1,"legacy checkpoint migrates without losing its equipment")
  game.queue_free()
 if errors==0:print("MOONFALL v30.5 INVENTORY EXPANSION PASS")
 quit(0 if errors==0 else 1)
