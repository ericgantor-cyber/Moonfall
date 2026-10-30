extends SceneTree
const CURRENT_RELEASE = preload("res://tests/current_release.gd")
const FORGE:=preload("res://scripts/systems/forge_rules.gd")
const MONEY:=preload("res://scripts/systems/money_rules.gd")
const GEAR:=preload("res://scripts/systems/gear_codec.gd")
var failures:int=0
func check(ok:bool,what:String)->void:
 if ok:print("PASS v30.7: "+what)
 else:
  failures+=1
  push_error("FAIL v30.7: "+what)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 check(FORGE.repair_cost({"durability":100})==0,"full condition has no repair charge")
 check(FORGE.repair_cost({"durability":20,"rarity":"Rare","power":20})>0,"damaged gear has bounded repair cost")
 var item:Dictionary={"name":"Test Relic","rarity":"Rare","power":25,"durability":72,"enchant_level":0,"uid":"test-1"}
 var new_item:Dictionary=FORGE.enchanted_copy(item,["Fury","Haste"])
 check(int(new_item.get("enchant_level",0))==1 and int(new_item.get("power",0))>25,"enchant raises level and power")
 check(int(item.get("enchant_level",0))==0,"forge does not mutate original item")
 check(GEAR.sanitize(new_item,["Fury","Haste"]).get("affix","")!="","enchanted rare affix persists through gear codec")
 new_item["enchant_level"]=5
 check(FORGE.enchant_cost(new_item)==0 and int(FORGE.enchanted_copy(new_item,[])["enchant_level"])==5,"rank five is hard cap")
 check(FORGE.chest_reward(5)["coins"]>FORGE.chest_reward(0)["coins"],"higher region caches award more")
 check(FORGE.carrier_reward(5,"normal")>FORGE.carrier_reward(0,"normal"),"carriers scale with region")
 var profile:String=OS.get_environment("MOONFALL_TEST_PROFILE")
 var profile_normalized:String=profile.replace("\\","/").trim_suffix("/").to_lower()
 var data_normalized:String=ProjectSettings.globalize_path("user://").replace("\\","/").to_lower()
 if profile_normalized.is_empty() or not data_normalized.begins_with(profile_normalized+"/"):
  check(false,"requires isolated save profile")
  quit(1)
  return
 var packed:PackedScene=load("res://scenes/main.tscn")
 check(packed!=null,"game scene parses")
 if packed==null:quit(1);return
 var game:Node3D=packed.instantiate()
 root.add_child(game)
 await process_frame
 check(CURRENT_RELEASE.matches_project(game),"v30.7 scene active")
 check(game.get("treasure_chests").size()==6,"six caches generated")
 var carriers:int=0
 for beast in game.get("beasts"):
  if bool(beast.get("treasure_carrier",false)):carriers+=1
 check(carriers==6,"exactly one treasure carrier per region")
 game.call("_start_game","z")
 # Isolate economy from active monster aggro; gameplay deliberately blocks trading in combat.
 for creature in game.get("beasts"):
  creature["hostile"]=false
 var coins_before:int=MONEY.total(game.get("gold"),game.get("silver"),game.get("copper"))
 var chest:Dictionary=game.get("treasure_chests")[0]
 game.get("player").global_position=(chest["node"] as Node3D).global_position+Vector3(1.0,0.85,0.0)
 check(game.call("_open_treasure_chest",0),"nearby cache opens")
 check(game.get("opened_treasure_chests").size()==1,"cache records completion")
 check(game.get("gear_items").size()==1,"cache grants gear")
 check(MONEY.total(game.get("gold"),game.get("silver"),game.get("copper"))>coins_before,"cache grants coins")
 check(not game.call("_open_treasure_chest",0),"cache cannot be looted twice")
 game.call("_save_game")
 var saved:Dictionary=game.call("_read_save")
 check(saved.has("opened_treasure_chests") and (saved["opened_treasure_chests"] as Array).size()==1 and int(saved["opened_treasure_chests"][0])==0,"cache state persisted in schema two")
 game.call("_restore_data",saved)
 check(game.get("opened_treasure_chests")==[0] and (game.get("treasure_chests")[0]["label"] as Label3D).text=="OPENED CACHE","cache restored as opened")
 var legacy:Dictionary=saved.duplicate(true)
 legacy.erase("opened_treasure_chests")
 game.call("_restore_data",legacy)
 check(game.get("opened_treasure_chests").is_empty(),"older saves treat caches as unopened")
 # Move to the outpost for forging and equip the cache item.
 var shrine:Node3D=game.get("region_shrines")[0]
 game.get("player").global_position=shrine.global_position+Vector3(0,0.85,3)
 game.call("_equip",0)
 var equipped:Dictionary=game.get("equipped")
 check(equipped.has("uid") and equipped.get("durability",100)==100,"equipped items obtain stable identifiers")
 game.set("gold",300)
 game.call("_apply_incoming_damage",4.0)
 equipped=game.get("equipped")
 check(int(equipped.get("durability",100))==99,"taking damage wears equipped gear")
 var damaged:Dictionary=game.get("gear_items")[0]
 check(int(damaged.get("durability",100))==99,"wear syncs to bag item")
 var cost:int=FORGE.repair_cost(equipped)
 var before:int=MONEY.total(game.get("gold"),game.get("silver"),game.get("copper"))
 check(game.call("_forge_action","forge_repair"),"blacksmith repairs equipped gear")
 check(int(game.get("equipped").get("durability",0))==100 and MONEY.total(game.get("gold"),game.get("silver"),game.get("copper"))==before-cost,"repair restores condition and charges once")
 var level_before:int=int(game.get("equipped").get("enchant_level",0))
 check(game.call("_forge_action","forge_enchant"),"blacksmith enchants equipped gear")
 check(int(game.get("equipped").get("enchant_level",0))==level_before+1,"enchant rank increases")
 game.call("_save_game")
 saved=game.call("_read_save")
 game.call("_restore_data",saved)
 check(int(game.get("equipped").get("enchant_level",0))==level_before+1,"enchant persists after load")
 # Treasure bearer rewards come only once per creature defeat.
 var carrier:Dictionary={}
 for creature in game.get("beasts"):
  if bool(creature.get("treasure_carrier",false)):
   carrier=creature
   break
 check(not carrier.is_empty(),"treasure bearer is available for real combat")
 if not carrier.is_empty():
  var coin_count:int=game.get("coin_drops").size()
  var gear_count:int=game.get("gear_drops").size()
  game.call("_defeat_beast",carrier,false)
  check(game.get("gear_drops").size()>gear_count,"defeated treasure bearer creates extra gear drops")
  check(game.get("coin_drops").size()>coin_count,"defeated treasure bearer creates extra coin drop")
  var once:int=game.get("gear_drops").size()
  game.call("_defeat_beast",carrier,false)
  check(game.get("gear_drops").size()==once,"same death cannot award twice")
 game.call("_start_game","nyra")
 check(game.get("opened_treasure_chests").is_empty() and game.get("equipped").is_empty(),"new adventure resets caches and gear")
 game.queue_free()
 if failures==0:print("MOONFALL v30.7 FORGE & TREASURE PASS")
 quit(0 if failures==0 else 1)
