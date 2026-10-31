extends Node3D

# Isolated prototype. Never modifies existing Moonfall game assets.
const ARENA := 24.0
const BOSS_INTERVAL := 120.0
const MAX_MOBS := 28
const MAX_PICKUPS := 40
var player: CharacterBody3D
var camera: Camera3D
var ui: Label
var overlay: PanelContainer
var mobs: Array[Node3D] = []
var pickups: Array[Node3D] = []
var hero := "Z"
var started := false
var dead := false
var hp := 100.0
var max_hp := 100.0
var xp := 0
var level := 1
var kills := 0
var damage := 25.0
var clock := 0.0
var next_boss := BOSS_INTERVAL
var boss_alive := false
var spawn_wait := 0.0
var attack_wait := 0.0
var dodge_wait := 0.0
var dodge_left := 0.0
var invulnerable := 0.0
var dodge_dir := Vector3.ZERO
var zoom := 1.0
var camera_yaw := 0.0
var camera_pitch := -0.32
const CAMERA_SENSITIVITY := 0.006
var room_index := 0
var room_label: Label
var combat_flash := 0.0
var active_effects: Array[Node3D] = []
var animation_phase := 0.0
var target_marker: MeshInstance3D
var room_waypoints: Array[Vector3] = []
var cached_target: Node3D
var chests: Array[Node3D] = []
var gold := 0
var moon_shards := 0
var craft_heal_latch := false
var craft_mana_latch := false
var dungeon_seed := 0
var inventory: Array[Dictionary] = []
var equipped: Dictionary = {}
var inventory_open := false
var inventory_panel: Label
var equipment_panel: PanelContainer
var equipment_list: VBoxContainer
var equipment_open := false
var inventory_latch := false
var equip_latch := false
var dodge_latch := false
var binds_open := false
var binds_panel: Label
var binding_index := 0
var binding_capture := false
var target_latch := false
var locked_target: Node3D
var jump_velocity := 0.0
var jump_latch := false
var spell_menu: PanelContainer
var spell_list: VBoxContainer
var spell_summary: Label
var spell_preview: Label
var spell_scroll: ScrollContainer
var preview_time := 0.0
var spell_menu_open := false
var pad_menu_latch := false
var pad_select_latch := false
var pad_target_latch := false
var pad_inventory_latch := false
var pad_cursor_latch := false
var pad_camera_speed := 2.1
var pad_move_deadzone := 0.18
var pad_look_deadzone := 0.20
var pad_settings: PanelContainer
var pad_settings_open := false
var options_panel: PanelContainer
var options_open := false
var crafting_panel: PanelContainer
var crafting_open := false
var crafting_info: Label
var start_latch := false
var pad_vibration := false
var mana := 100.0
var max_mana := 100.0
var spell_cursor := 0
var school_cooldowns := [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
var spell_notice := ""
var notice_left := 0.0
var left_spell := 0
var right_spell := 1
var hand_cooldowns := [0.0, 0.0]
var spells := [
    {"name": "Flame", "cost": 16.0, "power": 1.4, "cooldown": 0.8},
    {"name": "Frost", "cost": 13.0, "power": 1.1, "cooldown": 0.65},
    {"name": "Arcane", "cost": 22.0, "power": 1.9, "cooldown": 1.2},
    {"name": "Restoration", "cost": 25.0, "power": 0.0, "cooldown": 2.0},
    {"name": "Storm", "cost": 18.0, "power": 1.6, "cooldown": 1.0},
    {"name": "Venom", "cost": 14.0, "power": 1.2, "cooldown": 0.9}
]
var bindings: Dictionary = {"forward": KEY_W, "back": KEY_S, "left": KEY_A, "right": KEY_D, "jump": KEY_SPACE, "dodge": KEY_Q, "inventory": KEY_B, "interact": KEY_F, "target": KEY_TAB}
const BIND_ORDER := ["forward", "back", "left", "right", "jump", "dodge", "inventory", "interact", "target"]
var selected_item := 0
var armor := 0.0
var equipped_armor: Dictionary = {}
const MAX_INVENTORY := 20
const RARITIES := ["Common", "Uncommon", "Rare", "Epic", "Legendary"]

func _ready() -> void:
    randomize()
    dungeon_seed = randi()
    var ground := MeshInstance3D.new()
    var plane := PlaneMesh.new()
    plane.size = Vector2(60, 60)
    ground.mesh = plane
    var ground_mat := StandardMaterial3D.new()
    ground_mat.albedo_color = Color(0.16, 0.10, 0.22)
    ground.material_override = ground_mat
    add_child(ground)
    var ground_body := StaticBody3D.new()
    ground_body.collision_layer = 2
    ground_body.collision_mask = 0
    var ground_shape := CollisionShape3D.new()
    var ground_box := BoxShape3D.new()
    ground_box.size = Vector3(60, 0.4, 60)
    ground_shape.shape = ground_box
    ground_shape.position.y = -0.2
    ground_body.add_child(ground_shape)
    add_child(ground_body)
    var sun := DirectionalLight3D.new()
    sun.rotation_degrees = Vector3(-55, -35, 0)
    add_child(sun)
    player = CharacterBody3D.new()
    player.collision_layer = 1
    player.collision_mask = 2
    var player_collider := CollisionShape3D.new()
    var player_capsule := CapsuleShape3D.new()
    player_capsule.radius = 0.55
    player_capsule.height = 2.2
    player_collider.shape = player_capsule
    player_collider.position.y = 1.1
    player.add_child(player_collider)
    add_child(player)
    var body := MeshInstance3D.new()
    body.name = "Placeholder"
    var capsule := CapsuleMesh.new()
    capsule.height = 2.2
    capsule.radius = 0.6
    body.mesh = capsule
    body.position.y = 1.1
    player.add_child(body)
    target_marker = MeshInstance3D.new()
    var ring := TorusMesh.new()
    ring.inner_radius = 0.8
    ring.outer_radius = 1.1
    target_marker.mesh = ring
    var ring_material := StandardMaterial3D.new()
    ring_material.albedo_color = Color(1, 0.8, 0.1)
    ring_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
    target_marker.material_override = ring_material
    target_marker.visible = false
    add_child(target_marker)
    camera = Camera3D.new()
    camera.current = true
    add_child(camera)
    camera.position = Vector3(0, 4, 9)
    camera.look_at(Vector3(0, 1.3, 0))
    var layer := CanvasLayer.new()
    add_child(layer)
    ui = Label.new()
    ui.position = Vector2(20, 20)
    ui.add_theme_font_size_override("font_size", 20)
    layer.add_child(ui)
    spell_menu = PanelContainer.new()
    spell_menu.position = Vector2(225, 45)
    spell_menu.custom_minimum_size = Vector2(790, 610)
    spell_menu.visible = false
    layer.add_child(spell_menu)
    var spell_layout := VBoxContainer.new()
    spell_menu.add_child(spell_layout)
    var spell_title := Label.new()
    spell_title.text = "MOONFALL | ARCANE CODEX"
    spell_title.add_theme_font_size_override("font_size", 27)
    spell_layout.add_child(spell_title)
    spell_summary = Label.new()
    spell_layout.add_child(spell_summary)
    spell_preview = Label.new()
    spell_preview.add_theme_font_size_override("font_size", 19)
    spell_layout.add_child(spell_preview)
    spell_scroll = ScrollContainer.new()
    spell_scroll.custom_minimum_size = Vector2(760, 400)
    spell_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
    spell_layout.add_child(spell_scroll)
    spell_list = VBoxContainer.new()
    spell_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
    spell_scroll.add_child(spell_list)
    inventory_panel = Label.new()
    inventory_panel.position = Vector2(20, 65)
    inventory_panel.add_theme_font_size_override("font_size", 18)
    inventory_panel.visible = false
    layer.add_child(inventory_panel)
    binds_panel = Label.new()
    binds_panel.position = Vector2(420, 65)
    binds_panel.add_theme_font_size_override("font_size", 18)
    binds_panel.visible = false
    layer.add_child(binds_panel)
    _load_bindings()
    _build_pad_settings(layer)
    _build_options_menu(layer)
    _build_equipment_menu(layer)
    _build_crafting_menu(layer)
    var center := CenterContainer.new()
    center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
    layer.add_child(center)
    overlay = PanelContainer.new()
    overlay.custom_minimum_size = Vector2(330, 170)
    center.add_child(overlay)
    var choices := VBoxContainer.new()
    overlay.add_child(choices)
    var heading := Label.new()
    heading.text = "MOONFALL 3D | Choose hero"
    choices.add_child(heading)
    for name in ["Z", "Nyra"]:
        var btn := Button.new()
        btn.text = "Play as " + name
        btn.pressed.connect(_choose.bind(name))
        choices.add_child(btn)
    _build_room()
    _build_solid_dungeon()
    _spawn_chests()
    _hud()

func _choose(name: String) -> void:
    hero = name
    started = true
    overlay.visible = false
    var mat := StandardMaterial3D.new()
    mat.albedo_color = Color(0.3, 0.6, 1.0) if name == "Z" else Color(0.8, 0.3, 0.95)
    (player.get_node("Placeholder") as MeshInstance3D).material_override = mat

func _unhandled_input(event: InputEvent) -> void:
    if event is InputEventKey and event.pressed and not event.echo:
        if binding_capture:
            if event.keycode == KEY_ESCAPE:
                binding_capture = false
            else:
                var action: String = BIND_ORDER[binding_index]
                if not _key_in_use(event.keycode, action):
                    bindings[action] = event.keycode
                    _save_bindings()
                binding_capture = false
            _render_binds()
            get_viewport().set_input_as_handled()
            return
        if event.keycode == KEY_ESCAPE and started:
            if options_open:
                _close_all_options()
            elif spell_menu_open or pad_settings_open or binds_open or inventory_open or crafting_open or equipment_open:
                _close_all_options()
                _open_options_menu()
            else:
                _open_options_menu()
            get_viewport().set_input_as_handled()
            return
        if options_open:
            get_viewport().set_input_as_handled()
            return
        if event.keycode == KEY_F9 and started and not binds_open:
            pad_settings_open = not pad_settings_open
            pad_settings.visible = pad_settings_open
            get_viewport().set_input_as_handled()
            return
        if event.keycode == KEY_M and started and not binds_open and not pad_settings_open:
            spell_menu_open = not spell_menu_open
            spell_menu.visible = spell_menu_open
            _render_spell_menu()
            get_viewport().set_input_as_handled()
            return
        if spell_menu_open:
            if event.keycode == KEY_UP:
                spell_cursor = posmod(spell_cursor - 1, spells.size())
            elif event.keycode == KEY_DOWN:
                spell_cursor = (spell_cursor + 1) % spells.size()
            elif event.keycode == KEY_Z:
                left_spell = spell_cursor
            elif event.keycode == KEY_X:
                right_spell = spell_cursor
            if event.keycode >= KEY_1 and event.keycode <= KEY_4:
                left_spell = event.keycode - KEY_1
            elif event.keycode >= KEY_5 and event.keycode <= KEY_8:
                right_spell = event.keycode - KEY_5
            elif event.keycode == KEY_ESCAPE:
                spell_menu_open = false
                spell_menu.visible = false
            _render_spell_menu()
            get_viewport().set_input_as_handled()
            return
        if event.keycode == KEY_F10 and started:
            binds_open = not binds_open
            binds_panel.visible = binds_open
            _render_binds()
            get_viewport().set_input_as_handled()
            return
        if binds_open:
            if event.keycode == KEY_UP:
                binding_index = posmod(binding_index - 1, BIND_ORDER.size())
            elif event.keycode == KEY_DOWN:
                binding_index = (binding_index + 1) % BIND_ORDER.size()
            elif event.keycode == KEY_ENTER:
                binding_capture = true
            elif event.keycode == KEY_ESCAPE:
                binds_open = false
                binds_panel.visible = false
            _render_binds()
            get_viewport().set_input_as_handled()
            return
    if not started or dead:
        return
    if event is InputEventMouseButton and event.pressed:
        if event.button_index == MOUSE_BUTTON_WHEEL_UP:
            zoom = clampf(zoom - 0.1, 0.55, 1.8)
        elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
            zoom = clampf(zoom + 0.1, 0.55, 1.8)
    elif event is InputEventMouseMotion:
        if Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT) or Input.is_mouse_button_pressed(MOUSE_BUTTON_MIDDLE):
            camera_yaw -= event.relative.x * CAMERA_SENSITIVITY
            camera_pitch = clampf(camera_pitch - event.relative.y * CAMERA_SENSITIVITY, -1.25, -0.1)
            if Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT):
                player.rotation.y = camera_yaw

