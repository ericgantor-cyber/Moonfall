extends Node3D
## Moonfall Native v1: independent Godot/Witchwood vertical slice.
## Original character-image bytes are preserved verbatim under assets/heroes/.

const Z_ART := preload("res://assets/heroes/Z_Original.webp")
const NYRA_ART := preload("res://assets/heroes/Nyra_Original.png")
const SAVE_PATH := "user://moonfall_native_v1.json"
const WALK_SPEED := 5.0
const SPRINT_SPEED := 7.5
const MAP_LIMIT := 27.0
const WOLF_AGGRO_RANGE := 10.5
const WOLF_ATTACK_RANGE := 1.6
const SHRINE_POS := Vector3(5.5, 0.0, -4.0)

var world: Node3D
var player: CharacterBody3D
var camera: Camera3D
var hero_sprite: Sprite3D
var wolves: Array[Dictionary] = []
var loot_drops: Array[Dictionary] = []
var rng := RandomNumberGenerator.new()
var hero := "z"
var hp := 100.0
var max_hp := 100.0
var gold := 0
var kills := 0
var shrine_awakened := false
var hero_playing := false
var menu_open := false
var senses_on := false
var attack_cooldown := 0.0
var status_message := ""
var status_seconds := 0.0
var clues: Array[Node3D] = []
var shrine_light: OmniLight3D
var hud: Label
var hint: Label
var status: Label
var start_screen: Control
var pause_screen: Control
var hero_portrait: TextureRect

func _ready() -> void:
	rng.seed = 1032026
	_build_scene()
	_build_ui()
	_show_message("Native Witchwood prototype loaded. Select Z or Nyra.", 5.0)

func _build_scene() -> void:
	world = Node3D.new()
	world.name = "WitchwoodWorld"
	add_child(world)
	var env := WorldEnvironment.new()
	env.name = "MoonlitAtmosphere"
	var settings := Environment.new()
	settings.background_mode = Environment.BG_COLOR
	settings.background_color = Color("080c1c")
	settings.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	settings.ambient_light_color = Color("8292c7")
	settings.ambient_light_energy = 0.45
	settings.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	settings.fog_enabled = true
	settings.fog_light_color = Color("243351")
	settings.fog_density = 0.013
	env.environment = settings
	add_child(env)

	var moon := DirectionalLight3D.new()
	moon.name = "Moonlight"
	moon.rotation_degrees = Vector3(-53, -38, 0)
	moon.light_color = Color("a0b2ff")
	moon.light_energy = 0.9
	moon.shadow_enabled = true
	moon.directional_shadow_max_distance = 42.0
	add_child(moon)

	var ground := MeshInstance3D.new()
	ground.name = "WitchwoodFloor"
	var plane := PlaneMesh.new()
	plane.size = Vector2(62, 62)
	ground.mesh = plane
	ground.material_override = _material(Color("19302a"), 0.95)
	world.add_child(ground)
	_add_box(Vector3(0, -0.15, 0), Vector3(61, 0.28, 61), Color("14211e"), true, "WorldBoundaryFloor")

	# Moonlit earthen path across the clearing; the height is visual only.
	for i in range(18):
		var px := float(i - 9) * 2.65
		_add_box(Vector3(px, 0.004, sin(float(i) * 0.65) * 1.4), Vector3(2.7, 0.012, 3.1), Color("44423a"), false, "Path")

	# Deterministic, authored major colliders. Art and physics come from the same calls.
	for spot in [Vector2(-22, -18), Vector2(-17, -15), Vector2(-11, -19), Vector2(-19, 11), Vector2(-13, 16), Vector2(16, -19), Vector2(22, -12), Vector2(21, 18), Vector2(13, 20)]:
		_add_pine(Vector3(spot.x, 0, spot.y), 1.15)
	for i in range(55):
		var x := rng.randf_range(-26.0, 26.0)
		var z := rng.randf_range(-26.0, 26.0)
		if Vector2(x, z).length() < 11.0 or absf(z) < 4.0:
			continue
		if Vector2(x - SHRINE_POS.x, z - SHRINE_POS.z).length() < 5.0:
			continue
		_add_pine(Vector3(x, 0, z), rng.randf_range(0.72, 1.35))
	for i in range(27):
		var x := rng.randf_range(-26, 26)
		var z := rng.randf_range(-26, 26)
		if Vector2(x, z).length() < 7.0 or Vector2(x - SHRINE_POS.x, z - SHRINE_POS.z).length() < 4.0:
			continue
		_add_boulder(Vector3(x, 0, z), rng.randf_range(0.35, 1.05))
	_add_ruins()
	_add_shrine()
	_add_mushrooms()
	_create_player()
	for wolf_pos in [Vector3(11, 0, 9), Vector3(-13, 0, -9), Vector3(15, 0, -8), Vector3(-18, 0, 14), Vector3(9, 0, 17)]:
		_spawn_wolf(wolf_pos)
	camera = Camera3D.new()
	camera.name = "IsometricCamera"
	camera.projection = Camera3D.PROJECTION_ORTHOGONAL
	camera.size = 18.5
	camera.near = 0.05
	camera.far = 150.0
	camera.current = true
	add_child(camera)
	_update_camera(1.0)

