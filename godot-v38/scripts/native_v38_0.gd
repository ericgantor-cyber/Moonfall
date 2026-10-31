extends "res://scripts/native_v37_7.gd"
## v38 practical overhaul: on-demand concise status and ocean safety feedback,
## leaving the combat/loot/world/save systems untouched and inheritance intact.
const V380_RULES:=preload("res://scripts/systems/overhaul_v380.gd")
const CELESTIAL_TALENTS:=preload("res://scripts/systems/celestial_talents_v398.gd")
var v398_panel:PanelContainer
var v398_tree_list:VBoxContainer
var v398_ranks:Dictionary={}
const V383_BENCHMARK:=preload("res://scripts/systems/combat_benchmark_v383.gd")
const V384_RINGS:=preload("res://scripts/systems/vitals_rings_v384.gd")
var v380_status_enabled:bool=false
var v380_status_panel:PanelContainer
var v380_status_text:Label
var v380_update_elapsed:float=0.0
var v380_status_updates:int=0
var v383_fps_samples:Array[int]=[]
var v383_combat_fps_samples:Array[int]=[]
var v383_peak_hostiles:int=0
var v389_slow_fight_samples:int=0
var v384_vitals_panel:PanelContainer
var v384_vitals_label:RichTextLabel
var v384_rings:Control
var v386_orb_image:TextureRect
var v386_orb_values:Label
var v384_vitals_elapsed:float=0.0
var v385_mana:float=100.0
const V385_MAX_MANA:float=100.0
const V385_MANA_REGEN:float=7.0
const V387_MAX_STAMINA:float=150.0
const V387_STAMINA_REGEN:float=18.0
const V387_DODGE_COST:float=28.0
var v387_stamina:float=V387_MAX_STAMINA
var v387_stamina_bar:ProgressBar
var v387_fallback_stamina_bar:ProgressBar
var v387_stamina_warning:Label
var v387_stamina_warning_elapsed:float=0.0
var v387_stamina_regen_wait:float=0.0
const V387_STAMINA_REGEN_DELAY:float=0.65
const V391_HEAVY_STAMINA_COST:float=32.0
const V391_HEAVY_DAMAGE_MULTIPLIER:float=1.65
var v391_heavy_attack_active:bool=false
const V392_PARRY_WINDOW:float=0.22
const V392_BLOCK_STAMINA_COST:float=12.0
const V392_PARRY_STAMINA_COST:float=18.0
var v392_guard_started:float=-999.0
const V393_RIPOSTE_WINDOW:float=1.1
const V393_RIPOSTE_MULTIPLIER:float=1.35
var v393_riposte_until:float=-999.0
var v393_riposte_active:bool=false
var v394_strike_direction:int=0
var v394_strike_direction_label:String="FRONT"
const V395_GUARD_WINDOW:float=0.9
const V395_GUARD_PERIOD:float=5.0
const V395_GUARD_REDUCTION:float=0.4
const V396_MATCHED_GUARD_REDUCTION:float=0.22

var v390_valid_attacks:int=0
var v390_blocked_attacks:int=0

func _ready()->void:
 super._ready()
 _v380_build_status_panel()
 _v384_build_vitals()
 _v398_build_tree()

func _build_ui()->void:
 super._build_ui()
 if not is_instance_valid(start_screen):return
 for node in start_screen.find_children("*","Label",true,false):
  var title:Label=node as Label
  if is_instance_valid(title) and "MOONFALL v37.7" in title.text:
   title.text="☾  MOONFALL v38.0 • THE LIVING REALM"

func _v380_build_status_panel()->void:
 var layer:CanvasLayer=get_node_or_null("NativeAtlasUI") as CanvasLayer
 if not is_instance_valid(layer):return
 v380_status_panel=PanelContainer.new()
 v380_status_panel.name="V380_OptionalLiveStatus"
 v380_status_panel.set_anchors_preset(Control.PRESET_TOP_RIGHT)
 v380_status_panel.offset_left=-370.0
 v380_status_panel.offset_right=-15.0
 v380_status_panel.offset_top=20.0
 v380_status_panel.offset_bottom=150.0
 v380_status_panel.mouse_filter=Control.MOUSE_FILTER_IGNORE
 var background:=StyleBoxFlat.new()
 background.bg_color=Color(0.03,0.09,0.16,0.86)
 background.border_color=Color(0.31,0.67,0.84,0.9)
 background.set_border_width_all(1)
 background.set_corner_radius_all(8)
 background.content_margin_left=10.0
 background.content_margin_right=10.0
 background.content_margin_top=7.0
 background.content_margin_bottom=7.0
 v380_status_panel.add_theme_stylebox_override("panel",background)
 layer.add_child(v380_status_panel)
 v380_status_text=Label.new()
 v380_status_text.name="V380_AccessibleReadout"
 v380_status_text.add_theme_font_size_override("font_size",13)
 v380_status_text.add_theme_color_override("font_color",Color("cce9f5"))
 v380_status_text.mouse_filter=Control.MOUSE_FILTER_IGNORE
 v380_status_panel.add_child(v380_status_text)
 v380_status_panel.visible=false

