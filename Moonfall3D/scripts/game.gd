extends Node3D

# Isolated prototype. Never modifies existing Moonfall game assets.
const ARENA := 24.0
const BOSS_INTERVAL := 120.0
const MAX_MOBS := 28
const MAX_PICKUPS := 40
var player: Node3D
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

func _ready() -> void:
    randomize()
    var ground := MeshInstance3D.new()
    var plane := PlaneMesh.new()
    plane.size = Vector2(60, 60)
    ground.mesh = plane
    var ground_mat := StandardMaterial3D.new()
    ground_mat.albedo_color = Color(0.16, 0.10, 0.22)
    ground.material_override = ground_mat
    add_child(ground)
    var sun := DirectionalLight3D.new()
    sun.rotation_degrees = Vector3(-55, -35, 0)
    add_child(sun)
    player = Node3D.new()
    add_child(player)
    var body := MeshInstance3D.new()
    body.name = "Placeholder"
    var capsule := CapsuleMesh.new()
    capsule.height = 2.2
    capsule.radius = 0.6
    body.mesh = capsule
    body.position.y = 1.1
    player.add_child(body)
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
    _hud()

func _choose(name: String) -> void:
    hero = name
    started = true
    overlay.visible = false
    var mat := StandardMaterial3D.new()
    mat.albedo_color = Color(0.3, 0.6, 1.0) if name == "Z" else Color(0.8, 0.3, 0.95)
    (player.get_node("Placeholder") as MeshInstance3D).material_override = mat

func _unhandled_input(event: InputEvent) -> void:
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
    if not started or dead:
        return
    clock += dt
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
    dodge_wait = maxf(0.0, dodge_wait - dt)
    dodge_left = maxf(0.0, dodge_left - dt)
    invulnerable = maxf(0.0, invulnerable - dt)
    var motion := Vector3.ZERO
    if Input.is_key_pressed(KEY_A) or Input.is_key_pressed(KEY_LEFT):
        motion.x -= 1
    if Input.is_key_pressed(KEY_D) or Input.is_key_pressed(KEY_RIGHT):
        motion.x += 1
    if Input.is_key_pressed(KEY_W) or Input.is_key_pressed(KEY_UP):
        motion.z -= 1
    if Input.is_key_pressed(KEY_S) or Input.is_key_pressed(KEY_DOWN):
        motion.z += 1
    motion = motion.normalized()
    # Camera-relative movement, with right-drag turning the character.
    motion = motion.rotated(Vector3.UP, camera_yaw)
    if Input.is_key_pressed(KEY_SPACE) and dodge_wait <= 0.0:
        dodge_dir = motion if motion.length() > 0.0 else Vector3.FORWARD
        dodge_left = 0.24
        dodge_wait = 1.1
        invulnerable = 0.35
    var velocity := dodge_dir * 18.0 if dodge_left > 0.0 else motion * 7.0
    player.position += velocity * dt
    player.position.x = clampf(player.position.x, -ARENA, ARENA)
    player.position.z = clampf(player.position.z, -ARENA, ARENA)
    # Normal movement turns to travel direction, except while right-drag turning.
    if velocity.length() > 0.0 and not Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT):
        player.rotation.y = atan2(velocity.x, velocity.z)
    var radius := 9.0 * zoom
    var horizontal := cos(camera_pitch) * radius
    var follow_offset := Vector3(sin(camera_yaw) * horizontal, -sin(camera_pitch) * radius + 1.3, cos(camera_yaw) * horizontal)
    var focus := player.position + Vector3.UP * 1.3
    camera.position = camera.position.lerp(player.position + follow_offset, minf(1.0, dt * 6.0))
    camera.look_at(focus)
    if (Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT) or Input.is_key_pressed(KEY_J)) and attack_wait <= 0.0:
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
        var diff := player.position - mob.position
        diff.y = 0.0
        var boss: bool = mob.get_meta("boss")
        var reach := 2.8 if boss else 1.7
        if diff.length() > reach:
            var steering := diff.normalized()
            # Different enemy movement patterns; no collision-pathfinding claim.
            if not boss and int(mob.get_meta("kind")) == 1 and diff.length() < 6.0:
                steering = Vector3(-steering.z, 0, steering.x) * 0.65 + steering * 0.35
            mob.position += steering.normalized() * (1.9 if boss else 2.3) * dt
            mob.position.x = clampf(mob.position.x, -ARENA, ARENA)
            mob.position.z = clampf(mob.position.z, -ARENA, ARENA)
        var hit_cd: float = maxf(0.0, float(mob.get_meta("hit_cd")) - dt)
        if diff.length() <= reach + 0.3 and hit_cd <= 0.0 and invulnerable <= 0.0:
            hp -= 25.0 if boss else 12.0
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
    mob.position = player.position + Vector3(cos(angle) * 18.0, 0, sin(angle) * 18.0)
    mob.position.x = clampf(mob.position.x, -ARENA, ARENA)
    mob.position.z = clampf(mob.position.z, -ARENA, ARENA)
    mob.set_meta("hp", 600.0 + level * 70.0 if boss else 45.0 + level * 10.0)
    mob.set_meta("boss", boss)
    mob.set_meta("kind", randi_range(0, 1))
    mob.set_meta("hit_cd", 0.0)
    add_child(mob)
    mobs.append(mob)
    if boss:
        boss_alive = true

func _attack() -> void:
    attack_wait = 0.25 if hero == "Z" else 0.38
    var target: Node3D = null
    var nearest := 5.2 if hero == "Z" else 8.0
    for mob in mobs:
        var d := mob.position.distance_to(player.position)
        if d < nearest:
            target = mob
            nearest = d
    if target == null:
        return
    _spell_effect(target.position, hero == "Nyra")
    var left: float = float(target.get_meta("hp")) - damage
    target.set_meta("hp", left)
    if left > 0.0:
        return
    var was_boss: bool = target.get_meta("boss")
    var death_position := target.position
    mobs.erase(target)
    target.queue_free()
    if was_boss:
        boss_alive = false
    kills += 1
    if kills % 15 == 0:
        room_index += 1
        _build_room()
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
    ui.text = "%s   Room: %d   HP: %d/%d   Level: %d   XP: %d   Kills: %d   DMG: %d" % [hero, room_index + 1, int(hp), int(max_hp), level, xp, kills, int(damage)]
    if dead:
        ui.text += "\nDefeated. Press R to restart."

# Decorative room changes every 15 kills. The arena stays connected and traversable.
func _build_room() -> void:
    var old := get_node_or_null("RoomDecor")
    if old != null:
        old.queue_free()
    var decor := Node3D.new()
    decor.name = "RoomDecor"
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