func _process(dt: float) -> void:
    _controller_menus()
    _controller_start_menu()
    if dead and Input.is_key_pressed(KEY_R):
        get_tree().reload_current_scene()
        return
    if spell_menu_open:
        preview_time += dt
        _animate_spell_preview()
    if not started or dead or binds_open or spell_menu_open or pad_settings_open or options_open or inventory_open or crafting_open or equipment_open:
        return
    var look := Vector2.ZERO
    if not Input.get_connected_joypads().is_empty():
        var pad: int = Input.get_connected_joypads()[0]
        look = Vector2(Input.get_joy_axis(pad, JOY_AXIS_RIGHT_X), Input.get_joy_axis(pad, JOY_AXIS_RIGHT_Y))
        if look.length() < pad_look_deadzone:
            look = Vector2.ZERO
    camera_yaw -= look.x * pad_camera_speed * dt
    camera_pitch = clampf(camera_pitch - look.y * pad_camera_speed * dt, -1.25, -0.1)
    clock += dt
    mana = minf(max_mana, mana + 7.0 * dt)
    notice_left = maxf(0.0, notice_left - dt)
    for school in range(school_cooldowns.size()):
        school_cooldowns[school] = maxf(0.0, school_cooldowns[school] - dt)
    for hand in range(hand_cooldowns.size()):
        hand_cooldowns[hand] = maxf(0.0, hand_cooldowns[hand] - dt)
    if Input.is_key_pressed(KEY_C) or _pad_button(JOY_BUTTON_LEFT_SHOULDER) and _pad_button(JOY_BUTTON_RIGHT_SHOULDER):
        _dual_cast()
    else:
        if Input.is_key_pressed(KEY_Z) or _pad_button(JOY_BUTTON_LEFT_SHOULDER):
            _cast_hand(0)
        if Input.is_key_pressed(KEY_X) or _pad_button(JOY_BUTTON_RIGHT_SHOULDER):
            _cast_hand(1)
    var inventory_key := _pressed("inventory") or _pad_button(JOY_BUTTON_BACK)
    if inventory_key and not inventory_latch:
        inventory_open = not inventory_open
        inventory_panel.visible = inventory_open
        _refresh_inventory()
    inventory_latch = inventory_key
    var target_button := _pad_button(JOY_BUTTON_RIGHT_STICK)
    if target_button and not pad_target_latch:
        _next_target()
    pad_target_latch = target_button
    var equip_key := Input.is_key_pressed(KEY_G)
    if inventory_open and equip_key and not equip_latch and not inventory.is_empty():
        selected_item = (selected_item + 1) % inventory.size()
        _equip_item(selected_item)
    equip_latch = equip_key
    var craft_heal := inventory_open and Input.is_key_pressed(KEY_H)
    if craft_heal and not craft_heal_latch:
        _craft_upgrade(false)
    craft_heal_latch = craft_heal
    var craft_mana := inventory_open and Input.is_key_pressed(KEY_J)
    if craft_mana and not craft_mana_latch:
        _craft_upgrade(true)
    craft_mana_latch = craft_mana
    # Three traversable regions, divided by walls with central doorways.
    var entered_room := _region_for_z(player.position.z)
    if entered_room != room_index:
        room_index = entered_room
        _build_room()
    combat_flash = maxf(0.0, combat_flash - dt)
    for effect in active_effects.duplicate():
        if not is_instance_valid(effect):
            active_effects.erase(effect)
            continue
        var life: float = float(effect.get_meta("life")) - dt
        if life <= 0.0:
            active_effects.erase(effect)
            effect.queue_free()
        else:
            effect.set_meta("life", life)
            effect.scale = Vector3.ONE * (1.0 + (0.3 - life) * 2.0)
            if effect.get_meta("spin", false):
                effect.rotate_y(dt * 7.0)
            if effect.has_meta("cast_hand"):
                var age: float = float(effect.get_meta("cast_age")) + dt
                effect.set_meta("cast_age", age)
                var hand: int = int(effect.get_meta("cast_hand"))
                var side := -0.7 if hand == 0 else 0.7
                var arc := sin(minf(age / 0.25, 1.0) * PI) * 0.4
                effect.position = player.position + player.global_transform.basis.x * side + Vector3.UP * (1.2 + arc)

    attack_wait = maxf(0.0, attack_wait - dt)
    var jump_key := _pressed("jump") or _pad_button(JOY_BUTTON_A)
    if jump_key and not jump_latch and player.is_on_floor():
        jump_velocity = 9.0
    jump_latch = jump_key
    if not player.is_on_floor():
        jump_velocity -= 25.0 * dt
    elif jump_velocity < 0.0:
        jump_velocity = 0.0
    dodge_wait = maxf(0.0, dodge_wait - dt)
    dodge_left = maxf(0.0, dodge_left - dt)
    invulnerable = maxf(0.0, invulnerable - dt)
    var motion := Vector3.ZERO
    if _pressed("left") or Input.is_key_pressed(KEY_LEFT):
        motion.x -= 1
    if _pressed("right") or Input.is_key_pressed(KEY_RIGHT):
        motion.x += 1
    if _pressed("forward") or Input.is_key_pressed(KEY_UP):
        motion.z -= 1
    if _pressed("back") or Input.is_key_pressed(KEY_DOWN):
        motion.z += 1
    var stick := Vector2.ZERO
    if not Input.get_connected_joypads().is_empty():
        var pad: int = Input.get_connected_joypads()[0]
        stick = Vector2(Input.get_joy_axis(pad, JOY_AXIS_LEFT_X), Input.get_joy_axis(pad, JOY_AXIS_LEFT_Y))
        if stick.length() < pad_move_deadzone:
            stick = Vector2.ZERO
    motion += Vector3(stick.x, 0.0, stick.y)
    motion = motion.limit_length(1.0)
    # Camera-relative movement, with right-drag turning the character.
    motion = motion.rotated(Vector3.UP, camera_yaw)
    var dodge_pressed := _pressed("dodge") or _pad_button(JOY_BUTTON_B)
    if dodge_pressed and not dodge_latch and dodge_wait <= 0.0:
        dodge_dir = motion if motion.length() > 0.0 else Vector3.FORWARD
        dodge_left = 0.24
        dodge_wait = 1.1
        invulnerable = 0.35
    dodge_latch = dodge_pressed
    var velocity := dodge_dir * 18.0 if dodge_left > 0.0 else motion * 7.0
    animation_phase += dt * (11.0 if velocity.length() > 0.0 else 3.0)
    # Placeholder animation only; replace when original-design character models arrive.
    var placeholder := player.get_node("Placeholder") as MeshInstance3D
    placeholder.position.y = 1.1 + (absf(sin(animation_phase)) * 0.12 if velocity.length() > 0.0 else sin(animation_phase) * 0.025)
    player.velocity = velocity + Vector3.UP * jump_velocity
    player.move_and_slide()
    if player.is_on_floor() and jump_velocity < 0.0:
        jump_velocity = 0.0
    # Normal movement turns to travel direction, except while right-drag turning.
    if velocity.length() > 0.0 and not Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT):
        player.rotation.y = atan2(velocity.x, velocity.z)
    var radius := 9.0 * zoom
    var horizontal := cos(camera_pitch) * radius
    var follow_offset := Vector3(sin(camera_yaw) * horizontal, -sin(camera_pitch) * radius + 1.3, cos(camera_yaw) * horizontal)
    var focus := player.position + Vector3.UP * 1.3
    var desired_camera := player.position + follow_offset
    # Pull camera forward when scene collision obstructs the third-person view.
    var ray := PhysicsRayQueryParameters3D.create(focus, desired_camera)
    ray.collision_mask = 2
    var hit := get_world_3d().direct_space_state.intersect_ray(ray)
    if not hit.is_empty():
        var near_hit: Vector3 = hit["position"]
        desired_camera = near_hit + (focus - near_hit).normalized() * 0.35
    camera.position = camera.position.lerp(desired_camera, minf(1.0, dt * 6.0))
    camera.look_at(focus)
    var target_key := _pressed("target")
    if target_key and not target_latch:
        locked_target = _next_target()
    target_latch = target_key
    cached_target = _select_target()
    target_marker.visible = cached_target != null
    if cached_target != null:
        target_marker.position = cached_target.position + Vector3.UP * 0.08
    if _pressed("interact"):
        _open_nearby_chest()
    if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT) and attack_wait <= 0.0:
        _attack()
    spawn_wait -= dt
    if spawn_wait <= 0.0 and mobs.size() < MAX_MOBS:
        _spawn(false)
        spawn_wait = maxf(0.5, 2.2 - float(kills) * 0.025)
    if clock >= next_boss and not boss_alive:
        _spawn(true)
        next_boss = clock + BOSS_INTERVAL
    for mob in mobs.duplicate():
        if not is_instance_valid(mob):
            continue
        var burn: float = maxf(0.0, float(mob.get_meta("burn", 0.0)) - dt)
        var venom: float = maxf(0.0, float(mob.get_meta("venom", 0.0)) - dt)
        var chill: float = maxf(0.0, float(mob.get_meta("chill", 0.0)) - dt)
        mob.set_meta("burn", burn)
        mob.set_meta("venom", venom)
        mob.set_meta("chill", chill)
        if burn > 0.0 or venom > 0.0:
            var dot := (6.0 if burn > 0.0 else 0.0) + (3.5 if venom > 0.0 else 0.0)
            var tick_hp := float(mob.get_meta("hp")) - dot * dt
            mob.set_meta("hp", tick_hp)
            if tick_hp <= 0.0:
                _defeat_mob(mob)
                continue
        var destination := player.position
        # Agents in separate regions aim for the doorway before pursuing the player.
        var mob_region := _region_for_z(mob.position.z)
        var player_region := _region_for_z(player.position.z)
        if mob_region != player_region:
            var door_z := -8.0 if mini(mob_region, player_region) == 0 else 8.0
            destination = Vector3(0, 0, door_z)
            if absf(mob.position.z - door_z) < 2.0 and absf(mob.position.x) < 3.5:
                destination = player.position
        var diff: Vector3 = destination - mob.position
        diff.y = 0.0
        var boss: bool = mob.get_meta("boss")
        var phase: int = int(mob.get_meta("phase", 1))
        if boss:
            var health_fraction: float = float(mob.get_meta("hp")) / maxf(1.0, float(mob.get_meta("max_hp")))
            var new_phase := 3 if health_fraction <= 0.30 else (2 if health_fraction <= 0.65 else 1)
            if new_phase > phase:
                phase = new_phase
                mob.set_meta("phase", phase)
                _school_effect(mob.position, 4 if phase == 3 else 2)
                _cast_notice("BOSS PHASE %d!" % phase)
        var reach := 2.8 if boss else 1.7
        if diff.length() > reach:
            var steering: Vector3 = diff.normalized()
            # Different enemy movement patterns; no collision-pathfinding claim.
            if not boss and int(mob.get_meta("kind")) == 1 and diff.length() < 6.0:
                steering = Vector3(-steering.z, 0, steering.x) * 0.65 * float(mob.get_meta("strafe_sign", 1.0)) + steering * 0.35
            # Raycast ahead to steer around solid obstacles.
            var travel: Vector3 = steering.normalized()
            var probe := PhysicsRayQueryParameters3D.create(mob.position + Vector3.UP, mob.position + Vector3.UP + travel * 3.0)
            probe.collision_mask = 2
            var obstacle := get_world_3d().direct_space_state.intersect_ray(probe)
            if not obstacle.is_empty():
                var left_dir := Vector3(-travel.z, 0, travel.x)
                var right_dir := -left_dir
                var left_ray := PhysicsRayQueryParameters3D.create(mob.position + Vector3.UP, mob.position + Vector3.UP + left_dir * 3.0)
                left_ray.collision_mask = 2
                var right_ray := PhysicsRayQueryParameters3D.create(mob.position + Vector3.UP, mob.position + Vector3.UP + right_dir * 3.0)
                right_ray.collision_mask = 2
                travel = left_dir if get_world_3d().direct_space_state.intersect_ray(left_ray).is_empty() else right_dir
            # Simple separation avoids piling every enemy onto the same point.
            var separation := Vector3.ZERO
            for other in mobs:
                if other == mob or not is_instance_valid(other):
                    continue
                var apart: Vector3 = mob.position - other.position
                apart.y = 0.0
                var distance_sq: float = apart.length_squared()
                if distance_sq > 0.01 and distance_sq < 3.24:
                    separation += apart.normalized() * (1.8 - sqrt(distance_sq))
            travel = (travel + separation * 0.6).normalized()
            mob.position += travel * (1.9 if boss else 2.3) * (0.65 if chill > 0.0 else 1.0) * dt
            mob.position.x = clampf(mob.position.x, -ARENA, ARENA)
            mob.position.z = clampf(mob.position.z, -ARENA, ARENA)
        var ability_cd: float = maxf(0.0, float(mob.get_meta("ability_cd")) - dt)
        var windup: float = maxf(0.0, float(mob.get_meta("windup", 0.0)) - dt)
        var actual_distance: float = mob.position.distance_to(player.position)
        if float(mob.get_meta("windup", 0.0)) > 0.0 and windup <= 0.0:
            # Boss attacks resolve at the previously telegraphed location.
            var impact: Vector3 = mob.get_meta("attack_position")
            var pattern: int = int(mob.get_meta("attack_kind"))
            _spell_effect(impact, false)
            var safe_distance := 3.1 if pattern == 0 else 1.8
            if player.position.distance_to(impact) < safe_distance and invulnerable <= 0.0 and _has_clear_path(mob.position + Vector3.UP, player.position + Vector3.UP):
                hp = maxf(0.0, hp - maxf(1.0, (19.0 if pattern == 0 else 13.0) - armor))
                dead = hp <= 0.0
        if boss and ability_cd <= 0.0 and windup <= 0.0 and actual_distance < 8.0 and actual_distance > reach and _has_clear_path(mob.position + Vector3.UP, player.position + Vector3.UP):
            var kind: int = randi_range(0, 1)
            if phase >= 3:
                kind = 1
            mob.set_meta("attack_kind", kind)
            mob.set_meta("attack_position", player.position if kind == 0 else mob.position + (player.position - mob.position).normalized() * 4.0)
            windup = (1.1 if kind == 0 else 0.75) * (0.75 if phase >= 3 else (0.9 if phase == 2 else 1.0))
            ability_cd = (5.0 if kind == 0 else 3.5) * (0.65 if phase >= 3 else (0.82 if phase == 2 else 1.0))
            _boss_warning(mob.get_meta("attack_position"), kind)
        elif not boss and ability_cd <= 0.0 and actual_distance < 7.0 and actual_distance > reach and _has_clear_path(mob.position + Vector3.UP, player.position + Vector3.UP):
            if int(mob.get_meta("kind")) == 1:
                var retreat: Vector3 = mob.position - player.position
                retreat.y = 0.0
                if retreat.length_squared() > 0.01:
                    mob.position += retreat.normalized() * minf(2.0, dt * 6.0)
            elif int(mob.get_meta("kind")) == 2:
                # Chargers close the gap in a short bounded dash.
                var rush: Vector3 = player.position - mob.position
                rush.y = 0.0
                if rush.length_squared() > 0.1:
                    var step: Vector3 = rush.normalized() * minf(2.5, rush.length())
                    if _has_clear_path(mob.position + Vector3.UP, mob.position + step + Vector3.UP):
                        mob.position += step
                        mob.position.x = clampf(mob.position.x, -ARENA, ARENA)
                        mob.position.z = clampf(mob.position.z, -ARENA, ARENA)
            ability_cd = 4.0
        mob.set_meta("windup", windup)
        mob.set_meta("ability_cd", ability_cd)
        var hit_cd: float = maxf(0.0, float(mob.get_meta("hit_cd")) - dt)
        # Skirmishers circle at close range; bruisers pursue directly.
        # Damage requires actual proximity, not proximity to the doorway waypoint.
        if mob.position.distance_to(player.position) <= reach + 0.3 and hit_cd <= 0.0 and invulnerable <= 0.0 and _has_clear_path(mob.position + Vector3.UP, player.position + Vector3.UP):
            hp -= maxf(1.0, (25.0 if boss else 12.0) - armor)
            hit_cd = 1.1
            if hp <= 0:
                hp = 0
                dead = true
        mob.set_meta("hit_cd", hit_cd)
        if dead:
            break
    for pickup in pickups.duplicate():
        if not is_instance_valid(pickup):
            continue
        if pickup.position.distance_to(player.position) < 1.7:
            hp = minf(max_hp, hp + 22.0)
            pickups.erase(pickup)
            pickup.queue_free()
    _hud()

