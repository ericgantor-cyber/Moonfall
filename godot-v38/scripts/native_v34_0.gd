extends "res://scripts/native_v33_8.gd"
## v34.0 combat/world overhaul. No new game mode, survival timer, build system,
## art changes, new save format, per-frame creature allocation or global enemy scans.
const V34_RULES:=preload("res://scripts/systems/lunar_overhaul_v34.gd")
var v34_resonance:int=0
var v34_finisher_active:bool=false
var v34_finisher_casts:int=0
var v34_finisher_hits:int=0
var v34_perfect_surge_dodges:int=0
var v34_surge_warnings:int=0
var v34_purified_markers:Array[Dictionary]=[]
var v34_purification_clock:float=0.0
var v34_resonance_panel:PanelContainer
var v34_resonance_label:Label

func _release_label()->String:return "v34.0 • Grand Lunar Overhaul"

func _build_scene()->void:
 super._build_scene()
 _v34_build_purification_markers()

func _v34_build_purification_markers()->void:
 # Persistent visual rewards derive only from the existing saved rift clear flags.
 var mesh:=CylinderMesh.new()
 mesh.top_radius=1.20
 mesh.bottom_radius=1.20
 mesh.height=0.035
 mesh.radial_segments=24
 var mat:=StandardMaterial3D.new()
 mat.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
 mat.transparency=BaseMaterial3D.TRANSPARENCY_ALPHA
 mat.albedo_color=Color(0.42,0.88,0.96,0.52)
 mat.emission_enabled=true
 mat.emission=Color(0.36,0.72,0.95)
 for site in range(rifts.size()):
  var glow:=MeshInstance3D.new()
  glow.name="PurifiedRift_%d"%site
  glow.mesh=mesh
  glow.material_override=mat
  glow.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
  glow.position=(rifts[site]["anchor"] as Vector3)+Vector3(0,0.08,0)
  glow.visible=false
  world.add_child(glow)
  v34_purified_markers.append({"node":glow,"site":site})

func _build_ui()->void:
 super._build_ui()
 # Single reusable, high-contrast status panel; independent of the crowded HUD.
 var layer:CanvasLayer=get_node("NativeAtlasUI") as CanvasLayer
 v34_resonance_panel=PanelContainer.new()
 v34_resonance_panel.name="MoonResonanceStatus"
 v34_resonance_panel.set_anchors_preset(Control.PRESET_TOP_RIGHT)
 v34_resonance_panel.offset_left=-310.0
 v34_resonance_panel.offset_right=-16.0
 v34_resonance_panel.offset_top=100.0
 v34_resonance_panel.offset_bottom=147.0
 v34_resonance_panel.mouse_filter=Control.MOUSE_FILTER_IGNORE
 var background:=StyleBoxFlat.new()
 background.bg_color=Color(0.075,0.055,0.11,0.89)
 background.border_color=Color(0.58,0.39,0.84,0.92)
 background.set_border_width_all(2)
 background.set_corner_radius_all(8)
 background.content_margin_left=12
 background.content_margin_top=7
 background.content_margin_right=12
 background.content_margin_bottom=7
 v34_resonance_panel.add_theme_stylebox_override("panel",background)
 layer.add_child(v34_resonance_panel)
 v34_resonance_label=Label.new()
 v34_resonance_label.add_theme_font_size_override("font_size",18)
 v34_resonance_label.add_theme_color_override("font_color",Color("f7e6ff"))
 v34_resonance_label.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER
 v34_resonance_panel.add_child(v34_resonance_label)
 _v34_refresh_meter()

func _v34_refresh_meter()->void:
 if not is_instance_valid(v34_resonance_label):return
 v34_resonance_label.text="MOON RESONANCE: READY • NOVA" if v34_resonance>=100 else "MOON RESONANCE  %d / 100"%v34_resonance
 if is_instance_valid(v34_resonance_panel):v34_resonance_panel.visible=hero_playing and not death_active

