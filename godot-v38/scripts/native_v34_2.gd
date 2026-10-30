extends "res://scripts/native_v34_1.gd"
## v34.2: region-specific invasion commanders, capped telegraphed hazards,
## readable wave status and saved faction rewards. No extra enemies or timers.
const V342_RULES:=preload("res://scripts/systems/invasion_overhaul_v342.gd")
var v342_warnings:Array[Dictionary]=[]
var v342_phase:int=0  # 0 resting, 1 visible warning
var v342_phase_clock:float=0.0
var v342_pattern:int=0
var v342_warning_site:int=-1
var v342_warning_count:int=0
var v342_avoided:int=0
var v342_hits:int=0
var v342_faction_rewards:int=0
var v342_hud_clock:float=0.0
var v342_status_panel:PanelContainer
var v342_status_label:Label

func _release_label()->String:return "v34.2 • Elemental Lunar Invasions"

func _build_scene()->void:
 super._build_scene()
 _v342_prepare_telegraphs()

func _v342_prepare_telegraphs()->void:
 # Six shared warning objects, instantiated at startup only (two/site).
 for site in range(V341_RULES.REGIONS.size()):
  var parts:Array[MeshInstance3D]=[]
  var color:Color=V342_RULES.warning_color(site)
  for slot in range(V342_RULES.MAX_WARNING_MARKERS):
   var mesh:=CylinderMesh.new()
   mesh.top_radius=V342_RULES.marker_radius(site)
   mesh.bottom_radius=V342_RULES.marker_radius(site)
   mesh.height=0.035
   mesh.radial_segments=28
   var mat:=StandardMaterial3D.new()
   mat.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
   mat.transparency=BaseMaterial3D.TRANSPARENCY_ALPHA
   mat.albedo_color=Color(color.r,color.g,color.b,0.60)
   mat.emission_enabled=true
   mat.emission=color
   var warning:=MeshInstance3D.new()
   warning.name="InvasionWarning_%d_%d"%[site,slot]
   warning.mesh=mesh
   warning.material_override=mat
   warning.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
   warning.visible=false
   world.add_child(warning)
   parts.append(warning)
  v342_warnings.append({"markers":parts})

func _build_ui()->void:
 super._build_ui()
 var layer:CanvasLayer=get_node("NativeAtlasUI") as CanvasLayer
 v342_status_panel=PanelContainer.new()
 v342_status_panel.name="LunarInvasionObjectives"
 v342_status_panel.set_anchors_preset(Control.PRESET_TOP_RIGHT)
 v342_status_panel.offset_left=-330.0
 v342_status_panel.offset_right=-16.0
 v342_status_panel.offset_top=155.0
 v342_status_panel.offset_bottom=219.0
 v342_status_panel.mouse_filter=Control.MOUSE_FILTER_IGNORE
 var bg:=StyleBoxFlat.new()
 bg.bg_color=Color(0.09,0.055,0.12,0.91)
 bg.border_color=Color(0.8,0.42,0.8,0.9)
 bg.set_border_width_all(2)
 bg.set_corner_radius_all(8)
 bg.content_margin_left=10
 bg.content_margin_right=10
 bg.content_margin_top=7
 bg.content_margin_bottom=7
 v342_status_panel.add_theme_stylebox_override("panel",bg)
 layer.add_child(v342_status_panel)
 v342_status_label=Label.new()
 v342_status_label.add_theme_font_size_override("font_size",15)
 v342_status_label.add_theme_color_override("font_color",Color("fff0ff"))
 v342_status_label.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER
 v342_status_panel.add_child(v342_status_label)
 v342_status_panel.visible=false

func _v342_refresh_status()->void:
 if not is_instance_valid(v342_status_panel):return
 var visible_now:bool=hero_playing and not death_active and v341_active_site>=0 and region_index==int(v341_sites[v341_active_site]["zone"])
 v342_status_panel.visible=visible_now
 if not visible_now:return
 var living:int=0
 for id in v341_sites[v341_active_site]["roster"]:
  if bool(beasts[int(id)].get("alive",false)):living+=1
 v342_status_label.text=V341_RULES.event_name(v341_active_site)+"  •  "+("COMMANDER" if v341_wave==3 else "WAVE %d / 2"%v341_wave)+"\n"+str(living)+" hostile(s) remain  •  Defeat to seal"

func _v342_hide_warnings()->void:
 for entry in v342_warnings:
  for node in entry["markers"]:
   var marker:MeshInstance3D=node
   marker.visible=false
 v342_phase=0
 v342_phase_clock=0.0
 v342_warning_site=-1

func _v342_reset()->void:
 _v342_hide_warnings()
 v342_pattern=0
 v342_hud_clock=0.0
 _v342_refresh_status()

func _v341_reset()->void:
 super._v341_reset()
 _v342_reset()

