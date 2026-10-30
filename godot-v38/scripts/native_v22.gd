extends "res://scripts/native_v21.gd"
## v22: asset-backed gear icons, visual-only arena dressing, 8 species detail rigs,
## and 5 region-specific guardian headdresses. No save schema or hitbox changes.
const V22_GEAR_DIR := "res://assets/v22/gear/"
const V22_GEAR_TYPES := ["blade","shield","helm","boots","cloak","ring","charm","armor","staff","potion","rune","relic"]
var v22_beast_details: Array[Node3D] = []
var v22_guardian_headdresses: Array[Node3D] = []
var v22_arena_roots: Array[Node3D] = []
var v22_icon_textures: Dictionary = {}
var v22_visual_parts := 0
var v22_arenas := 0

func _ready() -> void:
 super._ready()
 _show_message("Moonfall v22: creatures, gear and rift chambers",4.0)

func _icon_category(gear_name: String) -> String:
 var name := gear_name.to_lower()
 if "boot" in name: return "boots"
 if "charm" in name or "amulet" in name: return "charm"
 if "band" in name or "ring" in name: return "ring"
 if "helm" in name or "cowl" in name or "hood" in name: return "helm"
 if "mantle" in name or "cloak" in name: return "cloak"
 if "vest" in name or "jerkin" in name or "armor" in name or "jacket" in name: return "armor"
 if "sword" in name or "blade" in name or "axe" in name or "fang" in name: return "blade"
 if "shield" in name or "ward" in name: return "shield"
 if "staff" in name or "wand" in name: return "staff"
 if "draught" in name or "potion" in name or "oil" in name: return "potion"
 if "rune" in name or "sigil" in name: return "rune"
 return "relic"

func _gear_icon(gear_name: String) -> Texture2D:
 var cat := _icon_category(gear_name)
 if not v22_icon_textures.has(cat):
  v22_icon_textures[cat] = load(V22_GEAR_DIR + cat + ".png")
 return v22_icon_textures[cat]

