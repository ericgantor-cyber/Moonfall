extends "res://scripts/native_v30_7.gd"
## v30.8: working forge UI, atomic bulk repair, and polished, lightweight treasure visuals.
## Original artwork and schema-2 checkpoint paths are untouched.
const TREASURE_COLORS:Array[Color]=[
 Color("e6a95d"),Color("a0eac8"),Color("9db6ef"),
 Color("d7a2ed"),Color("e7bb7a"),Color("f38cbb")]
var v308_treasure_fx:Array[Dictionary]=[]
var v308_fx_clock:float=0.0

func _release_label()->String:
 return "v30.8"

func _build_scene()->void:
 super._build_scene()
 _build_treasure_fx()

func _build_treasure_fx()->void:
 # Visual-only nodes. No collision bodies or navigation-grid changes.
 for chest in treasure_chests:
  var zone:int=int(chest["zone"])
  var root_node:Node3D=chest["node"]
  var color:Color=TREASURE_COLORS[zone % TREASURE_COLORS.size()]
  var ring:=MeshInstance3D.new()
  ring.name="RunicCacheHalo"
  var torus:=TorusMesh.new()
  torus.inner_radius=0.78
  torus.outer_radius=0.85
  torus.rings=16
  torus.ring_segments=24
  ring.mesh=torus
  ring.position.y=0.15
  ring.material_override=_material(color,0.35,true)
  root_node.add_child(ring)
  var gem:=MeshInstance3D.new()
  gem.name="CacheCrystal"
  var crystal:=SphereMesh.new()
  crystal.radius=0.18
  crystal.height=0.34
  gem.mesh=crystal
  gem.position=Vector3(0.0,1.27,0.0)
  gem.material_override=_material(color,0.18,true)
  root_node.add_child(gem)
  var bands:=MeshInstance3D.new()
  bands.name="CacheMetalTrim"
  var trim:=BoxMesh.new()
  trim.size=Vector3(1.5,0.12,1.06)
  bands.mesh=trim
  bands.position.y=0.67
  bands.material_override=_material(Color("c2a67b"),0.35)
  root_node.add_child(bands)
  v308_treasure_fx.append({"halo":ring,"crystal":gem,"trim":bands,"zone":zone})
 _refresh_treasure_fx()

func _refresh_treasure_fx()->void:
 var detailed:bool=graphics_quality!="Smooth"
 for entry in v308_treasure_fx:
  var closed:bool=not (int(entry["zone"]) in opened_treasure_chests)
  var ring:MeshInstance3D=entry["halo"]
  var gem:MeshInstance3D=entry["crystal"]
  ring.visible=closed and detailed
  gem.visible=closed and detailed

func _apply_graphics_quality(mode:String)->void:
 super._apply_graphics_quality(mode)
 _refresh_treasure_fx()

func _process(delta:float)->void:
 super._process(delta)
 # Six chests maximum. Visual animation at 10 Hz; skip when no hero is playing.
 if not hero_playing or not is_instance_valid(player):return
 v308_fx_clock+=delta
 if v308_fx_clock<0.1:return
 v308_fx_clock=0.0
 for entry in v308_treasure_fx:
  var zone:int=int(entry["zone"])
  if zone in opened_treasure_chests:continue
  var chest:Dictionary=treasure_chests[zone]
  var root_node:Node3D=chest["node"]
  var distance_squared:float=player.global_position.distance_squared_to(root_node.global_position)
  if distance_squared>900.0:continue
  var halo:MeshInstance3D=entry["halo"]
  if halo.visible:halo.rotation.y+=0.09
  var crystal:MeshInstance3D=entry["crystal"]
  if crystal.visible:crystal.position.y=1.27+0.10*sin(map_time*2.5+float(zone))
  var label:Label3D=chest["label"]
  label.text="PRESS G • OPEN RARE CACHE" if distance_squared<=12.25 else "✦ RARE CACHE  ·  G"

func _refresh_treasure_chests()->void:
 super._refresh_treasure_chests()
 _refresh_treasure_fx()

func _atlas_execute(action:String)->void:
 # v30.7 rendered forge_ buttons, but the inherited Atlas dispatcher only
 # forwarded wallet_ actions. This restores real mouse/controller functionality.
 if action.begins_with("forge_"):
  _forge_action(action)
  if current_tab=="Wallet":_menu_tab("Wallet")
  return
 super._atlas_execute(action)