func _v341_activate_wave()->void:
 # Existing wave allocation, caps, and enemy attacks remain authoritative.
 super._v341_activate_wave()
 _v342_hide_warnings()
 if v341_active_site<0:return
 var site:int=v341_active_site
 for id in v341_sites[site]["roster"]:
  var beast:Dictionary=beasts[int(id)]
  if not bool(beast.get("alive",false)):continue
  var scaled:float=V342_RULES.health_scale(site,v341_wave)
  beast["max"]=float(beast["max"])*scaled
  beast["hp"]=float(beast["max"])
  beast["alert_until"]=map_time+60.0
  # Reuse existing elite lunge and guardian pattern logic.
  beast["elite_lunge_cd"]=V342_RULES.lunge_cooldown(site)
  if bool(beast.get("invasion_commander",false)):
   var tag:Label3D=beast["nameplate"]
   tag.text=V342_RULES.commander_title(site)
 _show_message(V342_RULES.wave_notice(site,v341_wave),2.7)
 _v342_refresh_status()

func _v341_advance_if_cleared()->void:
 var site:int=v341_active_site
 var final_blow:bool=site>=0 and v341_wave==3
 super._v341_advance_if_cleared()
 if final_blow and v341_active_site<0:
  # Award once after the original seal, using the existing saved materials.
  _gain_material(V342_RULES.bonus_material(site),1)
  v342_faction_rewards+=1
  suppress_autosave_message=true
  _save_game()
  suppress_autosave_message=false
  _show_message("REGION PURIFIED! Bonus: +1 "+V342_RULES.bonus_material(site)+".",3.4)
  _v342_hide_warnings()
 _v342_refresh_status()

func _v342_begin_strike()->void:
 if v341_active_site<0 or not is_instance_valid(player):return
 var site:int=v341_active_site
 var loc:Vector3=player.global_position
 v342_pattern+=1
 var markers:Array=v342_warnings[site]["markers"]
 var count:int=V342_RULES.marker_count(site)
 var centers:Array[Vector3]=V342_RULES.target_positions(site,loc,v342_pattern)
 for i in range(markers.size()):
  var m:MeshInstance3D=markers[i]
  m.visible=i<count
  if i<count:m.global_position=Vector3(centers[i].x,0.14,centers[i].z)
 v342_phase=1
 v342_phase_clock=0.0
 v342_warning_site=site
 v342_warning_count+=1
 _show_message(V342_RULES.warning_message(site),1.5)

func _v342_resolve_strike()->void:
 if v342_phase!=1 or v342_warning_site<0 or not is_instance_valid(player):return
 var site:int=v342_warning_site
 var caught:bool=false
 for m in v342_warnings[site]["markers"]:
  var marker:MeshInstance3D=m
  if marker.visible and V342_RULES.inside_warning(site,player.global_position,marker.global_position):
   caught=true
   break
 _v342_hide_warnings()
 if not caught:return
 if dodge_time>0.0:
  v342_avoided+=1
  _v34_gain_resonance(V342_RULES.DODGE_RESONANCE)
  _show_message("PERFECT INVASION DODGE! +"+str(V342_RULES.DODGE_RESONANCE)+" Moon Resonance.",1.7)
 else:
  v342_hits+=1
  _apply_incoming_damage(V342_RULES.strike_damage(site))
  if not death_active:_show_message("INVASION STRIKE! Escape the glowing circle.",1.6)

func _v342_tick(delta:float)->void:
 if v341_active_site<0 or v341_wave!=3 or not hero_playing or death_active or menu_open or mounted or not is_instance_valid(player):
  if v342_phase!=0:_v342_hide_warnings()
  return
 var site:int=v341_active_site
 if region_index!=int(v341_sites[site]["zone"]) or player.global_position.distance_squared_to(v341_sites[site]["anchor"])>55.0*55.0:
  if v342_phase!=0:_v342_hide_warnings()
  return
 if not _v341_roster_valid(site):
  _v341_reset()
  return
 var roster:Array=v341_sites[site]["roster"]
 var commander:Dictionary=beasts[int(roster[-1])]
 if not bool(commander.get("alive",false)):
  if v342_phase!=0:_v342_hide_warnings()
  return
 v342_phase_clock+=clampf(delta,0.0,0.15)
 if v342_phase==0 and v342_phase_clock>=V342_RULES.rest_seconds(site):_v342_begin_strike()
 elif v342_phase==1 and v342_phase_clock>=V342_RULES.warning_seconds(site):_v342_resolve_strike()

func _process(delta:float)->void:
 super._process(delta)
 _v342_tick(delta)
 v342_hud_clock+=clampf(delta,0.0,0.20)
 if v342_hud_clock>=0.25:
  v342_hud_clock=0.0
  _v342_refresh_status()

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if which=="World" and is_instance_valid(menu_body):
  menu_body.text+="\n\n[b]ELEMENTAL INVASION COMMANDERS[/b]\nStormbreach: focused lightning marks. Emberfall: split fire zones. Voidtide: broad void marks. All attacks have visible warnings; dodge to build Moon Resonance. Each commander grants an extra regional crafting material after sealing. The single-world enemy budget remains three. No building or survival timer."

func _perf_report()->Dictionary:
 var stats:Dictionary=super._perf_report()
 stats["version"]="v34.2"
 stats["elemental_warn_markers_prebuilt"]=v342_warnings.size()*V342_RULES.MAX_WARNING_MARKERS
 stats["elemental_warnings"]=v342_warning_count
 stats["elemental_dodges"]=v342_avoided
 stats["elemental_hits"]=v342_hits
 stats["elemental_faction_rewards"]=v342_faction_rewards
 return stats