func _v384_build_vitals()->void:
 var layer:CanvasLayer=get_node_or_null("NativeAtlasUI") as CanvasLayer
 if not is_instance_valid(layer):return
 v384_vitals_panel=PanelContainer.new()
 v384_vitals_panel.name="V384_Vitals"
 v384_vitals_panel.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
 v384_vitals_panel.offset_left=16.0
 v384_vitals_panel.offset_right=435.0
 v384_vitals_panel.offset_top=-135.0
 v384_vitals_panel.offset_bottom=-14.0
 v384_vitals_panel.mouse_filter=Control.MOUSE_FILTER_IGNORE
 var backdrop:=StyleBoxFlat.new()
 backdrop.bg_color=Color(0.045,0.035,0.09,0.86)
 backdrop.border_color=Color(0.47,0.34,0.58,0.9)
 backdrop.set_border_width_all(1)
 backdrop.set_corner_radius_all(12)
 backdrop.set_content_margin_all(9)
 v384_vitals_panel.add_theme_stylebox_override("panel",backdrop)
 layer.add_child(v384_vitals_panel)
 var layout:=HBoxContainer.new()
 layout.add_theme_constant_override("separation",12)
 v384_vitals_panel.add_child(layout)
 v384_rings=V384_RINGS.new()
 v384_rings.name="V384_HP_XP_Rings"
 layout.add_child(v384_rings)
 v384_vitals_label=RichTextLabel.new()
 v384_vitals_label.name="V384_VitalsText"
 v384_vitals_label.bbcode_enabled=true
 v384_vitals_label.scroll_active=false
 v384_vitals_label.mouse_filter=Control.MOUSE_FILTER_IGNORE
 v384_vitals_label.add_theme_font_size_override("normal_font_size",16)
 v384_vitals_label.custom_minimum_size=Vector2(205.0,96.0)
 layout.add_child(v384_vitals_label)
 # Functional green stamina meter even before the custom art PNG is installed.
 var fallback_stamina:=ProgressBar.new()
 fallback_stamina.name="V387_FallbackStaminaBar"
 fallback_stamina.min_value=0.0
 fallback_stamina.max_value=V387_MAX_STAMINA
 fallback_stamina.value=v387_stamina
 fallback_stamina.show_percentage=false
 fallback_stamina.custom_minimum_size=Vector2(110.0,14.0)
 fallback_stamina.mouse_filter=Control.MOUSE_FILTER_IGNORE
 var fallback_fill:=StyleBoxFlat.new()
 fallback_fill.bg_color=Color("#69e26b")
 fallback_fill.set_corner_radius_all(4)
 fallback_stamina.add_theme_stylebox_override("fill",fallback_fill)
 layout.add_child(fallback_stamina)
 v387_fallback_stamina_bar=fallback_stamina
 v387_stamina_warning=Label.new()
 v387_stamina_warning.name="V387_LowStaminaWarning"
 v387_stamina_warning.text="LOW STAMINA"
 v387_stamina_warning.add_theme_font_size_override("font_size",12)
 v387_stamina_warning.add_theme_color_override("font_color",Color("#ffd16e"))
 v387_stamina_warning.visible=false
 layout.add_child(v387_stamina_warning)
 v384_vitals_panel.visible=false
 # Use the user's HUD PNG when present; fall back to functional meters otherwise.
 var four_color_art:String="res://assets/ui/a_high_detail_fantasy_rpg_hud_game_ui_overlay_on.png"
 var original_orb_art:String="res://assets/ui/moonfall_orb_hud.png"
 var selected_hud_art:String=four_color_art if ResourceLoader.exists(four_color_art) else original_orb_art
 if ResourceLoader.exists(selected_hud_art):
  # Keep the fallback alive but hidden so regressions and asset reloads stay safe.
  v384_vitals_panel.visible=false
  var plain_vitals:PanelContainer=v384_vitals_panel
  v384_vitals_panel=PanelContainer.new()
  v384_vitals_panel.name="V386_OriginalOrbHUD"
  v384_vitals_panel.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
  v384_vitals_panel.offset_left=12.0
  v384_vitals_panel.offset_right=432.0
  v384_vitals_panel.offset_top=-400.0
  v384_vitals_panel.offset_bottom=-12.0
  v384_vitals_panel.mouse_filter=Control.MOUSE_FILTER_IGNORE
  var transparent_panel:=StyleBoxEmpty.new()
  v384_vitals_panel.add_theme_stylebox_override("panel",transparent_panel)
  var overlay:=Control.new()
  overlay.custom_minimum_size=Vector2(420,280)
  overlay.mouse_filter=Control.MOUSE_FILTER_IGNORE
  v384_vitals_panel.add_child(overlay)
  v386_orb_image=TextureRect.new()
  v386_orb_image.set_texture(load(selected_hud_art))
  v386_orb_image.expand_mode=TextureRect.EXPAND_IGNORE_SIZE
  v386_orb_image.stretch_mode=TextureRect.STRETCH_KEEP_ASPECT_CENTERED
  v386_orb_image.mouse_filter=Control.MOUSE_FILTER_IGNORE
  overlay.add_child(v386_orb_image)
  v386_orb_image.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
  v386_orb_values=Label.new()
  v386_orb_values.mouse_filter=Control.MOUSE_FILTER_IGNORE
  overlay.add_child(v386_orb_values)
  v387_stamina_bar=ProgressBar.new()
  v387_stamina_bar.name="V387_StaminaBar"
  v387_stamina_bar.min_value=0.0
  v387_stamina_bar.max_value=V387_MAX_STAMINA
  v387_stamina_bar.value=v387_stamina
  v387_stamina_bar.show_percentage=false
  v387_stamina_bar.mouse_filter=Control.MOUSE_FILTER_IGNORE
  var stamina_fill:=StyleBoxFlat.new()
  stamina_fill.bg_color=Color("#69e26b")
  stamina_fill.set_corner_radius_all(5)
  v387_stamina_bar.add_theme_stylebox_override("fill",stamina_fill)
  var stamina_track:=StyleBoxFlat.new()
  stamina_track.bg_color=Color(0.04,0.14,0.06,0.85)
  stamina_track.set_corner_radius_all(5)
  v387_stamina_bar.add_theme_stylebox_override("background",stamina_track)
  overlay.add_child(v387_stamina_bar)
  v387_stamina_bar.set_anchors_preset(Control.PRESET_TOP_WIDE)
  v387_stamina_bar.offset_left=24.0
  v387_stamina_bar.offset_right=-24.0
  v387_stamina_bar.offset_top=330.0
  v387_stamina_bar.offset_bottom=345.0
  # Keep the low-resource cue available alongside the artwork, without a HUD box.
  var low_stam_hint:=Label.new()
  low_stam_hint.name="V387_OrbLowStamina"
  low_stam_hint.text="LOW STAMINA"
  low_stam_hint.add_theme_font_size_override("font_size",12)
  low_stam_hint.add_theme_color_override("font_color",Color("#ffd16e"))
  overlay.add_child(low_stam_hint)
  low_stam_hint.set_anchors_preset(Control.PRESET_TOP_WIDE)
  low_stam_hint.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER
  low_stam_hint.offset_top=348.0
  low_stam_hint.offset_bottom=370.0
  low_stam_hint.visible=false
  v386_orb_values.set_anchors_preset(Control.PRESET_TOP_LEFT)
  v386_orb_values.offset_left=10.0
  v386_orb_values.offset_right=410.0
  v386_orb_values.offset_top=284.0
  v386_orb_values.offset_bottom=326.0
  v386_orb_values.add_theme_font_size_override("font_size",14)
  v386_orb_values.add_theme_color_override("font_color",Color.WHITE)
  v386_orb_values.add_theme_color_override("font_shadow_color",Color(0.02,0.01,0.07,0.98))
  v386_orb_values.add_theme_constant_override("shadow_offset_x",1)
  v386_orb_values.add_theme_constant_override("shadow_offset_y",2)
  v386_orb_values.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER
  layer.add_child(v384_vitals_panel)
  v384_vitals_panel.visible=false
  plain_vitals.name="V384_BackupVitals"


