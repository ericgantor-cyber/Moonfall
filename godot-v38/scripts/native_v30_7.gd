extends "res://scripts/native_v30_6.gd"
## v30.7: one-use regional treasure caches, roaming coin carriers, and outpost forging.
## All new checkpoint data is optional inside the existing schema-2 save.
const FORGE_RULES:=preload("res://scripts/systems/forge_rules.gd")
var treasure_chests:Array[Dictionary]=[]
var treasure_carriers_by_zone:Dictionary={}
var next_gear_uid:int=1

func _release_label()->String:
 return "v30.7"

func _build_scene()->void:
 super._build_scene()
 _build_treasure_chests()

func _build_treasure_chests()->void:
 for zone in range(REGIONS.size()):
  var coord:Vector2=REGIONS[zone]["at"]
  var root_node:=Node3D.new()
  root_node.name="MoonfallRareCache_"+str(zone)
  root_node.position=Vector3(coord.x+9.0,0.0,coord.y-8.0)
  world.add_child(root_node)
  var body:=MeshInstance3D.new()
  var mesh:=BoxMesh.new()
  mesh.size=Vector3(1.25,0.8,0.88)
  body.mesh=mesh
  body.position.y=0.43
  body.material_override=_material(Color("473023"))
  root_node.add_child(body)
  var lid:=MeshInstance3D.new()
  var top:=BoxMesh.new()
  top.size=Vector3(1.38,0.23,0.96)
  lid.mesh=top
  lid.position.y=0.96
  lid.material_override=_material(Color("b98b4c"),0.4,true)
  root_node.add_child(lid)
  var label:=Label3D.new()
  label.text="✦ RARE CACHE  ·  G"
  label.position=Vector3(0,1.8,0)
  label.billboard=BaseMaterial3D.BILLBOARD_ENABLED
  label.font_size=22
  label.pixel_size=0.005
  label.modulate=Color("ffe19a")
  root_node.add_child(label)
  treasure_chests.append({"node":root_node,"lid":lid,"label":label,"zone":zone})

func _refresh_treasure_chests()->void:
 for chest in treasure_chests:
  var opened:bool=int(chest["zone"]) in opened_treasure_chests
  var lid:MeshInstance3D=chest["lid"]
  lid.position.y=0.57 if opened else 0.96
  var text_label:Label3D=chest["label"]
  text_label.text="OPENED CACHE" if opened else "✦ RARE CACHE  ·  G"
  text_label.modulate=Color("877f73") if opened else Color("ffe19a")

func _open_treasure_chest(zone:int)->bool:
 if not hero_playing or mounted or _combat_threat():
  _show_message("You must be safe and dismounted to open a treasure cache.",2)
  return false
 if zone<0 or zone>=treasure_chests.size() or zone in opened_treasure_chests:return false
 var chest:Dictionary=treasure_chests[zone]
 if player.global_position.distance_to((chest["node"] as Node3D).global_position)>3.5:return false
 if gear_items.size()>=bag_capacity and vault_items.size()>=INVENTORY_RULES.VAULT_LIMIT:
  _show_message("Your bag and vault are full. Cache remains sealed.",2.5)
  return false
 var reward:Dictionary=FORGE_RULES.chest_reward(zone)
 var item:Dictionary=_safe_item(reward["gear"])
 if gear_items.size()<bag_capacity:gear_items.append(item)
 else:vault_items.append(item)
 _add_coins(int(reward["coins"]))
 _gain_material(str(reward["material"]),2)
 opened_treasure_chests.append(zone)
 _refresh_treasure_chests()
 _show_message("RARE CACHE! "+str(item["name"])+" and "+MONEY_RULES.format_total(int(reward["coins"])),3.5)
 _save_game()
 return true

func _interact()->void:
 if hero_playing and not mounted:
  for chest in treasure_chests:
   if int(chest["zone"]) in opened_treasure_chests:continue
   if player.global_position.distance_to((chest["node"] as Node3D).global_position)<=3.5:
    _open_treasure_chest(int(chest["zone"]))
    return
 super._interact()

func _spawn_beast(at:Vector3,kind:String,zone:int,variant:String="normal",boss:bool=false)->void:
 super._spawn_beast(at,kind,zone,variant,boss)
 if beasts.is_empty() or boss or not bool(BEASTS.get(kind,{}).get("hostile",false)) or treasure_carriers_by_zone.has(zone):return
 treasure_carriers_by_zone[zone]=true
 var creature:Dictionary=beasts[-1]
 creature["treasure_carrier"]=true
 var plate:Label3D=creature.get("nameplate")
 if is_instance_valid(plate):plate.text="✦ TREASURE BEARER ✦  "+plate.text
 var glow:=OmniLight3D.new()
 glow.name="TreasureBearerGlimmer"
 glow.omni_range=3.5
 glow.light_color=Color("e8b65d")
 glow.light_energy=0.65
 (creature["body"] as CharacterBody3D).add_child(glow)