func _spawn(boss: bool) -> void:
    var mob := Node3D.new()
    var body := MeshInstance3D.new()
    var shape := SphereMesh.new()
    shape.radius = 1.6 if boss else 0.8
    shape.height = 3.2 if boss else 1.6
    body.mesh = shape
    body.position.y = 1.6 if boss else 0.8
    var mat := StandardMaterial3D.new()
    var mob_kind := randi_range(0, 2)
    mat.albedo_color = Color(1.0, 0.25, 0.75) if boss else [Color(0.9, 0.2, 0.25), Color(0.4, 0.75, 0.9), Color(0.95, 0.65, 0.18)][mob_kind]
    body.material_override = mat
    mob.add_child(body)
    var angle := randf() * TAU
    mob.position = player.position + Vector3(cos(angle) * 8.0, 0, sin(angle) * 5.0)
    mob.position.x = clampf(mob.position.x, -ARENA, ARENA)
    mob.position.z = clampf(mob.position.z, -ARENA, ARENA)
    # Avoid placing enemies inside dividing walls.
    if absf(absf(mob.position.z) - 8.0) < 2.0 and absf(mob.position.x) > 3.0:
        mob.position.z += -3.0 if mob.position.z > 0 else 3.0
    mob.set_meta("hp", 600.0 + level * 70.0 if boss else 45.0 + level * 10.0)
    mob.set_meta("boss", boss)
    mob.set_meta("max_hp", 600.0 + level * 70.0 if boss else 45.0 + level * 10.0)
    mob.set_meta("phase", 1)
    mob.set_meta("kind", mob_kind)
    mob.set_meta("hit_cd", 0.0)
    mob.set_meta("ability_cd", randf_range(3.0, 6.0))
    mob.set_meta("strafe_sign", -1.0 if randf() < 0.5 else 1.0)
    mob.set_meta("windup", 0.0)
    mob.set_meta("attack_position", Vector3.ZERO)
    mob.set_meta("attack_kind", 0)
    add_child(mob)
    mobs.append(mob)
    if boss:
        boss_alive = true