func _v398_build_tree()->void:
 var layer:CanvasLayer=get_node_or_null("NativeAtlasUI") as CanvasLayer
 if not is_instance_valid(layer):return
 v398_panel=PanelContainer.new()
 v398_panel.name="CelestialTalentTree"
 v398_panel.set_anchors_preset(Control.PRESET_CENTER)
 v398_panel.custom_minimum_size=Vector2(490,550)
 v398_panel.position=Vector2(-245,-275)
 layer.add_child(v398_panel)
 var scroller:ScrollContainer=ScrollContainer.new()
 v398_panel.add_child(scroller)
 v398_tree_list=VBoxContainer.new()
 v398_tree_list.custom_minimum_size=Vector2(465,0)
 scroller.add_child(v398_tree_list)
 v398_panel.visible=false

func _v398_refresh_tree()->void:
 if not is_instance_valid(v398_tree_list):return
 for child in v398_tree_list.get_children():child.queue_free()
 var level_now:int=maxi(1,int(level))
 var points:int=CELESTIAL_TALENTS.points_for_level(level_now)-CELESTIAL_TALENTS.points_spent(v398_ranks)
 var heading:Label=Label.new()
 heading.text="CELESTIAL TALENTS  |  Points: "+str(points)+"  |  T to close"
 v398_tree_list.add_child(heading)
 for branch in CELESTIAL_TALENTS.BRANCHES:
  var section:Label=Label.new()
  section.text=str(branch).to_upper()
  v398_tree_list.add_child(section)
  for id in CELESTIAL_TALENTS.TALENTS:
   var info:Dictionary=CELESTIAL_TALENTS.TALENTS[id]
   if str(info["branch"])!=str(branch):continue
   if info.has("hero") and str(info["hero"])!=str(hero):continue
   var talent_button:Button=Button.new()
   talent_button.text=str(id).replace("_"," ").capitalize()+"  "+str(v398_ranks.get(id,0))+"/"+str(info["max_rank"])
   talent_button.disabled=not CELESTIAL_TALENTS.can_unlock(str(hero),level_now,v398_ranks,str(id))
   talent_button.pressed.connect(_v398_unlock.bind(str(id)))
   v398_tree_list.add_child(talent_button)

