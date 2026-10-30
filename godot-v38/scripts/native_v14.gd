extends "res://scripts/native_v11.gd"
## Moonfall v14: code-generated low-poly fauna, landmarks, VFX and dark-fantasy UI.
## All original hero and beast textures remain untouched. No additional game modes.

const VISUAL_SPECIES := ["wolf", "stag", "boar", "fox", "serpent", "panther", "elk", "owl"]
const MAX_VISUAL_HITS := 18
var visual_meshes: Dictionary = {}
var visual_materials: Dictionary = {}
var visual_rigs: Array[Node3D] = []
var decorative_groups: Array[Node3D] = []
var hit_effects: Array[Dictionary] = []
var v14_visual_tick := 0.0
var life_bar: ProgressBar
var experience_bar: ProgressBar
var visual_frames := 0
var visual_hit_count := 0

func _ready() -> void:
 super._ready()
 _show_message("Moonfall Native v14 • Living Art overhaul loaded.", 4.5)

func _mesh_for(shape: String) -> Mesh:
 if visual_meshes.has(shape):return visual_meshes[shape] as Mesh
 var mesh: Mesh
 match shape:
  "box":
   var m:=BoxMesh.new()
   m.size=Vector3.ONE
   mesh=m
  "cone":
   var m:=CylinderMesh.new()
   m.top_radius=0.0
   m.bottom_radius=0.5
   m.height=1.0
   m.radial_segments=8
   mesh=m
  "cylinder":
   var m:=CylinderMesh.new()
   m.top_radius=0.5
   m.bottom_radius=0.5
   m.height=1.0
   m.radial_segments=8
   mesh=m
  "ring":
   var m:=TorusMesh.new()
   m.inner_radius=0.83
   m.outer_radius=1.0
   m.rings=32
   m.ring_segments=6
   mesh=m
  _:
   var m:=SphereMesh.new()
   m.radius=0.5
   m.height=1.0
   m.radial_segments=10
   m.rings=5
   mesh=m
 visual_meshes[shape]=mesh
 return mesh

func _vmat(color: Color, luminous: bool=false) -> StandardMaterial3D:
 var key:=color.to_html(true)+(":glow" if luminous else ":matte")
 if visual_materials.has(key):return visual_materials[key] as StandardMaterial3D
 var material:=StandardMaterial3D.new()
 material.albedo_color=color
 material.metallic=0.12 if luminous else 0.0
 material.roughness=0.28 if luminous else 0.85
 if luminous:
  material.emission_enabled=true
  material.emission=color
  material.emission_energy_multiplier=1.6
 visual_materials[key]=material
 return material

func _form(parent: Node3D, shape: String, position: Vector3, size: Vector3, color: Color, luminous: bool=false, angles: Vector3=Vector3.ZERO) -> MeshInstance3D:
 var visual:=MeshInstance3D.new()
 visual.mesh=_mesh_for(shape)
 visual.material_override=_vmat(color,luminous)
 visual.position=position
 visual.scale=size
 visual.rotation_degrees=angles
 parent.add_child(visual)
 return visual

func _build_scene() -> void:
 super._build_scene()
 # Add one genuine tameable owl to each sector AFTER all existing slots.
 # Appending preserves every original wildlife, guardian and rift slot ID in old saves.
 for zone in range(REGIONS.size()):
  var center:Vector2=REGIONS[zone]["at"]
  _spawn_beast(Vector3(center.x-12.0,0.0,center.y-16.0),"owl",zone,"moon" if zone==4 else "normal")