func _attack() -> void:
    attack_wait = 0.25 if hero == "Z" else 0.38
    var target: Node3D = _select_target()
    if target == null:
        return
    _spell_effect(target.position, hero == "Nyra")
    var left: float = float(target.get_meta("hp")) - damage
    target.set_meta("hp", left)
    if left > 0.0:
        return
    _defeat_mob(target)

func _defeat_mob(target: Node3D) -> void:
    if not is_instance_valid(target) or not mobs.has(target):
        return
    var was_boss: bool = target.get_meta("boss")
    var death_position := target.position
    mobs.erase(target)
    target.queue_free()
    if was_boss:
        boss_alive = false
    kills += 1
    moon_shards += 4 if was_boss else (1 if randf() < 0.4 else 0)
    xp += 10 if not was_boss else 100
    while xp >= level * 60:
        xp -= level * 60
        level += 1
        max_hp += 12.0
        hp = minf(max_hp, hp + 12.0)
        damage += 5.0
        max_mana += 6.0
        mana = minf(max_mana, mana + 6.0)
        _cast_notice("LEVEL %d! HP, damage and mana increased" % level)
    if was_boss:
        gold += 100 + level * 10
        _loot_equipment()
    else:
        gold += randi_range(1, 5) + level
        if randf() < 0.12:
            _loot_equipment()
    if (randf() < 0.3 or was_boss) and pickups.size() < MAX_PICKUPS:
        var pickup := Node3D.new()
        var shape := MeshInstance3D.new()
        var sphere := SphereMesh.new()
        sphere.radius = 0.5
        sphere.height = 1.0
        shape.mesh = sphere
        shape.position.y = 0.5
        var green := StandardMaterial3D.new()
        green.albedo_color = Color(0.2, 1.0, 0.35)
        shape.material_override = green
        pickup.add_child(shape)
        pickup.position = death_position
        add_child(pickup)
        pickups.append(pickup)

func _hud() -> void:
    ui.text = "%s   Room: %d   HP: %d/%d   Level: %d   XP: %d   Kills: %d   DMG: %d   Gold: %d   Gear: %d/%d   [F] Chest [B] Bag [F10] Keys" % [hero, room_index + 1, int(hp), int(max_hp), level, xp, kills, int(damage), gold, inventory.size(), MAX_INVENTORY]
    if dead:
        ui.text += "\nDefeated. Press R to restart."