func _bag_repair_cost()->int:
 var total:int=0
 var seen:Dictionary={}
 for gear in gear_items:
  var uid:String=str(gear.get("uid",""))
  if uid!="" and seen.has(uid):continue
  total+=FORGE_RULES.repair_cost(gear)
  if uid!="":seen[uid]=true
 var equipped_uid:String=str(equipped.get("uid",""))
 if not equipped.is_empty() and (equipped_uid=="" or not seen.has(equipped_uid)):
  var already_in_bag:bool=false
  if equipped_uid=="":
   for gear in gear_items:
    if gear==equipped:
     already_in_bag=true
     break
  if not already_in_bag:total+=FORGE_RULES.repair_cost(equipped)
 return total

func _forge_action(action:String)->bool:
 if action!="forge_repair_all":return super._forge_action(action)
 if not _near_outpost_merchant():
  _show_message("Visit the outpost blacksmith when safe and dismounted.",2.0)
  return false
 var cost:int=_bag_repair_cost()
 if cost<=0:
  _show_message("All equipment is already in good condition.",2.0)
  return false
 if not _spend_coins(cost):
  _show_message("Not enough coins to repair all equipment.",2.0)
  return false
 for gear in gear_items:gear["durability"]=100
 if not equipped.is_empty():equipped["durability"]=100
 _show_message("Blacksmith repaired all equipment for "+MONEY_RULES.format_total(cost),3.0)
 _save_game()
 return true

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if which!="Wallet" or not _near_outpost_merchant():return
 var previous:String=menu_body.text
 # Parent adds a legacy forge appendix below the merchant. Replace that
 # section with an actionable summary at the top, not a duplicated appendix.
 var split_at:int=previous.find("\n[b]Hearthstone Blacksmith[/b]")
 if split_at>=0:previous=previous.substr(0,split_at)
 var wallet:int=MONEY_RULES.total(gold,silver,copper)
 var bulk:int=_bag_repair_cost()
 var summary:String="[color=#e8c88e][b]HEARTHSTONE FORGE • WORKBENCH[/b][/color]"
 summary+="\nAvailable coins: "+MONEY_RULES.format_total(wallet)
 if equipped.is_empty():
  summary+="\nEquip an item for repairs and enchantments."
 else:
  var repair:int=FORGE_RULES.repair_cost(equipped)
  var enchant:int=FORGE_RULES.enchant_cost(equipped)
  var current_rank:int=clampi(int(equipped.get("enchant_level",0)),0,FORGE_RULES.MAX_ENCHANT)
  summary+="\nEquipped: "+str(equipped.get("name","Gear"))
  summary+=" • Condition "+str(clampi(int(equipped.get("durability",100)),0,100))+"%"
  summary+=" • Enchant +"+str(current_rank)+"/"+str(FORGE_RULES.MAX_ENCHANT)
  if repair>0:summary+="\nSingle repair: "+MONEY_RULES.format_total(repair)+(" [color=#fa8a8a](insufficient coins)[/color]" if wallet<repair else "")
  if enchant>0:summary+="\nNext enchant: "+MONEY_RULES.format_total(enchant)+(" [color=#fa8a8a](insufficient coins)[/color]" if wallet<enchant else "")
 if bulk>0:
  _atlas_action("REPAIR ALL EQUIPMENT ("+MONEY_RULES.format_total(bulk)+")","forge_repair_all","One purchase repairs the entire bag and equipped gear; matching IDs are not charged twice.")
  summary+="\nRepair all: "+MONEY_RULES.format_total(bulk)+(" [color=#fa8a8a](insufficient coins)[/color]" if wallet<bulk else "")
 else:summary+="\nEverything is fully repaired."
 summary+="\nTreasure caches opened: "+str(opened_treasure_chests.size())+" / "+str(treasure_chests.size())
 menu_body.text=summary+"\n\n"+previous
 # Keep repair options visible before a long list of sellable bag items.
 for prefix in ["ENCHANT EQUIPPED","REPAIR EQUIPPED","REPAIR ALL EQUIPMENT"]:
  for child in menu_buttons.get_children():
   if child is Button and not child.is_queued_for_deletion() and child.text.begins_with(prefix):
    menu_buttons.move_child(child,0)
 for child in menu_buttons.get_children():
  if child is Button and not child.is_queued_for_deletion():
   var action_text:String=child.text
   if action_text.begins_with("REPAIR EQUIPPED"):
    child.disabled=equipped.is_empty() or wallet<FORGE_RULES.repair_cost(equipped)
   elif action_text.begins_with("ENCHANT EQUIPPED"):
    child.disabled=equipped.is_empty() or wallet<FORGE_RULES.enchant_cost(equipped)
   elif action_text.begins_with("REPAIR ALL EQUIPMENT"):
    child.disabled=wallet<bulk