func _material(color: Color, roughness: float = 0.85, emissive: bool = false) -> StandardMaterial3D:
	var mat := StandardMaterial3D.new()
	mat.albedo_color = color
	mat.roughness = roughness
	if emissive:
		mat.emission_enabled = true
		mat.emission = color
		mat.emission_energy_multiplier = 2.3
	return mat

func _add_box(at: Vector3, extents: Vector3, color: Color, solid: bool, label: String = "Prop") -> Node3D:
	var root: Node3D = StaticBody3D.new() if solid else Node3D.new()
	root.name = label
	root.position = at
	world.add_child(root)
	var mesh := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = extents
	mesh.mesh = box
	mesh.material_override = _material(color)
	root.add_child(mesh)
	if solid:
		var coll := CollisionShape3D.new()
		var shape := BoxShape3D.new()
		shape.size = extents
		coll.shape = shape
		root.add_child(coll)
	return root

func _add_cylinder(at: Vector3, radius: float, height: float, color: Color, solid: bool, top_radius: float = -1.0) -> Node3D:
	var root: Node3D = StaticBody3D.new() if solid else Node3D.new()
	root.position = at
	root.name = "Trunk" if solid else "Foliage"
	world.add_child(root)
	var mesh := MeshInstance3D.new()
	var shape := CylinderMesh.new()
	shape.bottom_radius = radius
	shape.top_radius = radius if top_radius < 0.0 else top_radius
	shape.height = height
	shape.radial_segments = 7
	mesh.mesh = shape
	mesh.material_override = _material(color)
	root.add_child(mesh)
	if solid:
		var coll := CollisionShape3D.new()
		var cyl := CylinderShape3D.new()
		cyl.radius = radius
		cyl.height = height
		coll.shape = cyl
		root.add_child(coll)
	return root

func _add_sphere(at: Vector3, radius: float, color: Color, solid: bool = false, glow: bool = false) -> Node3D:
	var root: Node3D = StaticBody3D.new() if solid else Node3D.new()
	root.name = "GlowingFlora" if glow else "StoneOrOrb"
	root.position = at
	world.add_child(root)
	var mesh := MeshInstance3D.new()
	var shape := SphereMesh.new()
	shape.radius = radius
	shape.height = 2 * radius
	mesh.mesh = shape
	mesh.material_override = _material(color, 0.65, glow)
	root.add_child(mesh)
	if solid:
		var collider := CollisionShape3D.new()
		var sphere := SphereShape3D.new()
		sphere.radius = radius
		collider.shape = sphere
		root.add_child(collider)
	return root

func _add_pine(at: Vector3, scale_amount: float) -> void:
	var height := 3.5 * scale_amount
	_add_cylinder(at + Vector3(0, height * 0.34, 0), 0.3 * scale_amount, height * 0.68, Color("382c32"), true)
	for i in range(3):
		var r := (1.65 - i * 0.31) * scale_amount
		var h := (2.0 - i * 0.14) * scale_amount
		_add_cylinder(at + Vector3(0, (1.8 + i * 0.92) * scale_amount, 0), r, h, Color("183c39") if i % 2 == 0 else Color("225248"), false, 0.0)