# Decorative room changes every 15 kills. The arena stays connected and traversable.
func _build_room() -> void:
    var old := get_node_or_null("RoomDecor")
    if old != null:
        old.queue_free()
    var decor := Node3D.new()
    decor.name = "RoomDecor"
    room_waypoints.clear()
    for direction in [Vector3(18, 0, 0), Vector3(-18, 0, 0), Vector3(0, 0, 18), Vector3(0, 0, -18)]:
        room_waypoints.append(direction)
    add_child(decor)
    var tint: Color = [Color(0.3, 0.12, 0.4), Color(0.22, 0.45, 0.55), Color(0.65, 0.2, 0.15)][room_index % 3]
    # Room-specific landmark clusters give each existing dungeon region an identity.
    for landmark in range(3):
        var monument := MeshInstance3D.new()
        var mon_mesh := BoxMesh.new()
        mon_mesh.size = Vector3(1.5, 1.0 + float(room_index) * 0.65, 1.5)
        monument.mesh = mon_mesh
        var mon_mat := StandardMaterial3D.new()
        mon_mat.albedo_color = tint.lightened(0.2)
        monument.material_override = mon_mat
        monument.position = Vector3(-9.0 + landmark * 9.0, mon_mesh.size.y * 0.5, -17.0 + room_index * 17.0)
        decor.add_child(monument)
    for i in range(12):
        var pillar := MeshInstance3D.new()
        var mesh := CylinderMesh.new()
        mesh.top_radius = 0.65
        mesh.bottom_radius = 0.9
        mesh.height = 4.0
        pillar.mesh = mesh
        var mat := StandardMaterial3D.new()
        mat.albedo_color = tint
        pillar.material_override = mat
        var a := float(i) * TAU / 12.0
        pillar.position = Vector3(cos(a) * 29.0, 2.0, sin(a) * 29.0)
        decor.add_child(pillar)

func _spell_effect(at: Vector3, nyra: bool) -> void:
    var effect := Node3D.new()
    var orb := MeshInstance3D.new()
    var mesh := SphereMesh.new()
    mesh.radius = 0.55 if nyra else 0.9
    mesh.height = 1.1 if nyra else 1.8
    orb.mesh = mesh
    var mat := StandardMaterial3D.new()
    mat.albedo_color = Color(0.5, 0.2, 1.0) if nyra else Color(1.0, 0.32, 0.1)
    mat.emission_enabled = true
    mat.emission = mat.albedo_color
    orb.material_override = mat
    effect.add_child(orb)
    effect.position = at + Vector3.UP
    effect.set_meta("life", 0.3)
    add_child(effect)
    active_effects.append(effect)

# Select in-range targets in front of the character preferentially.
# This does not implement mouse cursor picking or lock-on yet.
func _select_target() -> Node3D:
    var max_range := 5.2 if hero == "Z" else 8.0
    if is_instance_valid(locked_target) and locked_target != null and player.position.distance_to(locked_target.position) <= max_range:
        return locked_target
    var chosen: Node3D = null
    var best_score := INF
    var facing := -player.global_transform.basis.z
    for mob in mobs:
        if not is_instance_valid(mob):
            continue
        if not _has_clear_path(player.position + Vector3.UP, mob.position + Vector3.UP):
            continue
        var toward := mob.position - player.position
        toward.y = 0.0
        var distance := toward.length()
        if distance > max_range or distance < 0.01:
            continue
        var facing_score := facing.dot(toward.normalized())
        var score := distance + (1.0 - facing_score) * 3.0
        if score < best_score:
            best_score = score
            chosen = mob
    return chosen

# Solid arena boundaries and four interior obstacles.
# Physics layer 2 is world geometry; these meshes remain separate from original artwork.
func _build_solid_dungeon() -> void:
    var walls := Node3D.new()
    walls.name = "SolidDungeon"
    add_child(walls)
    _solid_block(walls, Vector3(0, 2, -25), Vector3(52, 4, 2))
    _solid_block(walls, Vector3(0, 2, 25), Vector3(52, 4, 2))
    _solid_block(walls, Vector3(-25, 2, 0), Vector3(2, 4, 52))
    _solid_block(walls, Vector3(25, 2, 0), Vector3(2, 4, 52))
    # Divide the arena into three walkable rooms. Each partition has an 8-unit doorway.
    for dividing_z in [-8.0, 8.0]:
        _solid_block(walls, Vector3(-14.0, 2.0, dividing_z), Vector3(20.0, 4.0, 1.5))
        _solid_block(walls, Vector3(14.0, 2.0, dividing_z), Vector3(20.0, 4.0, 1.5))
    # Seeded variations in obstacle size/position while retaining center doorways.
    var rng := RandomNumberGenerator.new()
    rng.seed = dungeon_seed
    for z in [-17.0, 0.0, 17.0]:
        var x := (-1.0 if rng.randi_range(0, 1) == 0 else 1.0) * rng.randf_range(12.0, 17.0)
        _solid_block(walls, Vector3(x, 1.5, z), Vector3(2.5, 3.0, 2.5))

func _solid_block(parent: Node3D, location: Vector3, dimensions: Vector3) -> void:
    var block := StaticBody3D.new()
    block.collision_layer = 2
    block.collision_mask = 0
    block.position = location
    var collision := CollisionShape3D.new()
    var shape := BoxShape3D.new()
    shape.size = dimensions
    collision.shape = shape
    block.add_child(collision)
    var visual := MeshInstance3D.new()
    var mesh := BoxMesh.new()
    mesh.size = dimensions
    visual.mesh = mesh
    var mat := StandardMaterial3D.new()
    mat.albedo_color = Color(0.24, 0.21, 0.31)
    visual.material_override = mat
    block.add_child(visual)
    parent.add_child(block)

# Region identifiers for the three connected dungeon spaces.
func _region_for_z(z: float) -> int:
    if z < -8.0:
        return 0
    if z > 8.0:
        return 2
    return 1

# Three chests per run, one in each connected room.
func _spawn_chests() -> void:
    for pos in [Vector3(0, 0, -17), Vector3(0, 0, 0), Vector3(0, 0, 17), Vector3(-12, 0, -15), Vector3(12, 0, 15)]:
        var chest := Node3D.new()
        var mesh_node := MeshInstance3D.new()
        var mesh := BoxMesh.new()
        mesh.size = Vector3(1.4, 1.0, 1.0)
        mesh_node.mesh = mesh
        mesh_node.position.y = 0.5
        var mat := StandardMaterial3D.new()
        mat.albedo_color = Color(0.75, 0.52, 0.12)
        mesh_node.material_override = mat
        chest.add_child(mesh_node)
        chest.position = pos
        chest.set_meta("greater", absf(pos.x) > 1.0)
        if bool(chest.get_meta("greater")):
            mat.albedo_color = Color(0.45, 0.7, 0.95)
        add_child(chest)
        chests.append(chest)

func _open_nearby_chest() -> void:
    for chest in chests.duplicate():
        if not is_instance_valid(chest) or player.position.distance_to(chest.position) > 2.5:
            continue
        var greater: bool = bool(chest.get_meta("greater", false))
        gold += randi_range(30, 65) if greater else randi_range(10, 35)
        moon_shards += randi_range(3, 6) if greater else randi_range(1, 3)
        _loot_equipment(2 if greater else 0)
        chests.erase(chest)
        chest.queue_free()
        break

# First inventory pass: chests yield equipment with weighted rarities.
# Best found weapon auto-equips. A manual equip interface can follow.
func _loot_equipment(min_rarity: int = 0) -> void:
    if inventory.size() >= MAX_INVENTORY:
        gold += 15
        return
    var roll := randf()
    var rarity_index := 0
    if roll > 0.995:
        rarity_index = 4
    elif roll > 0.965:
        rarity_index = 3
    elif roll > 0.85:
        rarity_index = 2
    elif roll > 0.55:
        rarity_index = 1
    rarity_index = maxi(rarity_index, mini(min_rarity, 4))
    var slot := "Armor" if randi_range(0, 1) == 0 else "Weapon"
    var item := {
        "name": (["Worn Blade", "Runed Sword", "Moonsteel Edge", "Astral Fang", "Eclipse Crownblade"] if slot == "Weapon" else ["Worn Guard", "Runed Vest", "Moonsteel Plate", "Astral Ward", "Eclipse Aegis"])[rarity_index],
        "rarity": RARITIES[rarity_index],
        "power": (rarity_index + 1) * 3 + maxi(0, level - 1),
        "slot": slot
    }
    inventory.append(item)
    if item["slot"] == "Weapon" and (equipped.is_empty() or int(item["power"]) > int(equipped.get("power", 0))):
        _equip_item(inventory.size() - 1)
    _refresh_inventory()