func _v398_unlock(id:String)->void:
 v398_ranks=CELESTIAL_TALENTS.unlock(str(hero),maxi(1,int(level)),v398_ranks,id)
 _v398_refresh_tree()

func _v398_toggle_tree()->void:
 if not hero_playing or death_active or menu_open or not is_instance_valid(v398_panel):return
 v398_panel.visible=not v398_panel.visible
 if v398_panel.visible:_v398_refresh_tree()

func _unhandled_input(event:InputEvent)->void:
 if event is InputEventKey and event.pressed and not event.echo and event.keycode==KEY_T:
  _v398_toggle_tree()
  get_viewport().set_input_as_handled()
  return
 if event is InputEventKey and event.pressed and not event.echo and hero_playing and not menu_open and not death_active:
  if event.keycode in [KEY_UP,KEY_RIGHT,KEY_DOWN,KEY_LEFT]:
   v394_strike_direction=[KEY_UP,KEY_RIGHT,KEY_DOWN,KEY_LEFT].find(event.keycode)
   v394_strike_direction_label=["HIGH","RIGHT","LOW","LEFT"][v394_strike_direction]
   _show_message("Sword angle: "+v394_strike_direction_label,0.55)
   get_viewport().set_input_as_handled()
   return
 if event is InputEventMouseButton and event.button_index==MOUSE_BUTTON_RIGHT and event.pressed and hero_playing and not menu_open and not death_active:
  v392_guard_started=map_time
  get_viewport().set_input_as_handled()
  return
 super._unhandled_input(event)