func _add_boulder(at: Vector3, radius: float) -> void:
	_add_sphere(at + Vector3(0, radius * 0.75, 0), radius, Color("465255"), true)

func _add_ruins() -> void:
	for k in range(3):
		var x := -9.5 + float(k) * 3.2
		_add_box(Vector3(x, 1.25, -14), Vector3(1.0, 2.5, 1.0), Color("58535f"), true, "RuinedPillar")
	_add_box(Vector3(-6.3, 3.1, -14), Vector3(8.5, 0.7, 1.1), Color("545069"), true, "RuinedLintel")
	for k in range(4):
		_add_box(Vector3(-15 + float(k) * 2.0, 0.6, 6), Vector3(1.8, 1.1, 1.15), Color("4d5059"), true, "BrokenWall")

func _add_shrine() -> void:
	_add_box(SHRINE_POS + Vector3(0, 0.3, 0), Vector3(3.0, 0.6, 2.0), Color("50445a"), true, "AwakeningShrineBase")
	_add_box(SHRINE_POS + Vector3(0, 1.05, 0.15), Vector3(1.45, 1.0, 0.55), Color("625773"), false, "ShrineAltar")
	for dx in [-1.0, 1.0]:
		_add_cylinder(SHRINE_POS + Vector3(dx, 1.6, -0.6), 0.22, 2.8, Color("70627b"), false)
	_add_box(SHRINE_POS + Vector3(0, 3.05, -0.6), Vector3(2.55, 0.5, 0.9), Color("655579"), false, "ShrineLintel")
	_add_sphere(SHRINE_POS + Vector3(0, 2.1, 0.1), 0.48, Color("7d58d2"), false, true)
	shrine_light = OmniLight3D.new()
	shrine_light.name = "ShrineMoonfire"
	shrine_light.position = SHRINE_POS + Vector3(0, 2.2, 0)
	shrine_light.light_color = Color("ae79ff")
	shrine_light.light_energy = 2.3
	shrine_light.omni_range = 8.0
	world.add_child(shrine_light)

func _add_mushrooms() -> void:
	for i in range(45):
		var x := rng.randf_range(-24, 24)
		var z := rng.randf_range(-24, 24)
		var glow_color := Color("63c5cb") if i % 2 == 0 else Color("9b6cff")
		var sphere := _add_sphere(Vector3(x, 0.12, z), rng.randf_range(0.09, 0.17), glow_color, false, true)
		if i % 7 == 0:
			clues.append(sphere)
			sphere.visible = false

func _create_player() -> void:
	player = CharacterBody3D.new()
	player.name = "Player"
	player.position = Vector3(0, 0.85, 0)
	world.add_child(player)
	var collider := CollisionShape3D.new()
	var capsule := CapsuleShape3D.new()
	capsule.radius = 0.34
	capsule.height = 1.7
	collider.shape = capsule
	player.add_child(collider)
	hero_sprite = Sprite3D.new()
	hero_sprite.name = "OriginalHeroArtBillboard"
	hero_sprite.texture = Z_ART
	hero_sprite.billboard = 1
	hero_sprite.pixel_size = 2.65 / float(Z_ART.get_height())
	hero_sprite.position = Vector3(0, 0.85, 0)
	hero_sprite.alpha_cut = SpriteBase3D.ALPHA_CUT_OPAQUE_PREPASS
	player.add_child(hero_sprite)