func _refresh_inventory() -> void:
    if inventory_panel == null:
        return
    var lines := PackedStringArray()
    lines.append("INVENTORY  [B] Close  [G] Equip next")
    lines.append("Armor: %d | Gold: %d | Shards: %d | Level: %d" % [int(armor), gold, moon_shards, level])
    lines.append("Equipped armor: %s" % str(equipped_armor.get("name", "None")))
    lines.append("CRAFT [H] Vitality (+10 max HP): 50 gold + 3 shards")
    lines.append("CRAFT [J] Focus (+10 max mana): 50 gold + 3 shards")
    if equipped.is_empty():
        lines.append("Equipped: None")
    else:
        lines.append("Equipped: %s (+%d)" % [equipped["name"], equipped["power"]])
    for item in inventory:
        lines.append("%s  %s (%s)  +%d" % [item["rarity"], item["name"], item["slot"], item["power"]])
    inventory_panel.text = "\n".join(lines)

func _equip_item(index: int) -> void:
    if index < 0 or index >= inventory.size():
        return
    var item: Dictionary = inventory[index]
    if item["slot"] == "Weapon":
        damage += float(int(item["power"]) - int(equipped.get("power", 0)))
        equipped = item.duplicate()
    else:
        equipped_armor = item.duplicate()
        armor = float(item["power"])
    _refresh_inventory()
    if equipment_open:
        _refresh_equipment_menu()

func _pressed(action: String) -> bool:
    return Input.is_key_pressed(int(bindings[action]))

func _render_binds() -> void:
    if binds_panel == null:
        return
    var lines := PackedStringArray(["KEYBINDS [F10] Close", "Up/Down: select | Enter: remap | Esc: cancel"])
    for i in range(BIND_ORDER.size()):
        var action: String = BIND_ORDER[i]
        var prefix := "> " if i == binding_index else "  "
        var key := int(bindings[action])
        lines.append("%s%s : %s" % [prefix, action, OS.get_keycode_string(key)])
    if binding_capture:
        lines.append("Press a new key, or Esc to cancel")
    binds_panel.text = "\n".join(lines)

func _save_bindings() -> void:
    var file := FileAccess.open("user://moonfall_keybinds.json", FileAccess.WRITE)
    if file != null:
        var saved := bindings.duplicate()
        file.store_string(JSON.stringify(saved))

func _load_bindings() -> void:
    if not FileAccess.file_exists("user://moonfall_keybinds.json"):
        return
    var file := FileAccess.open("user://moonfall_keybinds.json", FileAccess.READ)
    if file == null:
        return
    var parsed = JSON.parse_string(file.get_as_text())
    if not parsed is Dictionary:
        return
    for action in BIND_ORDER:
        if not parsed.has(action) or int(parsed[action]) <= 0:
            continue
        bindings[action] = int(parsed[action])
    # Migrate saved profiles: Space was dodge; now Space must jump.
    if not parsed.has("jump") and int(bindings["dodge"]) == KEY_SPACE:
        bindings["jump"] = KEY_SPACE
        bindings["dodge"] = KEY_Q
    _save_bindings()

func _next_target() -> Node3D:
    var candidates: Array[Node3D] = []
    for mob in mobs:
        if is_instance_valid(mob) and player.position.distance_to(mob.position) <= 12.0:
            candidates.append(mob)
    if candidates.is_empty():
        return null
    candidates.sort_custom(func(a: Node3D, b: Node3D) -> bool:
        return a.position.distance_to(player.position) < b.position.distance_to(player.position))
    var index := candidates.find(locked_target)
    return candidates[(index + 1) % candidates.size()]

# Twelve visible action slots. Every slot uses the current placeholder attack,
# with three distinct variants and individual cooldowns. Future abilities can replace these.
func _key_in_use(key: int, except_action: String) -> bool:
    for action in bindings:
        if action != except_action and int(bindings[action]) == key:
            return true
    return key == KEY_F10 or key == KEY_ESCAPE

func _render_spell_menu() -> void:
    if spell_menu == null:
        return
    spell_summary.text = "Mana %d/%d | LEFT: %s | RIGHT: %s\nSelect a spell, then choose LEFT or RIGHT. [M] Close" % [int(mana), int(max_mana), spells[left_spell]["name"], spells[right_spell]["name"]]
    var selected: Dictionary = spells[spell_cursor]
    var status: String = ["Burning", "Chilling", "Arcane burst", "Healing", "Shock", "Poisoning"][spell_cursor]
    spell_preview.text = "SELECTED: %s  |  %s\nMana: %d  |  Power: %.1fx  |  Cooldown: %.2fs" % [selected["name"], status, int(selected["cost"]), float(selected["power"]), float(selected["cooldown"])]
    for child in spell_list.get_children():
        spell_list.remove_child(child)
        child.queue_free()
    var groups := ["Destruction", "Destruction", "Mysticism", "Restoration", "Destruction", "Alchemy"]
    var descriptions := [
        "Ignites enemies: 4 seconds of burning damage.",
        "Chills enemies: reduces movement for 3.5 seconds.",
        "Heavy direct magical damage; no lingering effect.",
        "Restores health when injured; cannot heal beyond maximum.",
        "Lightning burst adds immediate shock damage.",
        "Poisons enemies for 6 seconds of lingering damage."
    ]
    var symbols := ["🔥", "❄", "✦", "✚", "⚡", "☣"]
    for group_name in ["Destruction", "Mysticism", "Restoration", "Alchemy"]:
        var section := Label.new()
        section.text = group_name.to_upper()
        section.add_theme_font_size_override("font_size", 20)
        spell_list.add_child(section)
        for i in range(spells.size()):
            if groups[i] != group_name:
                continue
            var line := HBoxContainer.new()
            spell_list.add_child(line)
            var select := Button.new()
            select.size_flags_horizontal = Control.SIZE_EXPAND_FILL
            select.custom_minimum_size.y = 60
            select.add_theme_font_size_override("font_size", 18)
            var prefix := "▶ " if i == spell_cursor else "  "
            select.text = "%s%s  %s\n%s | %d mana | %.1fs" % [prefix, symbols[i], spells[i]["name"], descriptions[i], int(spells[i]["cost"]), float(spells[i]["cooldown"])]
            select.alignment = HORIZONTAL_ALIGNMENT_LEFT
            select.pressed.connect(_select_spell_entry.bind(i))
            line.add_child(select)
            var left_button := Button.new()
            left_button.text = "LEFT%s" % (" ✓" if left_spell == i else "")
            left_button.custom_minimum_size.x = 74
            left_button.pressed.connect(_equip_spell_hand.bind(i, 0))
            line.add_child(left_button)
            var right_button := Button.new()
            right_button.text = "RIGHT%s" % (" ✓" if right_spell == i else "")
            right_button.custom_minimum_size.x = 80
            right_button.pressed.connect(_equip_spell_hand.bind(i, 1))
            line.add_child(right_button)

func _select_spell_entry(index: int) -> void:
    spell_cursor = index
    _render_spell_menu()

func _equip_spell_hand(index: int, hand: int) -> void:
    spell_cursor = index
    if hand == 0:
        left_spell = index
    else:
        right_spell = index
    _render_spell_menu()

func _cast_hand(hand: int) -> void:
    if hand_cooldowns[hand] > 0.0:
        return
    var index := left_spell if hand == 0 else right_spell
    if _cast_spell(index, 1.0):
        hand_cooldowns[hand] = float(spells[index]["cooldown"])
        _hand_cast_effect(hand, index)

func _dual_cast() -> void:
    if hand_cooldowns[0] > 0.0 or hand_cooldowns[1] > 0.0:
        return
    if left_spell != right_spell:
        return
    var index := left_spell
    if _cast_spell(index, 1.75, 2.0):
        hand_cooldowns[0] = float(spells[index]["cooldown"]) * 1.5
        hand_cooldowns[1] = hand_cooldowns[0]
        _hand_cast_effect(0, index)
        _hand_cast_effect(1, index)

func _cast_spell(index: int, multiplier: float, cost_multiplier: float = 1.0) -> bool:
    if index < 0 or index >= spells.size():
        return false
    if school_cooldowns[index] > 0.0:
        return false
    var spell: Dictionary = spells[index]
    var cost: float = float(spell["cost"]) * cost_multiplier
    if mana < cost:
        _cast_notice("Not enough mana")
        return false
    if index == 3:
        if hp >= max_hp:
            return false
        hp = minf(max_hp, hp + 24.0 * multiplier)
        _school_effect(player.position, index)
    else:
        var target := _select_target()
        if target == null:
            return false
        var dealt := damage * float(spell["power"]) * multiplier
        # Use the existing kill/XP/loot pipeline rather than bypassing it.
        var original := damage
        damage = dealt
        _attack_target(target)
        damage = original
        if is_instance_valid(target) and mobs.has(target):
            match index:
                0:
                    target.set_meta("burn", 4.0 * multiplier)
                1:
                    target.set_meta("chill", 3.5 * multiplier)
                4:
                    # Storm: bonus burst damage on the selected target.
                    var shock_hp := float(target.get_meta("hp")) - 9.0 * multiplier
                    target.set_meta("hp", shock_hp)
                    if shock_hp <= 0.0:
                        _defeat_mob(target)
                5:
                    target.set_meta("venom", 6.0 * multiplier)
        _school_effect(target.position if is_instance_valid(target) else player.position, index)
    mana -= cost
    school_cooldowns[index] = float(spell["cooldown"])
    _cast_notice("%s cast" % spell["name"])
    _rumble_cast()
    return true