func _apply_incoming_damage(raw:float)->void:
 if not hero_playing or menu_open or death_active or mounted or not Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT):
  super._apply_incoming_damage(raw)
  return
 var parry:bool=map_time-v392_guard_started<=V392_PARRY_WINDOW
 var cost:float=V392_PARRY_STAMINA_COST if parry else V392_BLOCK_STAMINA_COST
 if v387_stamina<cost:
  super._apply_incoming_damage(raw)
  return
 v387_stamina-=cost
 v387_stamina_regen_wait=V387_STAMINA_REGEN_DELAY
 if parry:
  v393_riposte_until=map_time+V393_RIPOSTE_WINDOW
  _show_message("PARRY! Riposte now!",0.8)
 else:
  _show_message("Blocked!",0.5)
  super._apply_incoming_damage(raw*0.35)

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0)->void:
 # Reuse the existing stagger timer and hit visuals; do not create new AI state.
 var hp_before:float=float(target.get("hp",0.0))
 # Regular hostile creatures periodically guard. Heavy strikes or ripostes break their guard.
 var guarding:bool=bool(target.get("v395_guarding",false)) and bool(target.get("hostile",false)) and not bool(target.get("boss",false))
 var guard_break:bool=guarding and (v391_heavy_attack_active or v393_riposte_active)
 var matched_guard:bool=guarding and int(target.get("v396_guard_direction",-1))==v394_strike_direction
 if guard_break:
  target["v395_guarding"]=false
  target["stagger_until"]=maxf(float(target.get("stagger_until",0.0)),map_time+0.65)
  _show_message("GUARD BROKEN!",0.65)
 elif guarding:
  _show_message("Guarded "+(["HIGH","RIGHT","LOW","LEFT"][v394_strike_direction])+"! Change angle or strike heavy.",0.65)
 var angle_damage:float=1.08 if v394_strike_direction==0 else 1.0
 var angle_stagger:float=0.30 if v394_strike_direction==2 else stagger_strength
 super._apply_player_hit(target,base_damage*(V396_MATCHED_GUARD_REDUCTION if matched_guard and not guard_break else V395_GUARD_REDUCTION if guarding and not guard_break else 1.0)*angle_damage*(V391_HEAVY_DAMAGE_MULTIPLIER if v391_heavy_attack_active else 1.0)*(V393_RIPOSTE_MULTIPLIER if v393_riposte_active else 1.0),maxf(angle_stagger,0.42) if v393_riposte_active else maxf(angle_stagger,0.38) if v391_heavy_attack_active else angle_stagger)
 if hp_before<=float(target.get("hp",0.0)):return
 if not bool(target.get("hostile",false)) or bool(target.get("boss",false)) or not bool(target.get("alive",false)):return
 # Successful hits briefly interrupt ordinary enemies; stronger inherited staggers win.
 target["stagger_until"]=maxf(float(target.get("stagger_until",0.0)),map_time+0.22)
 # A brief, collision-aware impulse follows the original damage and hit reaction.
 # Never push bosses or creatures with invalid bodies.
 var enemy_body:CharacterBody3D=target.get("body") as CharacterBody3D
 if not is_instance_valid(enemy_body) or not is_instance_valid(player):return
 var away:Vector3=enemy_body.global_position-player.global_position
 away.y=0.0
 if away.length_squared()<0.01:return
 target["v388_knock_dir"]=away.normalized()
 target["v388_knock_until"]=map_time+0.14
 target["hit_react_until"]=maxf(float(target.get("hit_react_until",0.0)),map_time+0.25)


func _update_beast_step(beast:Dictionary,delta:float)->void:
 super._update_beast_step(beast,delta)
 if not bool(beast.get("alive",false)) or bool(beast.get("boss",false)):return
 if bool(beast.get("hostile",false)):
  var guard_body:CharacterBody3D=beast.get("body") as CharacterBody3D
  if is_instance_valid(guard_body):
   var phase:float=fposmod(map_time+float(guard_body.get_instance_id()%5),V395_GUARD_PERIOD)
   beast["v395_guarding"]=phase<V395_GUARD_WINDOW and map_time>=float(beast.get("stagger_until",0.0))
   beast["v396_guard_direction"]=(int(floor(map_time/V395_GUARD_PERIOD))+guard_body.get_instance_id())%4
   # Reuse one lightweight floating label per enemy, only while guarding.
   var guard_label:Label3D=beast.get("v397_guard_label") as Label3D
   if not is_instance_valid(guard_label):
    guard_label=Label3D.new()
    guard_label.name="GuardDirection"
    guard_label.billboard=BaseMaterial3D.BILLBOARD_ENABLED
    guard_label.font_size=28
    guard_label.pixel_size=0.009
    guard_label.position=Vector3(0.0,2.15,0.0)
    guard_body.add_child(guard_label)
    beast["v397_guard_label"]=guard_label
   # Defensive tell: guarded attacks glow amber; staggered foes cannot guard.
   guard_label.modulate=Color(1.0,0.74,0.25,1.0)
   guard_label.visible=bool(beast["v395_guarding"])
   if guard_label.visible:
    var guard_index:int=int(beast["v396_guard_direction"])
    var guard_name:String=["HIGH","RIGHT","LOW","LEFT"][guard_index]
    if guard_label.text!="GUARD "+guard_name:
     guard_label.text="GUARD "+guard_name


 if map_time>=float(beast.get("v388_knock_until",0.0)):return
 var body:CharacterBody3D=beast.get("body") as CharacterBody3D
 if not is_instance_valid(body):return
 var direction:Vector3=beast.get("v388_knock_dir",Vector3.ZERO)
 if direction.length_squared()<0.01:return
 # Bounded 1.8-meter/second separation that obeys physics collisions.
 body.move_and_collide(direction*1.8*minf(maxf(delta,0.0),0.05))