func _spawn_wolf(at: Vector3) -> void:
	var body := CharacterBody3D.new()
	body.name = "WitchwoodWolf"
	body.position = at + Vector3(0, 0.62, 0)
	world.add_child(body)
	var col := CollisionShape3D.new()
	var shape := CapsuleShape3D.new()
	shape.radius = 0.45
	shape.height = 1.25
	col.shape = shape
	body.add_child(col)
	var mesh := MeshInstance3D.new()
	var torso := CapsuleMesh.new()
	torso.radius = 0.46
	torso.height = 1.4
	mesh.mesh = torso
	mesh.rotation_degrees = Vector3(0, 0, 90)
	mesh.material_override = _material(Color("353c55"))
	body.add_child(mesh)
	var head := MeshInstance3D.new()
	var snout := SphereMesh.new()
	snout.radius = 0.34
	snout.height = 0.68
	head.mesh = snout
	head.position = Vector3(0.54, 0.15, 0)
	head.material_override = _material(Color("566784"))
	body.add_child(head)
	for dz in [-0.2, 0.2]:
		var ear := MeshInstance3D.new()
		var cone := CylinderMesh.new()
		cone.bottom_radius = 0.16
		cone.top_radius = 0.0
		cone.height = 0.34
		ear.mesh = cone
		ear.material_override = _material(Color("5d6381"))
		ear.position = Vector3(0.55, 0.55, dz)
		body.add_child(ear)
		_add_wolf_eye(body, Vector3(0.8, 0.23, dz))
	var label := Label3D.new()
	label.text = "WOLF"
	label.font_size = 24
	label.pixel_size = 0.006
	label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	label.position = Vector3(0, 1.5, 0)
	label.modulate = Color("ffb3bd")
	body.add_child(label)
	wolves.append({"body": body, "home": at, "hp": 50.0, "attack": 0.0, "alive": true})

func _add_wolf_eye(body: CharacterBody3D, at: Vector3) -> void:
	var eye := MeshInstance3D.new()
	var sphere := SphereMesh.new()
	sphere.radius = 0.06
	sphere.height = 0.12
	eye.mesh = sphere
	eye.material_override = _material(Color("ed4f79"), 0.15, true)
	eye.position = at
	body.add_child(eye)

func _build_ui() -> void:
	var ui := CanvasLayer.new()
	ui.name = "Interface"
	add_child(ui)
	var root := Control.new()
	root.name = "HUDRoot"
	root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	ui.add_child(root)
	hud = _ui_label(root, Vector2(18, 12), 22, Color("f5e3bd"))
	hint = _ui_label(root, Vector2(18, 50), 15, Color("c1d9d0"))
	status = _ui_label(root, Vector2(18, 81), 16, Color("f9cf88"))
	var controls := _ui_label(root, Vector2(18, 0), 15, Color("bac9e6"))
	controls.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_LEFT)
	controls.position = Vector2(18, -72)
	controls.text = "WASD / Arrows: move  •  Shift: run  •  Left click / Space: strike  •  G: interact  •  F: Hunter Senses  •  Esc: menu"
	start_screen = _make_menu(root, "CHOOSE YOUR HERO")
	var columns := HBoxContainer.new()
	columns.alignment = BoxContainer.ALIGNMENT_CENTER
	columns.add_theme_constant_override("separation", 32)
	columns.position = Vector2(70, 96)
	columns.custom_minimum_size = Vector2(760, 380)
	start_screen.add_child(columns)
	for entry in [{"id": "z", "name": "Z • Void Reaper", "art": Z_ART}, {"id": "nyra", "name": "Nyra • Eclipse Queen", "art": NYRA_ART}]:
		var v := VBoxContainer.new()
		v.custom_minimum_size = Vector2(340, 380)
		columns.add_child(v)
		var portrait := TextureRect.new()
		portrait.texture = entry.art
		portrait.custom_minimum_size = Vector2(320, 290)
		portrait.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		portrait.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		v.add_child(portrait)
		var button := Button.new()
		button.text = "PLAY AS " + entry.name
		button.custom_minimum_size = Vector2(320, 56)
		button.pressed.connect(_start_game.bind(entry.id))
		v.add_child(button)
	if FileAccess.file_exists(SAVE_PATH):
		var resume := Button.new()
		resume.text = "CONTINUE SAVED NATIVE PROTOTYPE"
		resume.position = Vector2(270, 490)
		resume.size = Vector2(380, 45)
		resume.pressed.connect(_load_from_main_menu)
		start_screen.add_child(resume)
	pause_screen = _make_menu(root, "PAUSED • MOONFALL NATIVE")
	pause_screen.visible = false
	var actions := VBoxContainer.new()
	actions.position = Vector2(200, 128)
	actions.custom_minimum_size = Vector2(510, 300)
	pause_screen.add_child(actions)
	for entry in [{"name": "RESUME", "action": "resume"}, {"name": "SAVE CHECKPOINT", "action": "save"}, {"name": "LOAD CHECKPOINT", "action": "load"}, {"name": "RETURN TO HERO SELECT", "action": "title"}]:
		var btn := Button.new()
		btn.text = entry.name
		btn.custom_minimum_size = Vector2(460, 58)
		btn.pressed.connect(_menu_action.bind(entry.action))
		actions.add_child(btn)
	_update_hud()