func _defeat_beast(beast:Dictionary,pet_kill:bool)->void:
 if not bool(beast.get("alive",false)):return
 var treasure:bool=bool(beast.get("treasure_carrier",false))
 var zone:int=int(beast.get("zone",0))
 var at:Vector3=(beast["body"] as Node3D).global_position
 var variant:String=str(beast.get("variant","normal"))
 super._defeat_beast(beast,pet_kill)
 if treasure:
  var prize:int=FORGE_RULES.carrier_reward(zone,variant)
  _create_coin_drop(at,prize)
  _create_gear_drop(at,"Treasure Bearer","moon",zone)
  _show_message("TREASURE BEARER! Bonus coins and epic equipment.",2.5)
  # The base kill can save first, so commit the bonus after both rewards exist.
  _save_game()

func _equip(index:int)->void:
 if index<0 or index>=gear_items.size():return
 if not gear_items[index].has("uid"):
  gear_items[index]["uid"]="moonfall-"+str(next_gear_uid)
  next_gear_uid+=1
 super._equip(index)

func _sync_equipped_to_bag()->void:
 var uid:String=str(equipped.get("uid",""))
 if uid=="":return
 for i in range(gear_items.size()):
  if str(gear_items[i].get("uid",""))==uid:
   gear_items[i]=equipped.duplicate(true)
   return

func _apply_incoming_damage(raw:float)->void:
 var old_hp:float=hp
 super._apply_incoming_damage(raw)
 if hp<old_hp and not equipped.is_empty():
  equipped["durability"]=maxi(0,int(equipped.get("durability",100))-1)
  _sync_equipped_to_bag()

func _equipment_power()->int:
 if not equipped.is_empty() and int(equipped.get("durability",100))<=0:return 0
 return super._equipment_power()

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if which!="Wallet" or not _near_outpost_merchant():return
 menu_body.text+="
[b]Hearthstone Blacksmith[/b]
Repairs restore condition. Enchanting adds power and can grant a magical affix (maximum 5 ranks).
"
 if not equipped.is_empty():
  var repair:int=FORGE_RULES.repair_cost(equipped)
  var enchant:int=FORGE_RULES.enchant_cost(equipped)
  menu_body.text+="Equipped: "+str(equipped.get("name","Gear"))+" · Condition "+str(equipped.get("durability",100))+"/100 · Enchant +"+str(equipped.get("enchant_level",0))+"
"
  if repair>0:_atlas_action("REPAIR EQUIPPED ("+MONEY_RULES.format_total(repair)+")","forge_repair")
  if enchant>0:_atlas_action("ENCHANT EQUIPPED ("+MONEY_RULES.format_total(enchant)+")","forge_enchant")
 if equipped.is_empty():menu_body.text+="Equip an item before visiting the blacksmith.
"
 menu_body.text+="
[b]World Treasure[/b]
Opened rare caches: "+str(opened_treasure_chests.size())+" / "+str(treasure_chests.size())+". Every region hides a permanent cache; treasure-bearing monsters return when they respawn."

func _wallet_action(action:String)->void:
 if action.begins_with("forge_"):
  _forge_action(action)
 else:super._wallet_action(action)

func _forge_action(action:String)->bool:
 if not _near_outpost_merchant() or equipped.is_empty():
  _show_message("Visit Hearthstone's blacksmith with gear equipped.",2)
  return false
 var repaired:bool=action=="forge_repair"
 var enchanted:bool=action=="forge_enchant"
 if not repaired and not enchanted:return false
 var cost:int=FORGE_RULES.repair_cost(equipped) if repaired else FORGE_RULES.enchant_cost(equipped)
 if cost<=0:
  _show_message("Item is already fully repaired or enchanted.",2)
  return false
 if not _spend_coins(cost):
  _show_message("Not enough coins for blacksmith services.",2)
  return false
 if repaired:equipped["durability"]=100
 else:equipped=FORGE_RULES.enchanted_copy(equipped,AFFIXES)
 _sync_equipped_to_bag()
 _show_message(("Repaired " if repaired else "Enchanted ")+str(equipped.get("name","Gear"))+" for "+MONEY_RULES.format_total(cost),3)
 _save_game()
 return true

func _reset_new_adventure()->void:
 super._reset_new_adventure()
 opened_treasure_chests.clear()
 next_gear_uid=1
 _refresh_treasure_chests()

func _restore_data(data:Dictionary)->void:
 super._restore_data(data)
 opened_treasure_chests.clear()
 var saved:Variant=data.get("opened_treasure_chests",[])
 if saved is Array:
  for raw in saved:
   var zone:int=int(raw)
   if zone>=0 and zone<treasure_chests.size() and not (zone in opened_treasure_chests):opened_treasure_chests.append(zone)
 _refresh_treasure_chests()
 # Allocate new identifiers above those already present; never collide after a load.
 next_gear_uid=1
 for item in gear_items + vault_items + [equipped]:
  var uid:String=str(item.get("uid",""))
  if uid.begins_with("moonfall-") and uid.substr(9).is_valid_int():next_gear_uid=maxi(next_gear_uid,int(uid.substr(9))+1)