func _attack()->void:
 if not hero_playing or menu_open or death_active:
  v390_blocked_attacks+=1
  return
 # Hold Shift while attacking for a slower, harder-hitting melee strike.
 # Regular attacks continue through the original combo and targeting pipeline.
 var heavy_requested:bool=Input.is_key_pressed(KEY_SHIFT)
 if heavy_requested and attack_cooldown<=0.0 and not mounted and v387_stamina<V391_HEAVY_STAMINA_COST:
  _show_message("Not enough stamina for a heavy attack.",0.9)
  return
 # Arrow keys select one of four sword angles; normal attack remains usable without selection.
 var chosen_direction:int=v394_strike_direction
 if Input.is_key_pressed(KEY_UP):chosen_direction=0
 elif Input.is_key_pressed(KEY_RIGHT):chosen_direction=1
 elif Input.is_key_pressed(KEY_DOWN):chosen_direction=2
 elif Input.is_key_pressed(KEY_LEFT):chosen_direction=3
 v394_strike_direction=chosen_direction
 v394_strike_direction_label=["HIGH","RIGHT","LOW","LEFT"][chosen_direction]
 var cooldown_before:float=attack_cooldown
 v391_heavy_attack_active=heavy_requested and not mounted and attack_cooldown<=0.0
 v393_riposte_active=map_time<=v393_riposte_until and not mounted and attack_cooldown<=0.0
 super._attack()
 v391_heavy_attack_active=false
 v393_riposte_active=false
 if attack_cooldown>cooldown_before:
  v390_valid_attacks+=1
  if map_time<=v393_riposte_until:v393_riposte_until=-999.0
  if heavy_requested and not mounted:
   v387_stamina-=V391_HEAVY_STAMINA_COST
   v387_stamina_regen_wait=V387_STAMINA_REGEN_DELAY
   attack_cooldown=maxf(attack_cooldown,0.62)

func _spell(which:String)->void:
 # Rejected combat inputs must never consume mana while menus or death screens are active.
 if not hero_playing or menu_open or death_active:return
 if mounted or ability_cooldown>0.0:
  super._spell(which)
  return
 var cost:float=15.0 if which=="dash" else 25.0 if which=="nova" else 30.0
 if v385_mana<cost:
  _show_message("Not enough mana! Need "+str(roundi(cost))+".",1.3)
  return
 v385_mana-=cost
 super._spell(which)

func _dodge()->void:
 # Inactive gameplay must not trigger dodge actions or resource warnings.
 if not hero_playing or menu_open or death_active:return
 if mounted or dodge_cooldown>0.0:
  super._dodge()
  return
 if v387_stamina<V387_DODGE_COST:
  _show_message("Not enough stamina to dodge.",1.0)
  return
 # Charge stamina only when the dodge is eligible to start.
 v387_stamina-=V387_DODGE_COST
 v387_stamina_regen_wait=V387_STAMINA_REGEN_DELAY
 super._dodge()

func _start_game(selected_hero:String)->void:
 super._start_game(selected_hero)
 v385_mana=V385_MAX_MANA
 v387_stamina=V387_MAX_STAMINA
 v387_stamina_regen_wait=0.0

func _checkpoint_extra_data()->Dictionary:
 return {"mana":clampf(v385_mana,0.0,V385_MAX_MANA),"stamina":clampf(v387_stamina,0.0,V387_MAX_STAMINA)}

func _restore_data(data:Dictionary)->void:
 super._restore_data(data)
 v387_stamina_regen_wait=0.0
 var resources:Variant=data.get("optional_resources",{})
 # Legacy checkpoints without a mana field continue to load at full mana.
 if resources is Dictionary:
  v385_mana=clampf(float(resources.get("mana",V385_MAX_MANA)),0.0,V385_MAX_MANA)
  v387_stamina=clampf(float(resources.get("stamina",V387_MAX_STAMINA)),0.0,V387_MAX_STAMINA)
 else:
  v385_mana=V385_MAX_MANA
  v387_stamina=V387_MAX_STAMINA