func _spawn_beast(at: Vector3, kind: String, zone: int, variant: String="normal", boss: bool=false) -> void:
 super._spawn_beast(at,kind,zone,variant,boss)
 # Detail follows the existing animated rig. No new actors or collision shapes.
 var rig:Node3D = visual_rigs[-1]
 var details := Node3D.new()
 details.name = "V22_" + kind.capitalize() + "_Details"
 rig.add_child(details)
 v22_beast_details.append(details)
 var accent := Color(str(REGIONS[zone]["light"]))
 var ivory := Color("d8c6a7")
 var dark := Color("312b42")
 var copper := Color("ad7a56")
 match kind:
  "wolf":
   for z in [-0.29,0.29]:
    _form(details,"cone",Vector3(0.42,0.80,z),Vector3(0.26,0.75,0.26),dark,false,Vector3(0,0,18))
    _form(details,"cone",Vector3(0.90,-0.12,z),Vector3(0.18,0.48,0.18),ivory,false,Vector3(0,0,-68))
   for i in range(4): _form(details,"cone",Vector3(-0.40+0.24*float(i),0.60,0),Vector3(0.18,0.32,0.18),accent.darkened(0.45))
  "stag":
   for side in [-1.0,1.0]:
    _form(details,"cylinder",Vector3(0.60,1.35,side*0.31),Vector3(0.09,0.90,0.09),ivory,false,Vector3(0,0,side*22))
    _form(details,"cone",Vector3(0.94,1.53,side*0.47),Vector3(0.15,0.68,0.15),ivory,false,Vector3(0,0,side*35))
   _form(details,"ring",Vector3(-0.12,0.43,0),Vector3(0.64,0.09,0.64),accent,true,Vector3(75,0,0))
  "boar":
   for side in [-1.0,1.0]:
    _form(details,"cone",Vector3(1.16,-0.06,side*0.32),Vector3(0.23,0.70,0.23),ivory,false,Vector3(0,0,-58))
    _form(details,"box",Vector3(-0.11,0.56,side*0.42),Vector3(0.85,0.20,0.14),dark)
   for i in range(3):_form(details,"cone",Vector3(-0.52+0.35*float(i),0.76,0),Vector3(0.20,0.56,0.20),copper)
  "fox":
   for side in [-1.0,1.0]:
    _form(details,"cone",Vector3(0.53,0.84,side*0.27),Vector3(0.28,0.64,0.28),copper)
    _form(details,"sphere",Vector3(0.91,-0.02,side*0.25),Vector3(0.15,0.12,0.15),dark)
   _form(details,"cone",Vector3(-1.02,-0.03,0),Vector3(0.38,1.14,0.38),ivory,false,Vector3(0,0,-74))
  "serpent":
   _form(details,"sphere",Vector3(0.83,0.12,0),Vector3(0.82,0.27,0.72),Color("346e68"))
   for side in [-1.0,1.0]:
    _form(details,"sphere",Vector3(0.98,0.25,side*0.26),Vector3(0.20,0.13,0.14),accent,true)
    _form(details,"cone",Vector3(0.46,0.42,side*0.23),Vector3(0.23,0.62,0.23),ivory,false,Vector3(0,0,side*25))
   for i in range(5):_form(details,"box",Vector3(-0.58+0.25*float(i),-0.10,0.35),Vector3(0.12,0.08,0.38),copper)
  "panther":
   for side in [-1.0,1.0]:
    _form(details,"cone",Vector3(0.70,0.56,side*0.28),Vector3(0.25,0.52,0.25),dark)
    _form(details,"sphere",Vector3(0.97,0.19,side*0.27),Vector3(0.19,0.10,0.12),accent,true)
   for i in range(4):_form(details,"box",Vector3(-0.56+0.36*float(i),0.38,0),Vector3(0.10,0.14,0.50),Color("61507b"))
  "elk":
   for side in [-1.0,1.0]:
    _form(details,"cylinder",Vector3(0.53,1.44,side*0.43),Vector3(0.14,1.17,0.14),ivory,false,Vector3(0,0,side*19))
    for j in range(2):_form(details,"cone",Vector3(0.64+0.27*float(j),1.77+0.28*float(j),side*(0.39+0.16*float(j))),Vector3(0.20,0.52,0.20),ivory)
   _form(details,"box",Vector3(-0.12,0.46,0),Vector3(1.2,0.15,0.87),copper)
  "owl":
   for side in [-1.0,1.0]:
    _form(details,"cone",Vector3(0.43,0.97,side*0.35),Vector3(0.23,0.70,0.23),dark)
    _form(details,"sphere",Vector3(0.63,0.48,side*0.17),Vector3(0.27,0.26,0.27),accent,true)
    for i in range(3):_form(details,"cone",Vector3(-0.20+0.24*float(i),-0.1,side*(0.64+0.14*float(i))),Vector3(0.28,0.65,0.28),copper,false,Vector3(0,0,side*24))
 v22_visual_parts += details.get_child_count()
 if boss:
  var crown := Node3D.new()
  crown.name="V22_GuardianHeaddress_"+str(zone)
  rig.add_child(crown)
  v22_guardian_headdresses.append(crown)
  # Five geographically distinct silhouettes, not copies of the same crown.
  match zone:
   1:
    for side in [-1.0,1.0]:_form(crown,"cone",Vector3(-0.04,1.76,side*0.7),Vector3(0.42,1.70,0.42),accent,true,Vector3(0,0,side*28))
    _form(crown,"sphere",Vector3(0.22,1.44,0),Vector3(0.45,0.45,0.45),Color("8463d4"),true)
   2:
    for side in [-1.0,1.0]:_form(crown,"ring",Vector3(0.0,1.50,side*0.57),Vector3(0.92,0.16,0.92),accent,true,Vector3(0,0,52))
    _form(crown,"sphere",Vector3(0,1.65,0),Vector3(0.32,0.45,0.32),accent,true)
   3:
    for i in range(3):_form(crown,"cone",Vector3(0.14+0.34*float(i-1),1.75,0),Vector3(0.30,1.25+float(i%2)*0.45,0.30),Color("dfbd8a"),true)
   4:
    _form(crown,"ring",Vector3(0.0,1.65,0),Vector3(1.25,0.13,1.25),accent,true,Vector3(35,0,23))
    for i in range(4):_form(crown,"box",Vector3(float(i-2)*0.33,1.76,0),Vector3(0.15,0.6,0.20),Color("9775cd"))
   5:
    for side in [-1.0,1.0]:_form(crown,"cone",Vector3(-0.1,2.0,side*0.60),Vector3(0.55,1.90,0.55),Color("a64267"),true,Vector3(0,0,side*25))
    _form(crown,"sphere",Vector3(0.1,1.75,0),Vector3(0.46,0.46,0.46),Color("ef547c"),true)
  v22_visual_parts += crown.get_child_count()