func _build_region(zone: int) -> void:
 super._build_region(zone)
 # Add lightweight non-colliding landmarks under the EXISTING region visibility root.
 var root:=Node3D.new()
 root.name="V14_ArtDirection_"+str(zone)
 region_nodes[zone].add_child(root)
 decorative_groups.append(root)
 var center:Vector2=REGIONS[zone]["at"]
 var accent:=Color(str(REGIONS[zone]["light"]))
 var stone:=Color("403948")
 var at:=Vector3(center.x-15.0,0.0,center.y+12.0)
 # All six zones have a readable magical focal point, with different silhouettes.
 _form(root,"cylinder",at+Vector3(0,0.34,0),Vector3(5.3,0.65,5.3),stone)
 _form(root,"ring",at+Vector3(0,0.70,0),Vector3(2.7,0.13,2.7),accent,true)
 match zone:
  0: # Hearthstone: guarded campfire with timber palisades and tents.
   _form(root,"cone",at+Vector3(0,1.75,0),Vector3(1.8,2.0,1.8),Color("bb7149"))
   _form(root,"sphere",at+Vector3(0,2.9,0),Vector3(0.55,0.8,0.55),Color("ffd78b"),true)
   for i in range(6):
    var angle:=TAU*float(i)/6.0
    _form(root,"cylinder",at+Vector3(cos(angle)*3.8,1.25,sin(angle)*3.8),Vector3(0.4,2.5,0.4),Color("674b3b"))
    _form(root,"cone",at+Vector3(cos(angle)*3.8,2.6,sin(angle)*3.8),Vector3(0.65,0.7,0.65),Color("96705b"))
  1: # Witchwood: haunted heart-tree, roots, emerald lanterns.
   _form(root,"cylinder",at+Vector3(0,3.2,0),Vector3(1.8,6.4,1.8),Color("4a3044"))
   for i in range(5):
    var angle:=TAU*float(i)/5.0
    _form(root,"cone",at+Vector3(cos(angle)*1.7,5.2,sin(angle)*1.7),Vector3(3.3,4.0,3.3),Color("2c5e53"),false,Vector3(0,0,14))
   _form(root,"sphere",at+Vector3(0,6.9,0),Vector3(0.75,0.75,0.75),accent,true)
  2: # Moonmist: totems, glowing marsh reeds and cyan wisps.
   for i in range(3):
    var shift:float=float(i-1)*2.1
    _form(root,"cylinder",at+Vector3(shift,1.85,0),Vector3(0.7,3.7,0.7),Color("486d64"))
    _form(root,"sphere",at+Vector3(shift,3.9,0),Vector3(0.75,0.75,0.75),accent,true)
   for i in range(9):
    var angle:=TAU*float(i)/9.0
    _form(root,"cone",at+Vector3(cos(angle)*4.6,0.9,sin(angle)*4.6),Vector3(0.25,1.8,0.25),Color("4b816d"))
  3: # Highcrest: mine crane and glowing exposed crystal seam.
   _form(root,"box",at+Vector3(0,3,0),Vector3(0.8,6,0.9),Color("695548"))
   _form(root,"box",at+Vector3(1.4,5.7,0),Vector3(3.6,0.4,0.7),Color("8f725a"))
   for i in range(5):
    var angle:=float(i)*TAU/5.0
    _form(root,"cone",at+Vector3(cos(angle)*2.0,1.6,sin(angle)*2.0),Vector3(0.85,3.0,0.85),accent,true,Vector3(0,0,20))
  4: # Shattered Ruins: floating arcane stones and concentric seals.
   for i in range(5):
    var angle:=TAU*float(i)/5.0
    _form(root,"box",at+Vector3(cos(angle)*2.2,3.1+float(i%2)*0.7,sin(angle)*2.2),Vector3(1.1,2.4,0.85),Color("786b99"),false,Vector3(12,float(i)*36.0,11))
   _form(root,"ring",at+Vector3(0,0.76,0),Vector3(4.1,0.15,4.1),accent,true)
  5: # Eclipse Citadel: black spires and crimson eclipse orb.
   for i in range(4):
    var angle:=TAU*float(i)/4.0+0.8
    _form(root,"cone",at+Vector3(cos(angle)*2.4,3.2,sin(angle)*2.4),Vector3(1.4,6.4,1.4),Color("302333"))
    _form(root,"sphere",at+Vector3(cos(angle)*2.4,6.7,sin(angle)*2.4),Vector3(0.35,0.35,0.35),accent,true)
   _form(root,"sphere",at+Vector3(0,4.9,0),Vector3(2.4,2.4,2.4),Color("a43d67"),true)
 # Decorative ground motifs distinguish the biomes without extra colliders/lights.
 for i in range(8):
  var angle:=TAU*float(i)/8.0
  var radius:=8.5+float(i%3)*2.5
  var spot:=Vector3(center.x+cos(angle)*radius,0.09,center.y+sin(angle)*radius)
  _form(root,"ring",spot,Vector3(0.55,0.08,0.55),accent.darkened(0.2),true)