func _v384_refresh_vitals()->void:
 if is_instance_valid(v387_stamina_bar):
  v387_stamina_bar.value=clampf(v387_stamina,0.0,V387_MAX_STAMINA)
  var orb_hint:Label=v387_stamina_bar.get_parent().get_node_or_null("V387_OrbLowStamina") as Label
  if is_instance_valid(orb_hint):orb_hint.visible=v387_stamina<V387_DODGE_COST
 if is_instance_valid(v387_fallback_stamina_bar):v387_fallback_stamina_bar.value=clampf(v387_stamina,0.0,V387_MAX_STAMINA)
 if is_instance_valid(v387_stamina_warning):v387_stamina_warning.visible=v387_stamina<V387_DODGE_COST
 if is_instance_valid(v386_orb_values):
  var hp_value:int=clampi(roundi(hp),0,maxi(1,roundi(max_hp)))
  var mana_value:int=clampi(roundi(v385_mana),0,roundi(V385_MAX_MANA))
  var xp_value:int=clampi(hero_xp,0,maxi(1,hero_xp_to_next))
  v386_orb_values.text="LIVE  ♥ %d/%d   ◆ %d/%d\n✦ %d/%d   STAM %d/%d" % [hp_value,roundi(max_hp),mana_value,roundi(V385_MAX_MANA),xp_value,hero_xp_to_next,roundi(v387_stamina),roundi(V387_MAX_STAMINA)]
 if not is_instance_valid(v384_vitals_label):return
 if is_instance_valid(v384_rings):
  v384_rings.call("set_vitals",hp/maxf(1.0,max_hp),float(hero_xp)/float(maxi(1,hero_xp_to_next)))
 var health:int=roundi(maxf(0.0,hp))
 var health_max:int=maxi(1,roundi(max_hp))
 var experience:int=maxi(0,hero_xp)
 var experience_target:int=maxi(1,hero_xp_to_next)
 # This display is backed by the spell-cost resource in v38.5.
 v384_vitals_label.text="[color=#ff555f]● HP   %d / %d[/color]\n[color=#5caeff]● MANA   %d / %d[/color]\n[color=#b786ff]● XP   %d / %d[/color]\n[color=#69e26b]● STAM   %d / %d[/color]" % [health,health_max,roundi(v385_mana),roundi(V385_MAX_MANA),experience,experience_target,roundi(v387_stamina),roundi(V387_MAX_STAMINA)]

func _unhandled_key_input(event:InputEvent)->void:
 if event is InputEventKey and event.pressed and not event.echo and event.keycode==KEY_F2 and hero_playing and not menu_open:
  v380_status_enabled=not v380_status_enabled
  if v380_status_enabled:
   v383_fps_samples.clear()
   v383_combat_fps_samples.clear()
   v383_peak_hostiles=0
  v380_update_elapsed=V380_RULES.UPDATE_PERIOD
  get_viewport().set_input_as_handled()
  return
 super._unhandled_key_input(event)