func _attack_target(target: Node3D) -> void:
    if target == null or not is_instance_valid(target):
        return
    # Delegate through the established attack routine with the selected target lock.
    var previous := locked_target
    locked_target = target
    _attack()
    locked_target = previous

# Lightweight color-coded school flashes; does not alter either hero's artwork.
func _school_effect(at: Vector3, spell_index: int) -> void:
    if active_effects.size() >= 65:
        return
    var colors := [
        Color(1.0, 0.24, 0.05), Color(0.2, 0.85, 1.0),
        Color(0.65, 0.25, 1.0), Color(0.2, 1.0, 0.35),
        Color(1.0, 0.92, 0.2), Color(0.32, 0.9, 0.2)
    ]
    var pulse := MeshInstance3D.new()
    var sphere := SphereMesh.new()
    # Varied silhouettes without external textures or expensive particles.
    sphere.radius = [0.85, 0.65, 0.52, 0.78, 0.43, 0.60][spell_index]
    sphere.height = [1.2, 1.8, 1.1, 1.3, 2.1, 0.85][spell_index]
    pulse.mesh = sphere
    var mat := StandardMaterial3D.new()
    mat.albedo_color = colors[spell_index]
    mat.emission_enabled = true
    mat.emission = mat.albedo_color
    pulse.material_override = mat
    pulse.position = at + Vector3.UP
    pulse.set_meta("life", 0.3)
    pulse.set_meta("spin", true)
    pulse.set_meta("school", spell_index)
    pulse.set_meta("start_life", 0.3)
    add_child(pulse)
    active_effects.append(pulse)

func _has_clear_path(start: Vector3, finish: Vector3) -> bool:
    var query := PhysicsRayQueryParameters3D.create(start, finish)
    query.collision_mask = 2
    return get_world_3d().direct_space_state.intersect_ray(query).is_empty()

func _cast_notice(message: String) -> void:
    spell_notice = message
    notice_left = 1.3

func _animate_spell_preview() -> void:
    if spell_preview == null or not spell_menu_open:
        return
    # Text-based arcane animation remains readable and requires no new assets.
    var phases := ["✦ · ·", "· ✦ ·", "· · ✦", "· ✦ ·"]
    var phase := int(preview_time * 5.0) % phases.size()
    var selected: Dictionary = spells[spell_cursor]
    spell_preview.text = "%s  %s  %s\nMana %d | Power %.1fx | Cooldown %.2fs" % [
        phases[phase], selected["name"], phases[phase],
        int(selected["cost"]), float(selected["power"]), float(selected["cooldown"])
    ]

# Transient casting glows are separate from hero geometry and original art.
func _hand_cast_effect(hand: int, school: int) -> void:
    if active_effects.size() >= 65:
        return
    var colors := [
        Color(1.0, 0.22, 0.08), Color(0.3, 0.85, 1.0),
        Color(0.65, 0.3, 1.0), Color(0.2, 1.0, 0.4),
        Color(1.0, 0.88, 0.18), Color(0.3, 0.85, 0.25)
    ]
    var spark := MeshInstance3D.new()
    var sphere := SphereMesh.new()
    sphere.radius = 0.25
    sphere.height = 0.5
    spark.mesh = sphere
    var material := StandardMaterial3D.new()
    material.albedo_color = colors[school]
    material.emission_enabled = true
    material.emission = colors[school]
    spark.material_override = material
    spark.position = player.position + player.global_transform.basis.x * (-0.7 if hand == 0 else 0.7) + Vector3.UP * 1.2
    spark.set_meta("life", 0.25)
    spark.set_meta("spin", true)
    spark.set_meta("cast_hand", hand)
    spark.set_meta("cast_age", 0.0)
    spark.set_meta("cast_school", school)
    add_child(spark)
    active_effects.append(spark)

func _pad_button(button: int) -> bool:
    var pads := Input.get_connected_joypads()
    return not pads.is_empty() and Input.is_joy_button_pressed(pads[0], button)

func _controller_menus() -> void:
    if not started or dead or binds_open or options_open or pad_settings_open or crafting_open:
        return
    var menu_pressed := _pad_button(JOY_BUTTON_Y)
    if menu_pressed and not pad_menu_latch:
        spell_menu_open = not spell_menu_open
        spell_menu.visible = spell_menu_open
        if spell_menu_open:
            _render_spell_menu()
    pad_menu_latch = menu_pressed
    if not spell_menu_open:
        pad_cursor_latch = false
        pad_select_latch = false
        return
    var pads := Input.get_connected_joypads()
    var axis := 0.0
    if not pads.is_empty():
        axis = Input.get_joy_axis(pads[0], JOY_AXIS_LEFT_Y)
    var direction := 1 if axis > 0.55 else (-1 if axis < -0.55 else 0)
    if direction != 0 and not pad_cursor_latch:
        spell_cursor = posmod(spell_cursor + direction, spells.size())
        _render_spell_menu()
    pad_cursor_latch = direction != 0
    var equip_pressed := _pad_button(JOY_BUTTON_A) or _pad_button(JOY_BUTTON_X)
    if equip_pressed and not pad_select_latch:
        _equip_spell_hand(spell_cursor, 0 if _pad_button(JOY_BUTTON_X) else 1)
    pad_select_latch = equip_pressed

func _build_pad_settings(layer: CanvasLayer) -> void:
    pad_settings = PanelContainer.new()
    pad_settings.position = Vector2(365, 145)
    pad_settings.custom_minimum_size = Vector2(540, 300)
    pad_settings.visible = false
    layer.add_child(pad_settings)
    var rows := VBoxContainer.new()
    pad_settings.add_child(rows)
    var title := Label.new()
    title.text = "CONTROLLER ACCESSIBILITY  [F9] Close"
    rows.add_child(title)
    _add_pad_slider(rows, "Movement dead zone", 0.05, 0.55, pad_move_deadzone, 0)
    _add_pad_slider(rows, "Camera dead zone", 0.05, 0.55, pad_look_deadzone, 1)
    _add_pad_slider(rows, "Camera sensitivity", 0.4, 4.5, pad_camera_speed, 2)
    var rumble := CheckButton.new()
    rumble.text = "Controller vibration (off by default)"
    rumble.button_pressed = pad_vibration
    rumble.toggled.connect(_pad_rumble_changed)
    rows.add_child(rumble)

func _add_pad_slider(rows: VBoxContainer, title: String, minimum: float, maximum: float, initial: float, setting: int) -> void:
    var label := Label.new()
    label.text = title
    rows.add_child(label)
    var slider := HSlider.new()
    slider.min_value = minimum
    slider.max_value = maximum
    slider.step = 0.05
    slider.value = initial
    slider.custom_minimum_size.x = 440
    slider.value_changed.connect(_pad_setting_changed.bind(setting, label, title))
    rows.add_child(slider)

func _pad_setting_changed(value: float, setting: int, label: Label, title: String) -> void:
    match setting:
        0:
            pad_move_deadzone = value
        1:
            pad_look_deadzone = value
        2:
            pad_camera_speed = value
    label.text = "%s: %.2f" % [title, value]

func _pad_rumble_changed(enabled: bool) -> void:
    pad_vibration = enabled
    if not enabled:
        for pad_id in Input.get_connected_joypads():
            Input.stop_joy_vibration(pad_id)

func _rumble_cast() -> void:
    if not pad_vibration:
        return
    var pads := Input.get_connected_joypads()
    if not pads.is_empty():
        Input.start_joy_vibration(pads[0], 0.18, 0.3, 0.12)