func _ui_label(parent: Node, at: Vector2, font_size: int, color: Color) -> Label:
	var label := Label.new()
	label.position = at
	label.add_theme_font_size_override("font_size", font_size)
	label.add_theme_color_override("font_color", color)
	parent.add_child(label)
	return label

func _make_menu(parent: Control, title: String) -> Control:
	var panel := Control.new()
	panel.name = "Menu"
	panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	panel.mouse_filter = Control.MOUSE_FILTER_STOP
	parent.add_child(panel)
	var veil := ColorRect.new()
	veil.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	veil.color = Color(0.025, 0.02, 0.06, 0.96)
	panel.add_child(veil)
	var heading := _ui_label(panel, Vector2(72, 30), 34, Color("e9c587"))
	heading.text = "" + title
	var note := _ui_label(panel, Vector2(72, 78), 15, Color("a1aac7"))
	note.text = "Native isometric Witchwood vertical slice • Original character art • Godot 4"
	return panel

func _start_game(selected: String) -> void:
	hero = "nyra" if selected == "nyra" else "z"
	hero_sprite.texture = NYRA_ART if hero == "nyra" else Z_ART
	hero_sprite.pixel_size = 2.65 / float(hero_sprite.texture.get_height())
	hero_playing = true
	menu_open = false
	start_screen.visible = false
	pause_screen.visible = false
	_show_message("Entering Witchwood as " + ("Nyra" if hero == "nyra" else "Z") + ". Find the moonlit shrine!", 5.0)
	_update_hud()

func _load_from_main_menu() -> void:
	if not FileAccess.file_exists(SAVE_PATH):
		return
	var data := _read_save()
	if data.is_empty():
		_show_message("Save is invalid.", 4.0)
		return
	_start_game(str(data.get("hero", "z")))
	_restore_data(data)

func _menu_action(action: String) -> void:
	match action:
		"resume": _toggle_menu()
		"save":
			_save_game()
			_toggle_menu()
		"load":
			_load_game()
			_toggle_menu()
		"title":
			hero_playing = false
			menu_open = false
			pause_screen.visible = false
			start_screen.visible = true

func _unhandled_key_input(event: InputEvent) -> void:
	if not (event is InputEventKey) or not event.pressed or event.echo:
		return
	if event.keycode == KEY_ESCAPE or event.keycode == KEY_F10:
		if hero_playing:
			_toggle_menu()
		get_viewport().set_input_as_handled()
		return
	if not hero_playing or menu_open:
		return
	match event.keycode:
		KEY_G:
			_interact()
		KEY_F:
			senses_on = not senses_on
			for clue in clues:
				if is_instance_valid(clue):
					clue.visible = senses_on
			_show_message("Hunter Senses: " + ("ON" if senses_on else "OFF"), 2.2)
		KEY_SPACE:
			_attack()
	get_viewport().set_input_as_handled()

func _unhandled_input(event: InputEvent) -> void:
	if not hero_playing or menu_open:
		return
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
		_attack()

func _toggle_menu() -> void:
	menu_open = not menu_open
	pause_screen.visible = menu_open
	_update_hud()

func _physics_process(delta: float) -> void:
	if not hero_playing or menu_open:
		return
	attack_cooldown = maxf(0.0, attack_cooldown - delta)
	if status_seconds > 0.0:
		status_seconds -= delta
		if status_seconds <= 0.0:
			status_message = ""
	var dir := Vector3.ZERO
	if Input.is_key_pressed(KEY_W) or Input.is_key_pressed(KEY_UP): dir.z -= 1.0
	if Input.is_key_pressed(KEY_S) or Input.is_key_pressed(KEY_DOWN): dir.z += 1.0
	if Input.is_key_pressed(KEY_A) or Input.is_key_pressed(KEY_LEFT): dir.x -= 1.0
	if Input.is_key_pressed(KEY_D) or Input.is_key_pressed(KEY_RIGHT): dir.x += 1.0
	dir = dir.normalized()
	var speed := SPRINT_SPEED if Input.is_key_pressed(KEY_SHIFT) else WALK_SPEED
	player.velocity = dir * speed
	player.move_and_slide()
	player.position.x = clampf(player.position.x, -MAP_LIMIT, MAP_LIMIT)
	player.position.z = clampf(player.position.z, -MAP_LIMIT, MAP_LIMIT)
	if dir.length_squared() > 0.01:
		# The source image is a front-facing illustration, not a fabricated turn animation.
		hero_sprite.flip_h = dir.x < -0.2
	_update_wolves(delta)
	_update_hud()

