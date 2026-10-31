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
var dungeon_seed := 0
var inventory: Array[Dictionary] = []
var equipped: Dictionary = {}
var inventory_open := false
var inventory_panel: Label
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
var spell_menu_open := false
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
    spell_list = VBoxContainer.new()
    spell_layout.add_child(spell_list)
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
        if event.keycode == KEY_M and started and not binds_open:
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
    if dead and Input.is_key_pressed(KEY_R):
        get_tree().reload_current_scene()
        return
    if not started or dead or binds_open or spell_menu_open:
        return
    clock += dt
    mana = minf(max_mana, mana + 7.0 * dt)
    notice_left = maxf(0.0, notice_left - dt)
    for school in range(school_cooldowns.size()):
        school_cooldowns[school] = maxf(0.0, school_cooldowns[school] - dt)
    for hand in range(hand_cooldowns.size()):
        hand_cooldowns[hand] = maxf(0.0, hand_cooldowns[hand] - dt)
    if Input.is_key_pressed(KEY_C):
        _dual_cast()
    else:
        if Input.is_key_pressed(KEY_Z):
            _cast_hand(0)
        if Input.is_key_pressed(KEY_X):
            _cast_hand(1)
    var inventory_key := _pressed("inventory")
    if inventory_key and not inventory_latch:
        inventory_open = not inventory_open
        inventory_panel.visible = inventory_open
        _refresh_inventory()
    inventory_latch = inventory_key
    var equip_key := Input.is_key_pressed(KEY_G)
    if inventory_open and equip_key and not equip_latch and not inventory.is_empty():
        selected_item = (selected_item + 1) % inventory.size()
        _equip_item(selected_item)
    equip_latch = equip_key
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
    attack_wait = maxf(0.0, attack_wait - dt)
    var jump_key := _pressed("jump")
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
    motion = motion.normalized()
    # Camera-relative movement, with right-drag turning the character.
    motion = motion.rotated(Vector3.UP, camera_yaw)
    var dodge_pressed := _pressed("dodge")
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
        var diff := destination - mob.position
        diff.y = 0.0
        var boss: bool = mob.get_meta("boss")
        var reach := 2.8 if boss else 1.7
        if diff.length() > reach:
            var steering := diff.normalized()
            # Different enemy movement patterns; no collision-pathfinding claim.
            if not boss and int(mob.get_meta("kind")) == 1 and diff.length() < 6.0:
                steering = Vector3(-steering.z, 0, steering.x) * 0.65 + steering * 0.35
            # Raycast ahead to steer around solid obstacles.
            var travel := steering.normalized()
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
            mob.position += travel * (1.9 if boss else 2.3) * (0.65 if chill > 0.0 else 1.0) * dt
            mob.position.x = clampf(mob.position.x, -ARENA, ARENA)
            mob.position.z = clampf(mob.position.z, -ARENA, ARENA)
        var ability_cd: float = maxf(0.0, float(mob.get_meta("ability_cd")) - dt)
        var actual_distance := mob.position.distance_to(player.position)
        if ability_cd <= 0.0 and actual_distance < 7.0 and actual_distance > reach:
            if boss:
                _spell_effect(player.position, false)
                if invulnerable <= 0.0:
                    hp = maxf(0.0, hp - maxf(1.0, 18.0 - armor))
                    dead = hp <= 0.0
            elif int(mob.get_meta("kind")) == 1:
                mob.position -= (player.position - mob.position).normalized() * 2.0
            ability_cd = 6.0 if boss else 4.0
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
    mat.albedo_color = Color(1.0, 0.25, 0.75) if boss else Color(0.9, 0.2, 0.25)
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
    mob.set_meta("kind", randi_range(0, 1))
    mob.set_meta("hit_cd", 0.0)
    mob.set_meta("ability_cd", randf_range(3.0, 6.0))
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
    xp += 10 if not was_boss else 100
    while xp >= level * 60:
        xp -= level * 60
        level += 1
        max_hp += 12.0
        hp = minf(max_hp, hp + 12.0)
        damage += 5.0
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
    var tint := [Color(0.3, 0.12, 0.4), Color(0.22, 0.45, 0.55), Color(0.65, 0.2, 0.15)][room_index % 3]
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
    for pos in [Vector3(0, 0, -17), Vector3(0, 0, 0), Vector3(0, 0, 17)]:
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
        add_child(chest)
        chests.append(chest)

func _open_nearby_chest() -> void:
    for chest in chests.duplicate():
        if not is_instance_valid(chest) or player.position.distance_to(chest.position) > 2.5:
            continue
        gold += randi_range(10, 35)
        _loot_equipment()
        chests.erase(chest)
        chest.queue_free()
        break

# First inventory pass: chests yield equipment with weighted rarities.
# Best found weapon auto-equips. A manual equip interface can follow.
func _loot_equipment() -> void:
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
    var item := {
        "name": ["Worn Blade", "Runed Sword", "Moonsteel Edge", "Astral Fang", "Eclipse Crownblade"][rarity_index],
        "rarity": RARITIES[rarity_index],
        "power": (rarity_index + 1) * 3,
        "slot": "Armor" if randi_range(0, 1) == 0 else "Weapon"
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
    lines.append("Armor: %d" % int(armor))
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
        armor = float(item["power"])
    _refresh_inventory()

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

func _dual_cast() -> void:
    if hand_cooldowns[0] > 0.0 or hand_cooldowns[1] > 0.0:
        return
    if left_spell != right_spell:
        return
    var index := left_spell
    if _cast_spell(index, 1.75, 2.0):
        hand_cooldowns[0] = float(spells[index]["cooldown"]) * 1.5
        hand_cooldowns[1] = hand_cooldowns[0]

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
    var colors := [
        Color(1.0, 0.24, 0.05), Color(0.2, 0.85, 1.0),
        Color(0.65, 0.25, 1.0), Color(0.2, 1.0, 0.35),
        Color(1.0, 0.92, 0.2), Color(0.32, 0.9, 0.2)
    ]
    var pulse := MeshInstance3D.new()
    var sphere := SphereMesh.new()
    sphere.radius = 0.7
    sphere.height = 1.4
    pulse.mesh = sphere
    var mat := StandardMaterial3D.new()
    mat.albedo_color = colors[spell_index]
    mat.emission_enabled = true
    mat.emission = mat.albedo_color
    pulse.material_override = mat
    pulse.position = at + Vector3.UP
    pulse.set_meta("life", 0.3)
    add_child(pulse)
    active_effects.append(pulse)

func _has_clear_path(start: Vector3, finish: Vector3) -> bool:
    var query := PhysicsRayQueryParameters3D.create(start, finish)
    query.collision_mask = 2
    return get_world_3d().direct_space_state.intersect_ray(query).is_empty()

func _cast_notice(message: String) -> void:
    spell_notice = message
    notice_left = 1.3
