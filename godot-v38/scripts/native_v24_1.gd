extends "res://scripts/native_v24.gd"
## v24.1: accessible input and low-cost feedback, without touching gameplay physics.
## The underlying art, saves, monsters and the invisible terrain grid are unchanged.

const HIT_MESSAGE_LIFETIME := 1.1
var accessible_hit_text: Label
var accessible_hit_remaining := 0.0
var accessible_hit_count := 0

func _ready() -> void:
 super._ready()
 var overlay := CanvasLayer.new()
 overlay.name = "AccessibleHitFeedback"
 overlay.layer = 4
 add_child(overlay)
 accessible_hit_text = Label.new()
 accessible_hit_text.name = "RecentDamage"
 accessible_hit_text.anchor_left = 0.5
 accessible_hit_text.anchor_right = 0.5
 accessible_hit_text.offset_left = -150
 accessible_hit_text.offset_right = 150
 accessible_hit_text.offset_top = 98
 accessible_hit_text.offset_bottom = 140
 accessible_hit_text.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
 accessible_hit_text.add_theme_font_size_override("font_size", 19)
 accessible_hit_text.add_theme_color_override("font_color", Color("ffecd1"))
 accessible_hit_text.add_theme_color_override("font_shadow_color", Color(0.0, 0.0, 0.0, 1.0))
 accessible_hit_text.add_theme_constant_override("shadow_offset_y", 2)
 accessible_hit_text.mouse_filter = Control.MOUSE_FILTER_IGNORE
 accessible_hit_text.visible = false
 overlay.add_child(accessible_hit_text)

func _unhandled_input(event: InputEvent) -> void:
 # Keyboard access to camera zoom complements the existing mouse wheel.
 # Only consume real keypresses during live play, never while using a menu.
 if hero_playing and not menu_open and event is InputEventKey and event.pressed and not event.echo:
  if event.keycode == KEY_EQUAL or event.keycode == KEY_KP_ADD:
   _v24_set_zoom(v24_camera_size - 1.5)
   get_viewport().set_input_as_handled()
   return
  if event.keycode == KEY_MINUS or event.keycode == KEY_KP_SUBTRACT:
   _v24_set_zoom(v24_camera_size + 1.5)
   get_viewport().set_input_as_handled()
   return
 super._unhandled_input(event)

func _apply_player_hit(target: Dictionary, base_damage: float, stagger_strength: float = 0.0) -> void:
 var before: float = float(target.get("hp", 0.0))
 super._apply_player_hit(target, base_damage, stagger_strength)
 var dealt: int = ceili(maxf(0.0, before - float(target.get("hp", 0.0))))
 if dealt <= 0 or not hero_playing or not is_instance_valid(accessible_hit_text):return
 var body: CharacterBody3D = target.get("body")
 if not is_instance_valid(body) or not is_instance_valid(player):return
 if body.global_position.distance_squared_to(player.global_position) > 36.0 * 36.0:return
 accessible_hit_text.text = ("Critical hit: " if last_hit_critical else "Hit: ") + str(dealt)
 accessible_hit_text.visible = true
 accessible_hit_remaining = HIT_MESSAGE_LIFETIME
 accessible_hit_count += 1

func _process(delta: float) -> void:
 super._process(delta)
 if accessible_hit_remaining > 0.0:
  accessible_hit_remaining = maxf(0.0, accessible_hit_remaining - maxf(delta, 0.0))
  if accessible_hit_remaining <= 0.0 and is_instance_valid(accessible_hit_text):accessible_hit_text.visible = false
 if is_instance_valid(accessible_hit_text) and (not hero_playing or menu_open):accessible_hit_text.visible = false

func _reset_new_adventure() -> void:
 super._reset_new_adventure()
 accessible_hit_remaining = 0.0
 if is_instance_valid(accessible_hit_text):accessible_hit_text.visible = false