func _process(delta: float) -> void:
	if is_instance_valid(camera):
		_update_camera(delta)
	if is_instance_valid(shrine_light):
		shrine_light.light_energy = (3.3 if shrine_awakened else 1.6) + 0.15 * sin(Time.get_ticks_msec() * 0.002)

func _update_camera(delta: float) -> void:
	if not is_instance_valid(player) or not is_instance_valid(camera):
		return
	var center := player.global_position + Vector3(0, 0.75, 0)
	var goal := center + Vector3(14.0, 19.0, 14.0)
	camera.global_position = goal if delta >= 1.0 else camera.global_position.lerp(goal, minf(delta * 5.0, 1.0))
	camera.look_at(center, Vector3.UP)

func _update_wolves(delta: float) -> void:
	for wolf in wolves:
		if not wolf.alive:
			continue
		var body: CharacterBody3D = wolf.body
		if not is_instance_valid(body):
			continue
		var diff := player.global_position - body.global_position
		diff.y = 0.0
		var distance := diff.length()
		wolf.attack = maxf(0.0, float(wolf.attack) - delta)
		if distance < WOLF_AGGRO_RANGE and distance > WOLF_ATTACK_RANGE:
			body.velocity = diff.normalized() * 2.4
		elif distance >= WOLF_AGGRO_RANGE and body.global_position.distance_to(wolf.home) > 2.0:
			var home_dir: Vector3 = wolf.home - body.global_position
			home_dir.y = 0.0
			body.velocity = home_dir.normalized() * 1.3
		else:
			body.velocity = Vector3.ZERO
		body.move_and_slide()
		if body.velocity.length_squared() > 0.01:
			body.rotation.y = atan2(-body.velocity.z, body.velocity.x)
		if distance < WOLF_ATTACK_RANGE and wolf.attack <= 0.0:
			hp = maxf(0.0, hp - 10.0)
			wolf.attack = 1.3
			_show_message("Witchwood wolf strikes for 10!", 1.1)
			if hp <= 0.0:
				_respawn_player()

func _attack() -> void:
	if attack_cooldown > 0.0:
		return
	attack_cooldown = 0.36
	var nearest: Dictionary = {}
	var distance := 3.4
	for wolf in wolves:
		if not wolf.alive or not is_instance_valid(wolf.body):
			continue
		var d := player.global_position.distance_to(wolf.body.global_position)
		if d < distance:
			distance = d
			nearest = wolf
	if nearest.is_empty():
		_show_message("Strike! Move closer to a wolf.", 0.8)
		return
	nearest.hp = float(nearest.hp) - (32.0 if hero == "z" else 28.0)
	_show_message("Hit wolf for " + ("32" if hero == "z" else "28") + ".", 1.0)
	if nearest.hp <= 0.0:
		nearest.alive = false
		kills += 1
		var at: Vector3 = nearest.body.position
		nearest.body.queue_free()
		_drop_loot(at)
		_show_message("Wolf defeated. A rare Moonfang dropped!", 3.0)

func _drop_loot(at: Vector3) -> void:
	var root := Node3D.new()
	root.name = "MoonfangGroundLoot"
	root.position = Vector3(at.x, 0.0, at.z)
	world.add_child(root)
	var beam := _add_sphere(root.position + Vector3(0, 0.35, 0), 0.22, Color("65a9ff"), false, true)
	beam.reparent(root)
	beam.position = Vector3(0, 0.35, 0)
	var light := OmniLight3D.new()
	light.position = Vector3(0, 1.0, 0)
	light.light_color = Color("528cff")
	light.light_energy = 1.0
	light.omni_range = 3.0
	root.add_child(light)
	var tag := Label3D.new()
	tag.text = "◆ MOONFANG  (G)"
	tag.font_size = 26
	tag.pixel_size = 0.005
	tag.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	tag.position = Vector3(0, 1.25, 0)
	tag.modulate = Color("9dd7ff")
	root.add_child(tag)
	loot_drops.append({"node": root, "value": 12})