func _v34_gain_resonance(points:int)->void:
 if not hero_playing or death_active or v34_finisher_active:return
 var prior:int=v34_resonance
 v34_resonance=V34_RULES.gain(v34_resonance,points)
 if v34_resonance!=prior:
  _v34_refresh_meter()
  if prior<100 and v34_resonance==100:_show_message("MOON RESONANCE READY! Your next Moon Nova unleashes an Eclipse Break.",2.1)

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0)->void:
 var before:float=float(target.get("hp",0.0))
 super._apply_player_hit(target,base_damage,stagger_strength)
 if before>0.0 and float(target.get("hp",0.0))<before:
  _v34_gain_resonance(V34_RULES.HIT_GAIN)

func _defeat_beast(beast:Dictionary,pet_kill:bool)->void:
 var eligible:bool=bool(beast.get("alive",false)) and bool(beast.get("hostile",false)) and not pet_kill
 var is_boss:bool=bool(beast.get("boss",false))
 super._defeat_beast(beast,pet_kill)
 if eligible:_v34_gain_resonance(V34_RULES.BOSS_GAIN if is_boss else V34_RULES.KILL_GAIN)

func _spell(which:String)->void:
 var finisher:bool=which=="nova" and hero_playing and not menu_open and not death_active and not mounted and ability_cooldown<=0.0 and v34_resonance>=100
 if not finisher:
  super._spell(which)
  return
 # Reserve the charge before inherited spell damage; kills cannot recharge it mid-cast.
 v34_resonance=0
 v34_finisher_active=true
 _v34_refresh_meter()
 super._spell(which)
 var hits:int=0
 var power:float=V34_RULES.finisher_power(hero_level,rifts_cleared.size(),hero=="nyra")*_hero_damage_multiplier()
 # Bound the extra blast to four enemies already in the active region.
 for beast in zone_beast_lists[region_index]:
  if hits>=V34_RULES.FINISHER_TARGET_CAP:break
  if not bool(beast.get("alive",false)) or not bool(beast.get("hostile",false)):continue
  var body:CharacterBody3D=beast.get("body")
  if not is_instance_valid(body) or not is_instance_valid(player):continue
  if body.global_position.distance_squared_to(player.global_position)>V34_RULES.FINISHER_RANGE_SQ:continue
  if not _has_clear_sight(player,body):continue
  _apply_player_hit(beast,power,0.36)
  hits+=1
 v34_finisher_active=false
 v34_finisher_casts+=1
 v34_finisher_hits+=hits
 _show_message(("ECLIPSE BLOOM" if hero=="nyra" else "CRESCENT BREAK")+"! "+str(hits)+" extra strikes.",2.3)

func _hero_damage_multiplier()->float:
 return super._hero_damage_multiplier()*V34_RULES.damage_boon(rifts_cleared.size())

func _apply_incoming_damage(raw:float)->void:
 # The underlying armor/companion defenses still own the actual hit.
 # Each saved rift clear grants 2.5% additional damage resistance (max 7.5%).
 super._apply_incoming_damage(raw*V34_RULES.guard_boon(rifts_cleared.size()))