func _process(delta:float)->void:
 super._process(delta)
 if hero_playing and not death_active and not menu_open:
  v385_mana=minf(V385_MAX_MANA,v385_mana+V385_MANA_REGEN*maxf(0.0,delta))
  var stamina_step:float=maxf(0.0,delta)
  if v387_stamina_regen_wait>0.0:
   var delay_used:float=minf(v387_stamina_regen_wait,stamina_step)
   v387_stamina_regen_wait-=delay_used
   stamina_step-=delay_used
  if stamina_step>0.0:v387_stamina=minf(V387_MAX_STAMINA,v387_stamina+V387_STAMINA_REGEN*stamina_step)
 if is_instance_valid(v384_vitals_panel):
  var vitals_visible:bool=hero_playing and not death_active and not menu_open
  v384_vitals_panel.visible=vitals_visible
  if vitals_visible:
   v384_vitals_elapsed+=minf(delta,0.15)
   if v384_vitals_elapsed>=0.25:
    v384_vitals_elapsed=0.0
    _v384_refresh_vitals()
 if not is_instance_valid(v380_status_panel):return
 var visible_now:bool=v380_status_enabled and hero_playing and not death_active and not menu_open
 v380_status_panel.visible=visible_now
 if not visible_now:return
 v380_update_elapsed+=minf(delta,0.15)
 if v380_update_elapsed<V380_RULES.UPDATE_PERIOD:return
 v380_update_elapsed=0.0
 var region:String="Unknown"
 if region_index>=0 and region_index<REGIONS.size():region=str(REGIONS[region_index]["name"])
 var underwater:bool=V373_RULES.truly_submerged(v373_depth)
 var fps_now:int=maxi(0,Engine.get_frames_per_second())
 V383_BENCHMARK.record_fps(v383_fps_samples,fps_now)
 if current_threat:
  V383_BENCHMARK.record_fps(v383_combat_fps_samples,fps_now)
  if fps_now>0 and fps_now<30:v389_slow_fight_samples+=1
 var summary:Dictionary=V383_BENCHMARK.summarize_fps(v383_fps_samples)
 var combat_summary:Dictionary=V383_BENCHMARK.summarize_fps(v383_combat_fps_samples)
 var hostile_count:int=0
 if region_index>=0 and region_index<zone_beast_lists.size():
  for creature in zone_beast_lists[region_index]:
   if bool(creature.get("alive",false)) and bool(creature.get("hostile",false)):hostile_count+=1
 v383_peak_hostiles=maxi(v383_peak_hostiles,hostile_count)
 v380_status_text.text=V380_RULES.safe_label(region,hp,max_hp,v372_oxygen,underwater,v373_depth,current_threat)
 v380_status_text.text+="\nFPS "+str(fps_now)+" (avg/min "+str(summary["average"])+"/"+str(summary["min"])+")  •  Foes "+str(hostile_count)
 if int(combat_summary["count"])>0:
  v380_status_text.text+="\nCombat FPS avg/min: "+str(combat_summary["average"])+"/"+str(combat_summary["min"])
 else:
  v380_status_text.text+="\nCombat FPS: no encounter sampled yet"
 v380_status_text.add_theme_color_override("font_color",Color("ffb5a9") if V380_RULES.air_urgency(v372_oxygen,underwater)>0 else Color("cce9f5"))
 v380_status_updates+=1

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if not is_instance_valid(menu_body):return
 if which=="System":
  menu_body.text+="\n\n[b]v38 LIVE STATUS[/b]\nF2 toggles a compact status readout showing your region, HP, underwater depth and oxygen. It updates only four times per second and stays hidden by default. When enabled, it also shows FPS, rolling 10-second average/minimum, hostile count, and encounter-only FPS. F9's performance report includes sample and peak enemy details. Combat samples are recorded only while a nearby threat is active."

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["overhaul"]="v38.0 safer swim art pose, optional status and lean handoff"
 report["status_visible"]=v380_status_enabled
 report["status_ui_updates"]=v380_status_updates
 report["combat_valid_attack_inputs"]=v390_valid_attacks
 report["combat_blocked_attack_inputs"]=v390_blocked_attacks
 # Collected only on demand when the existing profiler requests a report.
 # Never scan enemy children during normal render/physics frames.
 report["combat_fps"]=Engine.get_frames_per_second()
 report["combat_frame_process_ms"]=Performance.get_monitor(Performance.TIME_PROCESS)*1000.0
 report["combat_frame_physics_ms"]=Performance.get_monitor(Performance.TIME_PHYSICS_PROCESS)*1000.0
 # Engine counters help distinguish render load, scene size and physics pressure.
 # Query only when the existing performance report is requested, never every frame.
 report["combat_render_draw_calls"]=int(Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME))
 report["combat_render_primitives"]=int(Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME))
 report["combat_scene_nodes"]=int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT))
 report["combat_physics_collision_pairs"]=int(Performance.get_monitor(Performance.PHYSICS_3D_COLLISION_PAIRS))
 # Per-frame rendering/physics monitors are sampled only when a report is opened.
 report["combat_frame_ms"]=1000.0/maxf(1.0,float(Engine.get_frames_per_second()))
 report["combat_draw_calls_per_hostile"]=0.0
 var active_hostiles:int=0
 var damage_number_count:int=0
 if region_index>=0 and region_index<zone_beast_lists.size():
  for beast in zone_beast_lists[region_index]:
   if not bool(beast.get("alive",false)):continue
   if bool(beast.get("hostile",false)):active_hostiles+=1
   var body:CharacterBody3D=beast.get("body") as CharacterBody3D
   if not is_instance_valid(body):continue
   for child in body.get_children():
    if child is Label3D and (child.has_meta("damage_float_tween") or child.name.begins_with("DamageFloat")):
     if not child.is_queued_for_deletion():damage_number_count+=1
 report["combat_active_hostiles"]=active_hostiles
 if active_hostiles>0:report["combat_draw_calls_per_hostile"]=float(report["combat_render_draw_calls"])/float(active_hostiles)
 report["combat_damage_floaters"]=damage_number_count
 var summary:Dictionary=V383_BENCHMARK.summarize_fps(v383_fps_samples)
 report["combat_sample_count"]=int(summary["count"])
 report["combat_sample_avg_fps"]=int(summary["average"])
 report["combat_sample_min_fps"]=int(summary["min"])
 report["combat_sample_max_fps"]=int(summary["max"])
 report["combat_peak_hostiles"]=v383_peak_hostiles
 report["combat_slow_fight_samples"]=v389_slow_fight_samples
 var combat_summary:Dictionary=V383_BENCHMARK.summarize_fps(v383_combat_fps_samples)
 report["combat_encounter_sample_count"]=int(combat_summary["count"])
 report["combat_encounter_avg_fps"]=int(combat_summary["average"])
 report["combat_encounter_min_fps"]=int(combat_summary["min"])
 return report