func _build_rift_encounters() -> void:
 super._build_rift_encounters()
 for site_index in range(rifts.size()):
  var rift: Dictionary = rifts[site_index]
  var zone: int = int(rift["zone"])
  var root := Node3D.new()
  root.name="V22_RiftChamber_"+str(site_index)
  root.position = rift["anchor"]
  decorative_groups[zone].add_child(root)
  v22_arena_roots.append(root)
  var accent := Color(str(REGIONS[zone]["light"]))
  var stone := Color("52475e") if site_index==1 else Color("46383d") if site_index==2 else Color("3d524b")
  _form(root,"ring",Vector3(0,0.05,0),Vector3(7.5,0.11,7.5),accent.darkened(0.35),true)
  # Open arena arches outside the combat path, with no collision or blocking tiles.
  for j in range(6):
   var angle:float = float(j)*TAU/6.0
   var spot := Vector3(cos(angle)*9.2,0,sin(angle)*9.2)
   var gate := Node3D.new()
   gate.name="V22_Arch_%d" % j
   gate.position=spot
   gate.rotation.y=-angle
   root.add_child(gate)
   for side in [-1.0,1.0]:
    _form(gate,"cylinder",Vector3(side*1.15,1.60,0),Vector3(0.54,3.2,0.54),stone)
    _form(gate,"cone",Vector3(side*1.15,3.43,0),Vector3(0.70,0.9,0.70),accent,true)
   _form(gate,"box",Vector3(0,3.26,0),Vector3(3.05,0.52,0.75),stone.lightened(0.1))
   if site_index==0:
    _form(gate,"cone",Vector3(0,3.76,0),Vector3(1.3,1.5,1.3),Color("405d48"))
   elif site_index==1:
    _form(gate,"ring",Vector3(0,2.15,0),Vector3(0.62,0.10,0.62),accent,true,Vector3(90,0,0))
   else:
    _form(gate,"sphere",Vector3(0,3.60,0),Vector3(0.40,0.48,0.40),Color("ca3b65"),true)
  # Three authored interior motifs: roots, relic dais, or braziers.
  for i in range(8):
   var a:float = TAU*float(i)/8.0
   var pos := Vector3(cos(a)*6.9,0,sin(a)*6.9)
   match site_index:
    0:
     _form(root,"cone",pos+Vector3(0,0.65,0),Vector3(0.55,1.35,0.55),Color("557665"))
    1:
     _form(root,"box",pos+Vector3(0,0.32,0),Vector3(0.88,0.63,0.85),Color("746b8f"),false,Vector3(0,float(i)*21.0,13))
    2:
     _form(root,"cone",pos+Vector3(0,0.78,0),Vector3(0.65,1.56,0.65),Color("8a315a"))
  v22_arenas+=1

func _create_gear_drop(at:Vector3,kind:String,variant:String,zone:int,existing:Dictionary={}) -> void:
 var old_count := gear_drops.size()
 super._create_gear_drop(at,kind,variant,zone,existing)
 if gear_drops.size()<=old_count:return
 var drop:Dictionary=gear_drops[-1]
 var item:Dictionary=drop["data"]
 var icon := Sprite3D.new()
 icon.name="V22_LootIcon_"+_icon_category(str(item.get("name","")))
 icon.texture=_gear_icon(str(item.get("name","")))
 icon.billboard=BaseMaterial3D.BILLBOARD_ENABLED
 icon.pixel_size=0.0055
 icon.position=Vector3(0,1.58,0)
 icon.shaded=false
 (drop["node"] as Node3D).add_child(icon)

func _menu_tab(which:String) -> void:
 super._menu_tab(which)
 if which=="Inventory":
  var selected_name := str(equipped.get("name","Relic"))
  if equipped.is_empty() and not gear_items.is_empty():selected_name=str(gear_items[0].get("name","Relic"))
  beast_guide.texture=_gear_icon(selected_name)
  var index := 0
  for child in menu_buttons.get_children():
   if child is Button and not child.is_queued_for_deletion() and child.text.begins_with("EQUIP:"):
    if index < gear_items.size():
     child.icon=_gear_icon(str(gear_items[index].get("name","")))
     child.icon_alignment=HORIZONTAL_ALIGNMENT_LEFT
     child.expand_icon=false
     index+=1
  menu_body.text += "\n[b]V22 EQUIPMENT ART:[/b] Item icons appear beside the equip buttons and on gear drops."
 elif which=="Bestiary":
  menu_body.text=menu_body.text.replace("simple native 3D placeholders in-world","distinct 3D anatomy and species detailing in-world")

func _apply_graphics_quality(mode:String) -> void:
 super._apply_graphics_quality(mode)
 for n in v22_beast_details:
  if is_instance_valid(n):n.visible=(graphics_quality!="Smooth")
 for n in v22_guardian_headdresses:
  if is_instance_valid(n):n.visible=(graphics_quality!="Smooth")
