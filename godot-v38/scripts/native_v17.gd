extends "res://scripts/native_v16.gd"
## v17: encounter UI and legible guardian warnings.
## All added systems are cosmetic; original artwork, boss damage,
## physics, progression and checkpoint schema are unchanged.
const BOSS_UI_DISTANCE_SQUARED := 32.0 * 32.0
const BOSS_UI_REFRESH := 0.1
var boss_ui: PanelContainer
var boss_title: Label
var boss_health: ProgressBar
var boss_hint: Label
var boss_ui_clock := 0.0
var boss_ui_refresh_count := 0
var boss_warning_pulses := 0
var boss_last_visible_id := -1

func _ready() -> void:
 super._ready()
 _show_message("Moonfall Native v17: Guardian encounters and Windows release checks.",4.5)

func _build_ui() -> void:
 super._build_ui()
 var layer:CanvasLayer=get_node("NativeAtlasUI")
 var root:Control=layer.get_child(0) as Control
 boss_ui=PanelContainer.new()
 boss_ui.name="GuardianEncounter"
 boss_ui.anchor_left=0.5
 boss_ui.anchor_right=0.5
 boss_ui.offset_left=-230
 boss_ui.offset_right=230
 boss_ui.offset_top=16
 boss_ui.offset_bottom=102
 boss_ui.mouse_filter=Control.MOUSE_FILTER_IGNORE
 var style:=StyleBoxFlat.new()
 style.bg_color=Color("130e20e6")
 style.border_color=Color("a77dc3")
 style.set_border_width_all(2)
 style.set_corner_radius_all(8)
 style.content_margin_left=15
 style.content_margin_right=15
 style.content_margin_top=7
 style.content_margin_bottom=7
 boss_ui.add_theme_stylebox_override("panel",style)
 root.add_child(boss_ui)
 var column:=VBoxContainer.new()
 column.mouse_filter=Control.MOUSE_FILTER_IGNORE
 column.add_theme_constant_override("separation",4)
 boss_ui.add_child(column)
 boss_title=Label.new()
 boss_title.text="GUARDIAN • PHASE 1"
 boss_title.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER
 boss_title.add_theme_color_override("font_color",Color("ffe6ca"))
 boss_title.add_theme_font_size_override("font_size",18)
 column.add_child(boss_title)
 boss_health=ProgressBar.new()
 boss_health.custom_minimum_size=Vector2(420,16)
 boss_health.show_percentage=false
 boss_health.max_value=100
 boss_health.value=100
 boss_health.mouse_filter=Control.MOUSE_FILTER_IGNORE
 var fill:=StyleBoxFlat.new()
 fill.bg_color=Color("ec5b87")
 fill.set_corner_radius_all(4)
 boss_health.add_theme_stylebox_override("fill",fill)
 column.add_child(boss_health)
 boss_hint=Label.new()
 boss_hint.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER
 boss_hint.add_theme_color_override("font_color",Color("f9d092"))
 boss_hint.add_theme_font_size_override("font_size",13)
 column.add_child(boss_hint)
 boss_ui.visible=false

func _start_game(selected:String) -> void:
 super._start_game(selected)
 boss_ui_clock=0.0
 boss_last_visible_id=-1
 if is_instance_valid(boss_ui):boss_ui.visible=false

func _refresh_boss_phase(beast:Dictionary) -> void:
 var previous:=int(beast.get("boss_phase",1))
 super._refresh_boss_phase(beast)
 var phase:=int(beast.get("boss_phase",1))
 if phase<=previous:return
 var body:CharacterBody3D=beast.get("body")
 if is_instance_valid(body) and is_instance_valid(player):
  if body.global_position.distance_squared_to(player.global_position)<BOSS_UI_DISTANCE_SQUARED:
   _cue(body.global_position+Vector3(0,0.05,0),"spell",Color("ff547f") if phase==3 else Color("ffcc73"))
   boss_warning_pulses+=1
 boss_ui_clock=BOSS_UI_REFRESH

func _set_boss_telegraph(beast:Dictionary,active:bool) -> void:
 super._set_boss_telegraph(beast,active)
 var id:=int(beast.get("boss_id",-1))
 if not boss_telegraphs.has(id):return
 var warning:MeshInstance3D=boss_telegraphs[id]
 if not is_instance_valid(warning):return
 # Brightens imminent danger, without modifying the mechanically accurate radius.
 if active:
  var remaining:=float(beast.get("windup",0.0))
  warning.transparency=clampf(0.16+0.30*remaining/maxf(0.01,BOSS_WINDUP_SECONDS),0.12,0.57)
  if warning.material_override is StandardMaterial3D:
   var material:StandardMaterial3D=warning.material_override
   material.transparency=BaseMaterial3D.TRANSPARENCY_ALPHA
   material.albedo_color=Color("ff3863a8") if int(beast.get("boss_phase",1))>=3 else Color("ff954fa8")

func _closest_visible_guardian() -> Dictionary:
 if not is_instance_valid(player) or region_index<0 or region_index>=zone_beast_lists.size():return {}
 var best:Dictionary={}
 var nearest:=BOSS_UI_DISTANCE_SQUARED
 for creature in zone_beast_lists[region_index]:
  if not bool(creature.get("boss",false)) or not bool(creature.get("alive",false)):continue
  var body:CharacterBody3D=creature.get("body")
  if not is_instance_valid(body) or not body.visible:continue
  var distance:=body.global_position.distance_squared_to(player.global_position)
  if distance<nearest:
   nearest=distance
   best=creature
 return best

func _update_guardian_overlay() -> void:
 boss_ui_refresh_count+=1
 if not is_instance_valid(boss_ui):return
 if not hero_playing or menu_open:
  boss_ui.visible=false
  boss_last_visible_id=-1
  return
 var guardian:=_closest_visible_guardian()
 if guardian.is_empty():
  boss_ui.visible=false
  boss_last_visible_id=-1
  return
 boss_ui.visible=true
 boss_last_visible_id=int(guardian.get("boss_id",-1))
 var current_hp:=maxf(0.0,float(guardian.get("hp",0)))
 var total_hp:=maxf(1.0,float(guardian.get("max",1)))
 var phase:=int(guardian.get("boss_phase",1))
 boss_title.text=str(guardian.get("kind","Guardian")).to_upper()+" GUARDIAN  •  PHASE "+str(phase)+"/3"
 boss_health.max_value=total_hp
 boss_health.value=minf(current_hp,total_hp)
 var windup:=float(guardian.get("windup",0.0))
 if windup>0.0:
  boss_hint.text="⚠  INCOMING ATTACK IN "+("%.1f" % windup)+"s  •  DODGE THE MARKED CIRCLE"
 elif map_time<float(guardian.get("stagger_until",0.0)):
  boss_hint.text="STAGGERED  •  STRIKE NOW"
 else:
  boss_hint.text="HP "+str(ceili(current_hp))+" / "+str(ceili(total_hp))+"  •  Watch for red ground warnings"

func _process(delta:float) -> void:
 super._process(delta)
 boss_ui_clock+=clampf(delta,0.0,0.2)
 if boss_ui_clock>=BOSS_UI_REFRESH:
  boss_ui_clock=fmod(boss_ui_clock,BOSS_UI_REFRESH)
  _update_guardian_overlay()