func _spawn_beast(at: Vector3, kind: String, zone: int, variant: String="normal", boss: bool=false) -> void:
 super._spawn_beast(at,kind,zone,variant,boss)
 var beast:Dictionary=beasts[-1]
 var body:CharacterBody3D=beast["body"]
 # Remove only the OLD procedural capsule from view, never gameplay collisions or art files.
 for child in body.get_children():
  if child is MeshInstance3D:child.visible=false
 var rig:=Node3D.new()
 rig.name="V14_"+kind.capitalize()+"_Rig"
 body.add_child(rig)
 visual_rigs.append(rig)
 var pelt:=Color("535d70")
 match kind:
  "stag","elk":pelt=Color("8f6851")
  "fox":pelt=Color("bd7448")
  "boar":pelt=Color("605052")
  "serpent":pelt=Color("3c7668")
  "panther":pelt=Color("27283e")
  "owl":pelt=Color("807780")
 if variant=="moon":pelt=pelt.lerp(Color("a38dff"),0.55)
 if variant=="dusk":pelt=pelt.lerp(Color("f28255"),0.48)
 if variant=="elder":pelt=pelt.lerp(Color("8ac69e"),0.42)
 var accent:=Color(str(REGIONS[zone]["light"]))
 var size:float=1.55 if boss else 1.22 if variant=="elder" else 1.0
 rig.scale=Vector3.ONE*size
 if kind=="serpent":
  for i in range(5):
   _form(rig,"sphere",Vector3(-0.64+float(i)*0.30,-0.28+0.07*sin(float(i)),0),Vector3(0.50-float(i)*0.05,0.48-float(i)*0.05,0.48-float(i)*0.05),pelt)
  _form(rig,"sphere",Vector3(0.64,-0.02,0),Vector3(0.61,0.43,0.44),pelt.lightened(0.12))
  _form(rig,"cone",Vector3(0.87,0.12,0),Vector3(0.16,0.44,0.16),accent,true,Vector3(0,0,-45))
 elif kind=="owl":
  _form(rig,"sphere",Vector3(0,-0.03,0),Vector3(0.90,1.05,0.75),pelt)
  _form(rig,"sphere",Vector3(0.3,0.44,0),Vector3(0.70,0.72,0.70),pelt.lightened(0.09))
  for side in [-1.0,1.0]:
   _form(rig,"sphere",Vector3(-0.12,-0.05,side*0.46),Vector3(0.70,0.27,0.80),pelt.darkened(0.15),false,Vector3(0,0,20*side))
   _form(rig,"sphere",Vector3(0.61,0.49,side*0.19),Vector3(0.15,0.19,0.15),accent,true)
   _form(rig,"cone",Vector3(0.3,0.95,side*0.31),Vector3(0.21,0.43,0.21),pelt)
  _form(rig,"cone",Vector3(0.73,0.25,0),Vector3(0.2,0.38,0.2),Color("f1bd6d"),false,Vector3(0,0,-90))
 else:
  var mass:=Vector3(1.44,0.76,0.70) if kind=="boar" else Vector3(1.42,0.65,0.60) if kind in ["stag","elk"] else Vector3(1.20,0.60,0.61)
  _form(rig,"sphere",Vector3(-0.10,-0.04,0),mass,pelt)
  for x in [-0.47,0.36]:
   for side in [-1.0,1.0]:
    _form(rig,"cylinder",Vector3(x,-0.42,side*0.23),Vector3(0.21,0.72,0.21),pelt.darkened(0.13))
    _form(rig,"sphere",Vector3(x+0.08,-0.75,side*0.23),Vector3(0.27,0.17,0.22),pelt.darkened(0.26))
  _form(rig,"sphere",Vector3(0.60,0.13,0),Vector3(0.69,0.58,0.53),pelt.lightened(0.14))
  _form(rig,"sphere",Vector3(0.93,-0.05,0),Vector3(0.38,0.29,0.36),pelt.lightened(0.06))
  _form(rig,"cone",Vector3(-0.90,0.05,0),Vector3(0.27,0.82,0.27),pelt,false,Vector3(0,0,-68))
  for side in [-1.0,1.0]:
   _form(rig,"cone",Vector3(0.56,0.61,side*0.24),Vector3(0.26,0.48,0.26),pelt.darkened(0.07))
   _form(rig,"sphere",Vector3(0.87,0.20,side*0.26),Vector3(0.12,0.12,0.12),accent,true)
   if kind in ["stag","elk"]:
    _form(rig,"cylinder",Vector3(0.54,0.95,side*0.24),Vector3(0.10,0.83,0.10),Color("d9c39b"),false,Vector3(0,0,side*19))
    _form(rig,"cone",Vector3(0.77,1.35,side*0.37),Vector3(0.13,0.55,0.13),Color("ddc9a4"),false,Vector3(0,0,side*29))
   if kind=="boar":
    _form(rig,"cone",Vector3(1.12,0.01,side*0.25),Vector3(0.17,0.42,0.17),Color("e1d6be"),false,Vector3(0,0,-32))
  if kind=="panther":
   for k in range(5):
    _form(rig,"sphere",Vector3(-0.55+float(k)*0.27,0.23,0.0),Vector3(0.16,0.15,0.25),Color("635784"))
 # Distinct silhouettes, colored glowing markings, boss coronas.
 if variant!="normal":
  _form(rig,"ring",Vector3(0,-0.69,0),Vector3(0.85,0.05,0.85),accent,true)
 if boss:
  rig.set_meta("boss_ornamented",true)
  _form(rig,"ring",Vector3(0,1.52,0),Vector3(0.9,0.10,0.9),Color("ff628b"),true,Vector3(28,0,0))
  for i in range(4):
   _form(rig,"cone",Vector3(-0.25+float(i)*0.28,0.73,0),Vector3(0.24,0.7,0.24),Color("db9bca"),true)

