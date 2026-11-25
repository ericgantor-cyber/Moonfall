extends SceneTree
const STOCK:=preload("res://scripts/systems/inventory_rules.gd")
var failures:int=0

func check(ok:bool,description:String)->void:
 if ok:print("PASS v30.4: "+description)
 else:
  failures+=1
  push_error("FAIL v30.4: "+description)

func _initialize()->void:call_deferred("_run")

func _run()->void:
 var supplies:Dictionary={"Leather":3,"fang":1}
 STOCK.add_material(supplies,"Leather",999999)
 check(supplies["Leather"]==99999,"material gains saturate instead of overflowing")
 STOCK.add_material(supplies,"Leather",-999999)
 check(supplies["Leather"]==0,"material losses cannot create negative stock")
 check(not STOCK.consume_one(supplies,"Leather"),"cannot consume missing material")
 STOCK.add_material(supplies,"Leather",3)
 var recipe:Dictionary={"name":"Leather Jerkin","cost":{"Leather":3,"fang":1},"result":"Leather Jerkin","power":10}
 check(STOCK.craft_issue(recipe,supplies,0).is_empty(),"valid equipment recipe accepted")
 var short_supplies:Dictionary={"Leather":2,"fang":1}
 var before:Dictionary=short_supplies.duplicate(true)
 check(STOCK.craft_issue(recipe,short_supplies,0).begins_with("Missing Leather"),"missing ingredients reported")
 check(short_supplies==before,"failed crafting does not spend materials")
 check(STOCK.craft_issue(recipe,supplies,100)=="Bag full.","full bag blocks equipment crafting")
 var supplies_before_invalid:Dictionary=supplies.duplicate(true)
 for wrong_cost in [-1,0,100000]:
  var wrong_recipe:Dictionary={"name":"Broken recipe","cost":{"Leather":wrong_cost},"result":"Broken","power":1}
  check(STOCK.craft_issue(wrong_recipe,supplies,0)=="Invalid recipe cost.","Malformed recipe cost is rejected")
 check(supplies==supplies_before_invalid,"Invalid crafting requests never modify materials")

 var crafted:Dictionary=STOCK.craft_commit(recipe,supplies)
 check(crafted["gear"]["rarity"]=="Crafted" and crafted["gear"]["power"]==10,"crafted equipment remains compatible")
 check(supplies["Leather"]==0 and supplies["fang"]==0,"crafting consumes exact recipe costs")
 var potion:Dictionary={"name":"Healing Draught","cost":{"herb":2},"result":"Healing Draught","power":0}
 var herbs:Dictionary={"herb":3}
 check(STOCK.craft_issue(potion,herbs,100).is_empty(),"full bag does not block consumable crafting")
 var result:Dictionary=STOCK.craft_commit(potion,herbs)
 STOCK.add_material(herbs,str(result["material"]),1)
 check(herbs["herb"]==1 and herbs["Healing Draught"]==1,"consumable crafting preserves materials and result")
 check(STOCK.consume_one(herbs,"Healing Draught") and herbs["Healing Draught"]==0,"consumable is removed exactly once")
 check(STOCK.can_store_gear(99) and not STOCK.can_store_gear(100),"starting equipment limit is 100")
 var gear:Array=[{"name":"Original","power":999,"rarity":"Rare"}]
 var selected:Dictionary=STOCK.equip_copy(gear,0)
 selected["name"]="Changed"
 check(gear[0]["name"]=="Original" and STOCK.equipment_power(selected)==250,"equipping copies gear and clamps power")
 check(STOCK.equip_copy(gear,999).is_empty(),"invalid equipment selection is safe")
 var source_gear:Array=[{"name":"Guarded","affixes":[{"stat":"Power","value":4}]},{"name":"Second"}]
 var destination_gear:Array=[]
 check(not STOCK.transfer_gear(source_gear,source_gear,0,100),"same-container transfers are rejected without deleting gear")
 check(source_gear.size()==2,"rejected self-transfer preserves all gear")
 check(not STOCK.transfer_gear(source_gear,destination_gear,0,0),"zero-capacity transfer is rejected")
 check(not STOCK.transfer_gear(source_gear,destination_gear,0,1001),"oversized capacity cannot bypass vault limit")
 check(not STOCK.transfer_gear(source_gear,destination_gear,-1,100),"negative index cannot transfer gear")
 check(source_gear.size()==2 and destination_gear.is_empty(),"invalid transfers preserve both containers")
 check(STOCK.transfer_gear(source_gear,destination_gear,0,100),"valid gear transfer succeeds")
 check(source_gear.size()==1 and destination_gear.size()==1,"successful transfer moves exactly one item")
 destination_gear[0]["affixes"][0]["value"]=99
 check(source_gear[0]["name"]=="Second","transferred gear retains independent nested data")
 var full_gear:Array=[{"name":"Already stored"}]
 check(not STOCK.transfer_gear(source_gear,full_gear,0,1),"full destination rejects transfer")
 check(source_gear.size()==1 and full_gear.size()==1,"full destination does not lose gear")
 var bad_gear:Array=[{}]
 check(not STOCK.transfer_gear(bad_gear,destination_gear,0,100),"empty gear record is rejected")
 check(bad_gear.size()==1,"invalid gear is not removed from source")
 var loot:Array=[{"data":{"rarity":"Legendary"}},{"data":{"rarity":"Uncommon"}},{"data":{"rarity":"Common"}}]
 check(STOCK.first_discardable_drop(loot)==1,"lowest eligible ground drop evicted first")
 check(STOCK.first_discardable_drop([{"data":{"rarity":"Legendary"}}])==-1,"legendary-only ground loot is protected")
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"main scene loads with modular inventory")
 if scene!=null:
  var game:Node3D=scene.instantiate()
  root.add_child(game)
  await process_frame
  game.set("materials",{"herb":99998})
  game.call("_gain_material","herb",12)
  check(int(game.get("materials")["herb"])==99999,"legacy material wrapper delegates to module")
  game.set("equipped",{"name":"Test","power":999})
  check(int(game.call("_equipment_power"))==250,"legacy equipment wrapper delegates to module")
  var profile:String=OS.get_environment("MOONFALL_TEST_PROFILE")
  var profile_normalized:String=profile.replace("\\","/").trim_suffix("/").to_lower()
  var data_normalized:String=ProjectSettings.globalize_path("user://").replace("\\","/").to_lower()
  if profile_normalized.is_empty() or not data_normalized.begins_with(profile_normalized+"/"):
   check(false,"integration tests require isolated save profile")
  else:
   game.call("_start_game","z")
   var player:CharacterBody3D=game.get("player")
   player.global_position=Vector3(300,0.85,300)
   game.set("materials",{"Leather":3,"fang":1,"Healing Draught":1})
   var prior_bag:int=game.get("gear_items").size()
   game.call("_craft",0)
   check(game.get("gear_items").size()==prior_bag+1,"old crafting UI creates an item through new module")
   check(int(game.get("materials").get("Leather",-1))==0,"old crafting UI spends exact materials")
   game.call("_equip",prior_bag)
   check(str(game.get("equipped").get("name",""))=="Leather Jerkin","old equip action selects crafted item")
   game.set("hp",10.0)
   game.call("_use_consumable","Healing Draught")
   check(int(game.get("materials").get("Healing Draught",-1))==0 and float(game.get("hp"))>10.0,"old consumable action heals and consumes once")
  game.queue_free()
 if failures==0:print("MOONFALL v30.4 INVENTORY RULES PASS")
 quit(0 if failures==0 else 1)