func _build_options_menu(layer: CanvasLayer) -> void:
    options_panel = PanelContainer.new()
    options_panel.position = Vector2(440, 150)
    options_panel.custom_minimum_size = Vector2(400, 420)
    options_panel.visible = false
    layer.add_child(options_panel)
    var menu := VBoxContainer.new()
    options_panel.add_child(menu)
    var title := Label.new()
    title.text = "MOONFALL | OPTIONS"
    title.add_theme_font_size_override("font_size", 26)
    menu.add_child(title)
    for entry in ["Resume", "Spellbook", "Inventory", "Equipment", "Crafting", "Keybindings", "Controller Settings"]:
        var button := Button.new()
        button.text = entry
        button.custom_minimum_size.y = 60
        button.focus_mode = Control.FOCUS_ALL
        button.pressed.connect(_options_action.bind(entry))
        menu.add_child(button)

func _close_all_options() -> void:
    options_open = false
    options_panel.visible = false
    crafting_open = false
    crafting_panel.visible = false
    equipment_open = false
    equipment_panel.visible = false
    spell_menu_open = false
    spell_menu.visible = false
    pad_settings_open = false
    pad_settings.visible = false
    binds_open = false
    binding_capture = false
    binds_panel.visible = false
    inventory_open = false
    inventory_panel.visible = false

func _open_options_menu() -> void:
    _close_all_options()
    options_open = true
    options_panel.visible = true
    var buttons := options_panel.get_child(0).get_children()
    if buttons.size() > 1:
        (buttons[1] as Button).grab_focus()

func _options_action(action: String) -> void:
    _close_all_options()
    match action:
        "Spellbook":
            spell_menu_open = true
            spell_menu.visible = true
            _render_spell_menu()
        "Inventory":
            inventory_open = true
            inventory_panel.visible = true
            _refresh_inventory()
        "Equipment":
            equipment_open = true
            equipment_panel.visible = true
            _refresh_equipment_menu()
        "Crafting":
            crafting_open = true
            crafting_panel.visible = true
            _refresh_crafting()
            (crafting_panel.get_child(0).get_child(2) as Button).grab_focus()
        "Keybindings":
            binds_open = true
            binds_panel.visible = true
            _render_binds()
        "Controller Settings":
            pad_settings_open = true
            pad_settings.visible = true

func _controller_start_menu() -> void:
    var pressed := _pad_button(JOY_BUTTON_START)
    if pressed and not start_latch and started:
        if options_open:
            _close_all_options()
        else:
            _open_options_menu()
    start_latch = pressed

func _boss_warning(position: Vector3, kind: int) -> void:
    if active_effects.size() >= 65:
        return
    var indicator := MeshInstance3D.new()
    var cylinder := CylinderMesh.new()
    cylinder.top_radius = 3.1 if kind == 0 else 1.8
    cylinder.bottom_radius = cylinder.top_radius
    cylinder.height = 0.05
    indicator.mesh = cylinder
    indicator.position = position + Vector3.UP * 0.06
    var material := StandardMaterial3D.new()
    material.albedo_color = Color(1.0, 0.12, 0.1, 0.65)
    material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
    material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
    indicator.material_override = material
    indicator.set_meta("life", 1.1 if kind == 0 else 0.75)
    add_child(indicator)
    active_effects.append(indicator)

func _craft_upgrade(mana_upgrade: bool) -> void:
    if gold < 50 or moon_shards < 3:
        _cast_notice("Crafting requires 50 gold and 3 Moon Shards")
        return
    gold -= 50
    moon_shards -= 3
    if mana_upgrade:
        max_mana += 10.0
        mana = minf(max_mana, mana + 10.0)
        _cast_notice("Crafted Focus: +10 max mana")
    else:
        max_hp += 10.0
        hp = minf(max_hp, hp + 10.0)
        _cast_notice("Crafted Vitality: +10 max HP")
    _refresh_inventory()
    if crafting_open:
        _refresh_crafting()

func _build_crafting_menu(layer: CanvasLayer) -> void:
    crafting_panel = PanelContainer.new()
    crafting_panel.position = Vector2(420, 155)
    crafting_panel.custom_minimum_size = Vector2(460, 360)
    crafting_panel.visible = false
    layer.add_child(crafting_panel)
    var rows := VBoxContainer.new()
    crafting_panel.add_child(rows)
    var title := Label.new()
    title.text = "MOONFALL | CRAFTING"
    title.add_theme_font_size_override("font_size", 24)
    rows.add_child(title)
    crafting_info = Label.new()
    rows.add_child(crafting_info)
    for recipe in ["Vitality (+10 max HP)", "Focus (+10 max Mana)"]:
        var button := Button.new()
        button.text = recipe + " | 50 Gold + 3 Shards"
        button.custom_minimum_size.y = 62
        button.focus_mode = Control.FOCUS_ALL
        button.pressed.connect(_craft_upgrade.bind(recipe.begins_with("Focus")))
        rows.add_child(button)
    var forge := Button.new()
    forge.text = "Forge equipped weapon: +3 damage | 80 Gold + 5 Shards"
    forge.custom_minimum_size.y = 62
    forge.focus_mode = Control.FOCUS_ALL
    forge.pressed.connect(_forge_weapon)
    rows.add_child(forge)
    var forge_armor := Button.new()
    forge_armor.text = "Forge equipped armor: +2 armor | 70 Gold + 4 Shards"
    forge_armor.custom_minimum_size.y = 62
    forge_armor.focus_mode = Control.FOCUS_ALL
    forge_armor.pressed.connect(_forge_armor)
    rows.add_child(forge_armor)
    var back := Button.new()
    back.text = "Back to Options"
    back.custom_minimum_size.y = 58
    back.pressed.connect(_open_options_menu)
    rows.add_child(back)

func _refresh_crafting() -> void:
    if crafting_info != null:
        crafting_info.text = "Gold: %d | Moon Shards: %d" % [gold, moon_shards]

func _forge_weapon() -> void:
    if equipped.is_empty():
        _cast_notice("Equip a weapon before forging")
        return
    if gold < 80 or moon_shards < 5:
        _cast_notice("Forging needs 80 gold and 5 Moon Shards")
        return
    gold -= 80
    moon_shards -= 5
    equipped["power"] = int(equipped["power"]) + 3
    damage += 3.0
    for i in range(inventory.size()):
        if inventory[i].get("slot", "") == "Weapon" and inventory[i].get("name", "") == equipped["name"]:
            inventory[i]["power"] = equipped["power"]
            break
    _cast_notice("Weapon forged: +3 damage")
    _refresh_inventory()
    _refresh_crafting()

func _forge_armor() -> void:
    if equipped_armor.is_empty():
        _cast_notice("Equip armor before forging")
        return
    if gold < 70 or moon_shards < 4:
        _cast_notice("Armor forging needs 70 gold and 4 Moon Shards")
        return
    gold -= 70
    moon_shards -= 4
    equipped_armor["power"] = int(equipped_armor["power"]) + 2
    armor = float(equipped_armor["power"])
    for i in range(inventory.size()):
        if inventory[i].get("slot", "") == "Armor" and inventory[i].get("name", "") == equipped_armor["name"]:
            inventory[i]["power"] = equipped_armor["power"]
            break
    _cast_notice("Armor forged: +2 armor")
    _refresh_inventory()
    _refresh_crafting()

func _build_equipment_menu(layer: CanvasLayer) -> void:
    equipment_panel = PanelContainer.new()
    equipment_panel.position = Vector2(380, 110)
    equipment_panel.custom_minimum_size = Vector2(540, 460)
    equipment_panel.visible = false
    layer.add_child(equipment_panel)
    var scroll := ScrollContainer.new()
    equipment_panel.add_child(scroll)
    equipment_list = VBoxContainer.new()
    equipment_list.custom_minimum_size.x = 490
    scroll.add_child(equipment_list)

func _refresh_equipment_menu() -> void:
    if equipment_list == null:
        return
    for old in equipment_list.get_children():
        equipment_list.remove_child(old)
        old.queue_free()
    var title := Label.new()
    title.text = "EQUIPMENT | Choose an item"
    equipment_list.add_child(title)
    for i in range(inventory.size()):
        var item: Dictionary = inventory[i]
        var button := Button.new()
        button.text = "%s %s (%s) +%d" % [item["rarity"], item["name"], item["slot"], int(item["power"])]
        button.custom_minimum_size.y = 52
        button.focus_mode = Control.FOCUS_ALL
        button.pressed.connect(_choose_equipment.bind(i))
        equipment_list.add_child(button)
    var back := Button.new()
    back.text = "Back to Options"
    back.custom_minimum_size.y = 56
    back.pressed.connect(_open_options_menu)
    equipment_list.add_child(back)
    if equipment_list.get_child_count() > 1:
        (equipment_list.get_child(1) as Button).grab_focus()

func _choose_equipment(index: int) -> void:
    _equip_item(index)
    _refresh_equipment_menu()