func _create_gear_drop(at: Vector3, kind: String, variant: String, zone: int, existing: Dictionary={}) -> void:
 var before:=gear_drops.size()
 super._create_gear_drop(at,kind,variant,zone,existing)
 if gear_drops.size()<=before:return
 var entry:Dictionary=gear_drops[-1]
 var node:Node3D=entry["node"]
 var rarity:=str(entry["data"].get("rarity","Common"))
 var hue:=Color("bdcedd")
 match rarity:
  "Uncommon":hue=Color("72e3a6")
  "Rare":hue=Color("77b5ff")
  "Epic":hue=Color("c59cff")
  "Legendary":hue=Color("ffd17c")
  "Mythic":hue=Color("ff6c9e")
 _form(node,"ring",Vector3(0,0.15,0),Vector3(0.62,0.08,0.62),hue,true)
 _form(node,"cone",Vector3(0,0.65,0),Vector3(0.43,0.68,0.43),hue,true,Vector3(0,0,180))

func _apply_player_hit(target: Dictionary, base_damage: float, stagger_strength: float=0.0) -> void:
 super._apply_player_hit(target,base_damage,stagger_strength)
 if base_damage>0 and not target.is_empty() and is_instance_valid(target.get("body")):
  var node:Node3D=target["body"]
  _impact_ring(node.global_position+Vector3(0,-0.1,0),Color("ffc477") if last_hit_critical else Color("a58aff"))

func _spell(which: String) -> void:
 var prior:float=ability_cooldown
 super._spell(which)
 if is_instance_valid(player) and ability_cooldown>prior:
  _impact_ring(player.global_position+Vector3(0,-0.70,0),Color("6cd2fa") if which=="nova" else Color("f679ac"))

func _impact_ring(at: Vector3, color: Color) -> void:
 if hit_effects.size()>=MAX_VISUAL_HITS:return
 var root:=Node3D.new()
 root.name="V14_ImpactPulse"
 root.position=at
 world.add_child(root)
 _form(root,"ring",Vector3.ZERO,Vector3(0.60,0.13,0.60),color,true)
 hit_effects.append({"node":root,"time":0.0})
 visual_hit_count+=1

func _process(delta: float) -> void:
 super._process(delta)
 # Animated combat pulses own no colliders or lights; release after 0.45s.
 for entry in hit_effects.duplicate():
  entry["time"]=float(entry["time"])+delta
  var node:Node3D=entry["node"]
  if not is_instance_valid(node):
   hit_effects.erase(entry)
   continue
  var fraction:float=float(entry["time"])/0.45
  node.scale=Vector3.ONE*(1.0+fraction*2.0)
  if fraction>=1.0:
   hit_effects.erase(entry)
   node.queue_free()
 v14_visual_tick+=delta
 if v14_visual_tick<0.075:return
 v14_visual_tick=fmod(v14_visual_tick,0.075)
 visual_frames+=1
 for index in range(mini(visual_rigs.size(),beasts.size())):
  var beast:Dictionary=beasts[index]
  if not bool(beast.get("alive",false)):continue
  var actor:CharacterBody3D=beast["body"]
  if not is_instance_valid(actor) or not actor.visible:continue
  if is_instance_valid(player) and actor.global_position.distance_squared_to(player.global_position)>1600.0:continue
  var rig:Node3D=visual_rigs[index]
  if not is_instance_valid(rig):continue
  var speed:float=actor.velocity.length()
  var target_y:float=sin(map_time*4.0+float(index))*0.025 if speed<0.12 else sin(map_time*10.0+float(index))*0.055
  rig.position.y=lerpf(rig.position.y,target_y,0.5)
  if speed>0.15:
   var angle:float=atan2(-actor.velocity.z,actor.velocity.x)
   rig.rotation.y=lerp_angle(rig.rotation.y,angle,0.21)

