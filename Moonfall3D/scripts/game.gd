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
    animation_phase += dt * (11.0 if velocity.length() > 0.0 else 3.0)
    # Placeholder animation only; replace when original-design character models arrive.
    var placeholder := player.get_node("Placeholder") as MeshInstance3D
    placeholder.position.y = 1.1 + (absf(sin(animation_phase)) * 0.12 if velocity.length() > 0.0 else sin(animation_phase) * 0.025)
    player.velocity = velocity
    player.move_and_slide()
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
    cached_target = _select_target()
    target_marker.visible = cached_target != null
    if cached_target != null:
        target_marker.position = cached_target.position + Vector3.UP * 0.08
    if Input.is_key_pressed(KEY_E):
        _open_nearby_chest()
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
            mob.position += travel * (1.9 if boss else 2.3) * dt
            mob.position.x = clampf(mob.position.x, -ARENA, ARENA)
            mob.position.z = clampf(mob.position.z, -ARENA, ARENA)
        var hit_cd: float = maxf(0.0, float(mob.get_meta("hit_cd")) - dt)
        if mob.position.distance_to(player.position) <= reach + 0.3 and hit_cd <= 0.0 and invulnerable <= 0.0:
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
    ui.text = "%s   Room: %d   HP: %d/%d   Level: %d   XP: %d   Kills: %d   DMG: %d   Gold: %d   [E] Chest" % [hero, room_index + 1, int(hp), int(max_hp), level, xp, kills, int(damage), gold]
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
    var chosen: Node3D = null
    var best_score := INF
    var facing := -player.global_transform.basis.z
    for mob in mobs:
        if not is_instance_valid(mob):
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
        if randi_range(0, 1) == 0:
            hp = minf(max_hp, hp + 25.0)
        else:
            damage += 2.0
        chests.erase(chest)
        chest.queue_free()
        break