func _depth_surge_tick(delta:float)->void:
 # Replaces a repetitive static hazard with a fair, chamber-specific telegraph.
 # Prebuilt nodes only, one rift checked, never a survival countdown.
 if not hero_playing or death_active or menu_open or mounted or not is_instance_valid(player):
  if depth_surge_site>=0 or depth_surge_phase!=0:_depth_reset_surge()
  return
 var site:int=_depth_near_corruption()
 if site!=depth_surge_site:
  _depth_reset_surge()
  depth_surge_site=site
 if site<0:return
 var stage:int=clampi(int(rifts[site].get("stage",1)),1,3)
 depth_surge_clock+=clampf(delta,0.0,0.15)
 if depth_surge_phase==0 and depth_surge_clock>=V34_RULES.surge_rest(stage):
  depth_surge_phase=1
  depth_surge_clock=0.0
  var root:Node3D=depth_surge_nodes[site]["node"]
  root.global_position=V34_RULES.safe_surge_position(rifts[site]["anchor"],player.global_position,stage)
  root.visible=true
  v34_surge_warnings+=1
  _show_message("LUNAR SURGE: red ground means danger. Dodge or move clear!",2.2)
 elif depth_surge_phase==1 and depth_surge_clock>=DEPTH_SURGE_TELEGRAPH:
  depth_surge_phase=0
  depth_surge_clock=0.0
  var root:Node3D=depth_surge_nodes[site]["node"]
  root.visible=false
  if player.global_position.distance_squared_to(root.global_position)<=DEPTH_SURGE_HIT_RADIUS_SQ:
   if dodge_time>0.0:
    v34_perfect_surge_dodges+=1
    _v34_gain_resonance(V34_RULES.PERFECT_DODGE_GAIN)
    _show_message("PERFECT DODGE! +22 Moon Resonance",1.5)
   else:
    depth_surge_hits+=1
    _apply_incoming_damage(V34_RULES.rift_surge_damage(stage,region_index))
    if not death_active:_show_message("LUNAR SURGE HIT! Step outside the red circle.",1.6)

func _v34_update_purification()->void:
 if not is_instance_valid(player):return
 var here:Vector3=player.global_position
 for entry in v34_purified_markers:
  var site:int=int(entry["site"])
  var light:MeshInstance3D=entry["node"]
  light.visible=hero_playing and bool(rifts[site].get("cleared",false)) and here.distance_squared_to(light.global_position)<45.0*45.0

func _process(delta:float)->void:
 super._process(delta)
 v34_purification_clock+=clampf(delta,0.0,0.2)
 if v34_purification_clock>=0.50:
  v34_purification_clock=0.0
  _v34_update_purification()

func _start_game(selected:String)->void:
 v34_resonance=0
 v34_finisher_active=false
 super._start_game(selected)
 _v34_refresh_meter()
 _v34_update_purification()

func _restore_data(data:Dictionary)->void:
 v34_resonance=0
 v34_finisher_active=false
 super._restore_data(data)
 _v34_refresh_meter()
 _v34_update_purification()

func _respawn_player()->void:
 v34_resonance=0
 v34_finisher_active=false
 super._respawn_player()
 _v34_refresh_meter()

func _death_return_to_title()->void:
 super._death_return_to_title()
 _v34_refresh_meter()

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if not is_instance_valid(menu_body):return
 if which=="Character":
  menu_body.text+="\n\n[b]GRAND LUNAR COMBAT[/b]\nSuccessful hits, kills and perfect rift-surge dodges fill Moon Resonance (0–100). When full, the next Moon Nova becomes a Crescent Break for Z or Eclipse Bloom for Nyra, striking up to four nearby foes. Charge resets on death or reload; permanent progress is never lost."
 elif which=="World":
  menu_body.text+="\n\n[b]RIFT PURIFICATION BOONS[/b]\nSealed rifts: "+str(V34_RULES.cleared_boon_count(rifts_cleared.size()))+"/3. Every saved rift clear grants +2.5% damage and 2.5% damage resistance (both capped at 7.5%). Cleared rifts display a pale lunar seal. Rift surge positions become more threatening in later chambers, but always show a dodgeable warning. No building. No survival stopwatch."

func _perf_report()->Dictionary:
 var result:Dictionary=super._perf_report()
 result["version"]="v34.0"
 result["moon_resonance"]=v34_resonance
 result["crescent_finishers"]=v34_finisher_casts
 result["finisher_bonus_hits"]=v34_finisher_hits
 result["perfect_surge_dodges"]=v34_perfect_surge_dodges
 result["surge_warnings"]=v34_surge_warnings
 result["purified_rifts"]=V34_RULES.cleared_boon_count(rifts_cleared.size())
 return result