func _build_ui() -> void:
 super._build_ui()
 # Shared dark-fantasy palette, high-contrast text and large focus highlights.
 _style_controls(start_screen)
 _style_controls(menu_panel)
 if is_instance_valid(quick_actions):_style_controls(quick_actions)
 for heading in [hud,hint,status,world_hud]:
  heading.add_theme_color_override("font_shadow_color",Color(0.0,0.0,0.0,0.95))
  heading.add_theme_constant_override("shadow_offset_x",2)
  heading.add_theme_constant_override("shadow_offset_y",2)
 for root in [start_screen,menu_panel]:
  for child in root.get_children():
   if child is Label:
    if "v13" in child.text:child.text=child.text.replace("v13","v14")
    if "Witchwood vertical slice" in child.text:child.text="Six regions • One living world • Z and Nyra original art"
 # Unobtrusive bars provide combat at-a-glance without replacing the existing HUD.
 var ui_root:Control=hud.get_parent()
 var shade:=Panel.new()
 shade.name="V14_HUD_Backplate"
 shade.mouse_filter=Control.MOUSE_FILTER_IGNORE
 shade.position=Vector2(8,5)
 shade.size=Vector2(855,198)
 shade.add_theme_stylebox_override("panel",_panel_style(Color(0.026,0.034,0.072,0.82),Color(0.45,0.32,0.59,0.52),12))
 ui_root.add_child(shade)
 ui_root.move_child(shade,0)
 life_bar=_progress(ui_root,Vector2(19,157),Vector2(255,15),Color("c94b70"))
 experience_bar=_progress(ui_root,Vector2(19,180),Vector2(255,11),Color("ae8af7"))
 _update_hud()

func _panel_style(fill: Color, stroke: Color, rounded: int) -> StyleBoxFlat:
 var box:=StyleBoxFlat.new()
 box.bg_color=fill
 box.border_color=stroke
 box.set_border_width_all(2)
 box.set_corner_radius_all(rounded)
 box.content_margin_left=7.0
 box.content_margin_right=7.0
 box.content_margin_top=5.0
 box.content_margin_bottom=5.0
 return box

func _style_controls(node: Node) -> void:
 if node is Button:
  var button:Button=node
  button.add_theme_stylebox_override("normal",_panel_style(Color("24243e"),Color("635980"),7))
  button.add_theme_stylebox_override("hover",_panel_style(Color("3e315b"),Color("c5a1eb"),7))
  button.add_theme_stylebox_override("pressed",_panel_style(Color("59346a"),Color("f4bdc9"),7))
  button.add_theme_stylebox_override("focus",_panel_style(Color(0.35,0.22,0.55,0.18),Color("f4dca2"),8))
  button.add_theme_color_override("font_color",Color("f5e9da"))
  button.add_theme_color_override("font_hover_color",Color.WHITE)
 elif node is PanelContainer:
  (node as PanelContainer).add_theme_stylebox_override("panel",_panel_style(Color("121629"),Color("5d4a78"),10))
 for child in node.get_children():_style_controls(child)

func _progress(parent: Control, pos: Vector2, dimensions: Vector2, fill: Color) -> ProgressBar:
 var bar:=ProgressBar.new()
 bar.mouse_filter=Control.MOUSE_FILTER_IGNORE
 bar.position=pos
 bar.size=dimensions
 bar.show_percentage=false
 bar.add_theme_stylebox_override("background",_panel_style(Color("171624"),Color("54506e"),3))
 bar.add_theme_stylebox_override("fill",_panel_style(fill,fill,3))
 parent.add_child(bar)
 return bar

func _update_hud() -> void:
 super._update_hud()
 if is_instance_valid(life_bar):
  life_bar.max_value=maxf(1.0,max_hp)
  life_bar.value=hp
  life_bar.visible=hero_playing
 if is_instance_valid(experience_bar):
  experience_bar.max_value=maxf(1.0,float(hero_xp_to_next))
  experience_bar.value=float(hero_xp)
  experience_bar.visible=hero_playing

func _apply_graphics_quality(mode: String) -> void:
 super._apply_graphics_quality(mode)
 # The existing Smooth toggle now also hides nonessential environmental accents.
 for group in decorative_groups:
  if is_instance_valid(group):group.visible=(mode!="Smooth")