func _interact() -> void:
	for drop in loot_drops.duplicate():
		if is_instance_valid(drop.node) and player.global_position.distance_to(drop.node.global_position) < 3.2:
			gold += int(drop.value)
			drop.node.queue_free()
			loot_drops.erase(drop)
			_show_message("Collected Moonfang trophy: +12 gold.", 2.5)
			return
	if player.global_position.distance_to(SHRINE_POS) < 4.2:
		shrine_awakened = true
		hp = max_hp
		_save_game()
		_show_message("Witchwood shrine awakened! Health restored and checkpoint saved.", 3.5)
		return
	_show_message("No nearby item or shrine. Hold F to reveal clues.", 1.6)

func _respawn_player() -> void:
	hp = max_hp
	player.position = SHRINE_POS + Vector3(0, 0.85, 4.0) if shrine_awakened else Vector3(0, 0.85, 0)
	_show_message("You were defeated. Returned to checkpoint.", 3.2)

func _show_message(text: String, seconds: float = 2.0) -> void:
	status_message = text
	status_seconds = seconds
	if is_instance_valid(status):
		status.text = text

func _update_hud() -> void:
	if not is_instance_valid(hud):
		return
	hud.text = "MOONFALL NATIVE • " + ("NYRA" if hero == "nyra" else "Z") + "   ♥ " + str(int(hp)) + "/100   ◆ " + str(gold) + " gold   ⚔ " + str(kills) + " wolves"
	hint.text = ("PAUSED" if menu_open else "HUNTER SENSES ACTIVE" if senses_on else "SHRINE: AWAKENED" if shrine_awakened else "OBJECTIVE: AWAKEN THE WITCHWOOD SHRINE")
	status.text = status_message

func _save_game() -> void:
	if not hero_playing:
		return
	var data := {"schema": 1, "hero": hero, "position": [player.position.x, player.position.z], "hp": hp, "kills": kills, "gold": gold, "shrine": shrine_awakened}
	var file := FileAccess.open(SAVE_PATH, FileAccess.WRITE)
	if file == null:
		_show_message("Checkpoint could not be written.", 3.0)
		return
	file.store_string(JSON.stringify(data))
	file.close()
	_show_message("Checkpoint saved on this computer.", 2.5)

func _read_save() -> Dictionary:
	if not FileAccess.file_exists(SAVE_PATH):
		return {}
	var file := FileAccess.open(SAVE_PATH, FileAccess.READ)
	if file == null:
		return {}
	var parsed: Variant = JSON.parse_string(file.get_as_text())
	if typeof(parsed) != TYPE_DICTIONARY:
		return {}
	var d: Dictionary = parsed
	if int(d.get("schema", -1)) != 1 or not (str(d.get("hero", "")) in ["z", "nyra"]):
		return {}
	return d

func _load_game() -> void:
	var d := _read_save()
	if d.is_empty():
		_show_message("No valid native checkpoint found.", 3.5)
		return
	_start_game(str(d.hero))
	_restore_data(d)

func _restore_data(d: Dictionary) -> void:
	var coords: Variant = d.get("position", [])
	if coords is Array and coords.size() == 2:
		var x := float(coords[0])
		var z := float(coords[1])
		if is_finite(x) and is_finite(z):
			player.position = Vector3(clampf(x, -MAP_LIMIT, MAP_LIMIT), 0.85, clampf(z, -MAP_LIMIT, MAP_LIMIT))
	hp = clampf(float(d.get("hp", 100)), 1.0, 100.0)
	kills = clampi(int(d.get("kills", 0)), 0, 999999)
	gold = clampi(int(d.get("gold", 0)), 0, 9999999)
	shrine_awakened = bool(d.get("shrine", false))
	_show_message("Native checkpoint restored.", 2.7)
	_update_hud()
