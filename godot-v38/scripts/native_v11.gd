extends "res://scripts/main.gd"
const V383_COMBAT_RULES:=preload("res://scripts/systems/procedural_variants_v382.gd")
const V383_FEEDBACK:=preload("res://scripts/systems/combat_feedback_v383.gd")
const MINI_MAP_SCRIPT := preload("res://scripts/native_minimap.gd")
# v26: Stateless modules isolate persistence and progression from the world scene.
const SAVE_STORE := preload("res://scripts/systems/checkpoint_store.gd")
const GEAR_CODEC := preload("res://scripts/systems/gear_codec.gd")
const HERO_RULES := preload("res://scripts/systems/hero_rules.gd")
const SPAWN_RULES := preload("res://scripts/systems/spawn_rules.gd")
const AI_RULES := preload("res://scripts/systems/ai_rules.gd")
const COMBAT_RULES := preload("res://scripts/systems/combat_rules.gd")
# v30.3: Calendar, weather and lighting are maintained in a pure, testable module.
const WORLD_CYCLE := preload("res://scripts/systems/world_cycle.gd")
# v30.4: inventory, crafting and loot rules separated from world interactions.
const INVENTORY_RULES := preload("res://scripts/systems/inventory_rules.gd")
const MONEY_RULES := preload("res://scripts/systems/money_rules.gd")
## Moonfall Native v11: living-world simulation, regional encounters, AI retreat and native fixes.
## Retains v9 save schema and its six-region single-world architecture.
## Source images for Z and Nyra are inherited unmodified from main.gd.

const SAVE_V2 := "user://moonfall_native_v2.json"  # Save schema 2 stays compatible
const SAVE_BACKUP := "user://moonfall_native_v2.backup.json"
const SAVE_STAGING := "user://moonfall_native_v2.pending.json"
const MAX_GROUND_DROPS := 48
const MAX_CARCASSES := 40
const NORMAL_RESPAWN_SECONDS := 165.0
const ECOLOGY_RESPAWN_SECONDS := 120.0
const MIN_RESPAWN_DISTANCE := 13.0
const REGION_DRAW_X := 52.0
const REGION_DRAW_Z := 49.0
const REGION_REFRESH_SECONDS := 0.28
const RESPAWN_REFRESH_SECONDS := 0.45
const BEAST_THREAT_RANGE := 8.8
const BOSS_WINDUP_SECONDS := 1.5
const WORLD_X := 89.0
const WORLD_Z := 60.0
const HERO_START := Vector3(-60.0, 0.85, -29.0)
const SENSE_DISTANCE := 13.0
const RIDE_COMBAT_SECONDS := 20.0
const REGIONS := [
 {"name":"Hearthstone Outpost","at":Vector2(-60,-30),"land":"outpost","ground":"364037","light":"efd084","monster":"boar","tier":1},
 {"name":"Witchwood Wilds","at":Vector2(0,-30),"land":"forest","ground":"1b4035","light":"b29cff","monster":"wolf","tier":1},
 {"name":"Moonmist Marsh","at":Vector2(60,-30),"land":"swamp","ground":"1b3841","light":"80d0fd","monster":"serpent","tier":2},
 {"name":"Highcrest Quarry","at":Vector2(-60,30),"land":"quarry","ground":"5a544e","light":"e3b36b","monster":"boar","tier":2},
 {"name":"Shattered Ruins","at":Vector2(0,30),"land":"ruins","ground":"383046","light":"b990ff","monster":"wolf","tier":3},
 {"name":"Eclipse Citadel","at":Vector2(60,30),"land":"citadel","ground":"391d32","light":"f86d88","monster":"panther","tier":4}
]
const BEASTS := {
 "wolf":{"hp":58.0,"atk":10.0,"speed":2.6,"hostile":true,"skin":"Leather","drop":"fang"},
 "stag":{"hp":46.0,"atk":3.0,"speed":3.15,"hostile":false,"skin":"Fine Hide","drop":"antler"},
 "boar":{"hp":72.0,"atk":12.0,"speed":2.25,"hostile":true,"skin":"Thick Leather","drop":"tusk"},
 "fox":{"hp":37.0,"atk":4.0,"speed":3.4,"hostile":false,"skin":"Soft Pelt","drop":"fur"},
 "serpent":{"hp":81.0,"atk":14.0,"speed":2.0,"hostile":true,"skin":"Serpent Scale","drop":"venom"},
 "panther":{"hp":68.0,"atk":13.0,"speed":3.0,"hostile":true,"skin":"Shadow Pelt","drop":"claw"},
 "elk":{"hp":79.0,"atk":5.0,"speed":2.5,"hostile":false,"skin":"Heavy Hide","drop":"antler"},
 "owl":{"hp":34.0,"atk":3.0,"speed":2.5,"hostile":false,"skin":"Moonfeather","drop":"feather"}
}
const TALENTS := [
 {"id":"gentle","name":"Gentle Hands","branch":"Bond","tip":"Taming range +0.4m/rank."},
 {"id":"familiar","name":"First Trust","branch":"Bond","tip":"New companion health +8%/rank."},
 {"id":"kinship","name":"Shared Journey","branch":"Bond","tip":"Companion damage +5%/rank."},
 {"id":"wildspeak","name":"Wildspeaker","branch":"Bond","tip":"Taming succeeds at higher beast health."},
 {"id":"soulbond","name":"Soulbond","branch":"Bond","tip":"Companion heals over time."},
 {"id":"sharp","name":"Savage Instinct","branch":"Hunt","tip":"Companion attack damage +8%/rank."},
 {"id":"predator","name":"Elite Hunter","branch":"Hunt","tip":"Companion damage to elite beasts +10%/rank."},
 {"id":"stride","name":"Pursuit","branch":"Hunt","tip":"Companion pursuit speed +7%/rank."},
 {"id":"frenzy","name":"Pack Rhythm","branch":"Hunt","tip":"Companion attacks more often."},
 {"id":"apex","name":"Apex Bond","branch":"Hunt","tip":"Companion attacks gain crit chance."},
 {"id":"nurture","name":"Field Care","branch":"Warden","tip":"Feeding restores more health."},
 {"id":"ironhide","name":"Ironhide","branch":"Warden","tip":"Companion takes less damage."},
 {"id":"endurance","name":"Surefoot Mount","branch":"Warden","tip":"Travel mount moves faster."},
 {"id":"recover","name":"Wild Renewal","branch":"Warden","tip":"Companion periodically regenerates."},
 {"id":"guardian","name":"Guardian Call","branch":"Warden","tip":"Companion intercepts nearby threats."},
 {"id":"spirit","name":"Spirit Pact","branch":"Bond","tip":"Increases taming mastery on first species."},
 {"id":"alpha","name":"Alpha Instinct","branch":"Hunt","tip":"Companion attacks stagger targets."},
 {"id":"oath","name":"Guardian's Oath","branch":"Warden","tip":"Companion gains defensive aura."}
]
const RECIPES := [
 {"name":"Leather Jerkin","cost":{"Leather":3,"fang":1},"result":"Leather Jerkin","power":10},
 {"name":"Moonward Boots","cost":{"Fine Hide":2,"Moon Sigil":1},"result":"Moonward Boots","power":18},
 {"name":"Serpent Scale Vest","cost":{"Serpent Scale":3,"venom":2},"result":"Serpent Scale Vest","power":23},
 {"name":"Wolf Fang Charm","cost":{"fang":4,"Leather":1},"result":"Wolf Fang Charm","power":14},
 {"name":"Duskfire Cloak","cost":{"Dusk Cinder":2,"Thick Leather":3},"result":"Duskfire Cloak","power":31},
 {"name":"Elder Guardian Mantle","cost":{"Elder Rootstone":2,"Heavy Hide":3},"result":"Elder Guardian Mantle","power":36},
 {"name":"Healing Draught","cost":{"herb":2,"fur":1},"result":"Healing Draught","power":0},
 {"name":"Hunter's Oil","cost":{"venom":1,"herb":2},"result":"Hunter's Oil","power":0}
]
const PORTRAITS := {
 "moon_wolf":"res://assets/beasts/moon_wolf.webp",
 "moon_stag":"res://assets/beasts/moon_stag.webp",
 "moon_panther":"res://assets/beasts/moon_panther.webp",
 "dusk_boar":"res://assets/beasts/dusk_boar.webp",
 "dusk_fox":"res://assets/beasts/dusk_fox.webp",
 "dusk_bear":"res://assets/beasts/dusk_bear.webp",
 "elder_serpent":"res://assets/beasts/elder_serpent.webp",
 "elder_elk":"res://assets/beasts/elder_elk.webp",
 "elder_owl":"res://assets/beasts/elder_owl.webp"
}

var region_nodes: Array[Node3D] = []
var zone_beast_lists: Array[Array] = []
var region_refresh_tick := 0.0
var respawn_refresh_tick := 0.0
var visible_regions := 0
var ai_updates := 0
var region_shrines: Array[Node3D] = []
var shrine_lights: Array[OmniLight3D] = []
var shrine_discovery: Array[int] = []
var region_index := 0
var beasts: Array[Dictionary] = []
var carcasses: Array[Dictionary] = []
var gear_drops: Array[Dictionary] = []
var materials: Dictionary = {"herb":4}
var gear_items: Array[Dictionary] = []
# v30.6: Keep the legacy whole-gold field to preserve older checkpoint economics.
# Sub-gold denominations and uncollected coin bundles are optional schema-2 fields.
var silver:int=0
var copper:int=0
var coin_drops:Array[Dictionary]=[]
var opened_treasure_chests:Array[int]=[]
var coin_scan_time:float=0.0
const MAX_COIN_DROPS:int=48
# v30.5: Persistent expandable bag and independent 1000-item gear vault.
# Materials stay in the dedicated materials dictionary, never occupy gear slots.
var bag_capacity:int=100
var vault_items:Array[Dictionary]=[]
var inventory_query:String=""
var inventory_filter:String="All"
var inventory_sort:String="Default"
var inventory_page:int=0
var inventory_show_vault:bool=false
var inventory_search_input:LineEdit
var equipped: Dictionary = {}
var skinning_skill := 1
var crafting_skill := 1
# v36.2: separate persistent gathering and making skills, sharing existing materials.
var water_professions:Dictionary={"Fishing":1,"Herbalism":1,"Mining":1,"Cooking":1,"Alchemy":1,"Smithing":1}
var water_site_days:Dictionary={}
var tamer_points := 0
var learned_talents: Dictionary = {}
var discovered_species: Array[String] = []
var companion: Dictionary = {}
var contract_zone := -1
var contracts_done: Array[int] = []
var contract_kills := 0
var mounted := false
var mount_combat_time := 0.0
var dodge_time := 0.0
var dodge_cooldown := 0.0
var ability_cooldown := 0.0
var hunter_oil_time := 0.0
var skinning_target: Node3D
var skinning_left := 0.0
var skinning_origin := Vector3.ZERO
var kills_total := 0
var current_tab := "Character"
var menu_panel: Control
var menu_body: RichTextLabel
var menu_buttons: VBoxContainer
var beast_guide: TextureRect
var world_hud: Label
var companion_node: Node3D
var companion_attack_timer := 0.0
var auto_save_cooldown := 0.0
var last_hero_pos := Vector3.ZERO
var clue_data: Array[Dictionary] = []
var found_clues: Array[int] = []
var defeated_boss_ids: Array[int] = []
var herds_alerted := 0
var hunts_completed := 0
var blocked_hits := 0
var pickup_count := 0
var map_time := 0.0
var ecology_step := 0.0
var ui_snapshot := 0.0
var current_threat := false
var save_health := "UNTESTED"
var graphics_quality := "Full"
var pet_stunned_until := 0.0
var show_tips := true
var suppress_autosave_message := false
var boss_telegraphs: Dictionary = {}
var quick_actions: Control
var respawns_total := 0
var restored_wildlife := 0
# v8 persistent progression and short-lived combat momentum.
var hero_level := 1
var hero_xp := 0
var hero_xp_to_next := 75
var combat_combo := 0
var combat_combo_until := 0.0
var world_challenge := 0
# v9 combat systems use deterministic per-creature timers instead of creating nodes each frame.
var critical_hits := 0
var stagger_events := 0
var elite_attacks_dodged := 0
var last_hit_damage := 0
var last_hit_critical := false
const STAGGER_DURATION := 0.65
const STAGGER_RESIST_SECONDS := 1.4
const ELITE_LUNGE_RANGE := 5.2
const RIFT_WAVES := 3
const RIFT_SITES := [
 {"zone":1,"name":"Witchwood Hollow","kind":"wolf"},
 {"zone":4,"name":"Shattered Reliquary","kind":"wolf"},
 {"zone":5,"name":"Eclipse Crucible","kind":"panther"}
]
const HERO_PERKS := [
 {"id":"rend","branch":"Assault","name":"Moonsteel","tip":"Melee damage +7% per rank."},
 {"id":"execute","branch":"Assault","name":"Final Cut","tip":"Damage against wounded enemies +10% per rank."},
 {"id":"bloodmoon","branch":"Assault","name":"Blood Moon","tip":"Critical hits restore 3 health per rank."},
 {"id":"ward","branch":"Survival","name":"Moonward","tip":"Incoming damage -4% per rank."},
 {"id":"vigor","branch":"Survival","name":"Vital Ember","tip":"Maximum health +12 per rank."},
 {"id":"renewal","branch":"Survival","name":"Life From Victory","tip":"Defeats heal 4 health per rank."},
 {"id":"focus","branch":"Arcana","name":"Arcane Focus","tip":"Spell damage +8% per rank."},
 {"id":"reach","branch":"Arcana","name":"Moon Echo","tip":"Spell radius +0.5m per rank."},
 {"id":"celerity","branch":"Arcana","name":"Swift Invocation","tip":"Spell cooldown -8% per rank."}
]
const AFFIXES := ["Fury","Aegis","Vampiric","Haste"]
var hero_points := 0
var hero_perks:Dictionary = {}
var rifts:Array[Dictionary] = []
var rifts_cleared:Array[int] = []
var active_rift := -1
var bosses_transformed := 0
var rifts_completed := 0

# v11 keeps weather, time and regional encounters within the existing world.
# Slots and markers are built once, never generated per frame.
const DAY_LENGTH_SECONDS := 480.0
const WEATHER_INTERVAL_SECONDS := 145.0
const WEATHER_TYPES := ["Clear", "Mist", "Rain", "Eclipse"]
const WORLD_EVENT_GOAL := 3
const WORLD_EVENT_SITES := [
 {"name":"Hearthstone Supply Cache","kind":"cache","zone":0,"material":"herb"},
 {"name":"Witchwood Pack Hunt","kind":"hunt","zone":1,"material":"fang"},
 {"name":"Moonmist Alchemist's Cache","kind":"cache","zone":2,"material":"venom"},
 {"name":"Quarry Ravager Hunt","kind":"hunt","zone":3,"material":"tusk"},
 {"name":"Shattered Relic Cache","kind":"cache","zone":4,"material":"Moon Sigil"},
 {"name":"Eclipse Citadel Hunt","kind":"hunt","zone":5,"material":"Dusk Cinder"}
]
var world_clock := 240.0
var world_day := 1
var weather_name := "Clear"
var weather_remaining := WEATHER_INTERVAL_SECONDS
var weather_changes := 0
var living_visual_tick := 0.0
var living_environment: Environment
var world_events: Array[Dictionary] = []
var events_completed: Array[int] = []
var active_world_event := -1
var active_event_kills := 0
var enemy_retreats := 0
# v13: keyboard, controller, region power guidance and companion tactical stances.
var minimap: Control
var hero_start_button: Button
var atlas_first_tab: Button
var minimap_visible := true
var companion_stance := "Guard"
var scenery_light_tick := 0.0
const GAMEPAD_DEADZONE := 0.22


func _award_experience(zone: int, variant: String, boss: bool) -> void:
 var earned := 15 + zone * 8
 if variant != "normal":earned += 12
 if boss:earned += 100
 if map_time <= combat_combo_until:combat_combo = mini(combat_combo + 1, 12)
 else:combat_combo = 1
 combat_combo_until = map_time + 7.0
 earned += mini(10, combat_combo - 1) * 2
 hero_xp += earned
 var levels_gained := 0
 while hero_xp >= hero_xp_to_next and hero_level < 100:
  hero_xp -= hero_xp_to_next
  hero_level += 1
  levels_gained += 1
  hero_xp_to_next = 75 + (hero_level - 1) * 40
  tamer_points += 1
  hero_points += 1
 if hero_level >= 100:hero_xp = mini(hero_xp,hero_xp_to_next)
 if levels_gained > 0:
  hp = minf(max_hp,hp + 20.0 * levels_gained)
  _show_message("LEVEL UP! Level " + str(hero_level) + " • +" + str(levels_gained) + " Tamer point(s)",3.0)

func _hero_damage_multiplier() -> float:
 return HERO_RULES.damage_multiplier(hero_level, combat_combo, map_time <= combat_combo_until)

func _recommended_power(zone: int) -> int:
 return HERO_RULES.recommended_power(zone, REGIONS)

func _current_power() -> int:
 return hero_level * 11 + _equipment_power()

func _power_modifier(zone: int) -> float:
 return HERO_RULES.power_modifier(_current_power(), _recommended_power(zone))

func _cycle_companion_stance() -> void:
 match companion_stance:
  "Guard":companion_stance="Follow"
  "Follow":companion_stance="Passive"
  _:companion_stance="Guard"
 _show_message("Companion stance: " + companion_stance, 1.5)
 if menu_open:_menu_tab("Tamer")

func _toggle_minimap() -> void:
 minimap_visible = not minimap_visible
 if is_instance_valid(minimap):minimap.visible = minimap_visible and hero_playing
 _show_message("Minimap " + ("ON" if minimap_visible else "OFF"), 1.0)

func _gamepad_direction() -> Vector3:
 var pads: Array[int] = Input.get_connected_joypads()
 if pads.is_empty():return Vector3.ZERO
 var id:int = pads[0]
 var stick:=Vector2(Input.get_joy_axis(id,JOY_AXIS_LEFT_X),Input.get_joy_axis(id,JOY_AXIS_LEFT_Y))
 if stick.length() < GAMEPAD_DEADZONE:return Vector3.ZERO
 return Vector3(stick.x, 0.0, stick.y).limit_length(1.0)

## Camera mode hook. Legacy versions keep world-axis movement; 3D uses view-relative movement.
func _map_movement_to_camera(raw_direction:Vector3)->Vector3:
 return raw_direction

func _ready() -> void:
 rng.seed = 10262026
 _build_scene()
 _build_ui()
 _update_region_visibility()
 _show_message("Moonfall Native v13: one world, map, power balance, controller and companion stances.", 5.0)

func _build_scene() -> void:
 world = Node3D.new()
 world.name = "SixRegionWorld"
 add_child(world)
 var env := WorldEnvironment.new()
 var settings := Environment.new()
 settings.background_mode = Environment.BG_COLOR
 settings.background_color = Color("070b19")
 settings.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
 settings.ambient_light_color = Color("879ac9")
 settings.ambient_light_energy = 0.55
 settings.tonemap_mode = Environment.TONE_MAPPER_FILMIC
 settings.fog_enabled = true
 settings.fog_light_color = Color("1d2340")
 settings.fog_density = 0.008
 env.environment = settings
 living_environment = settings
 add_child(env)
 var moon := DirectionalLight3D.new()
 moon.name = "TwinMoonlight"
 moon.rotation_degrees = Vector3(-56, -35, 0)
 moon.light_color = Color("b0a6ff")
 moon.light_energy = 1.0
 moon.shadow_enabled = true
 moon.directional_shadow_max_distance = 43.0
 add_child(moon)
 _add_box(Vector3(0,-0.19,0),Vector3(180,0.32,121),Color("111c20"),true,"OneContinuousWorldFloor")
 zone_beast_lists.clear()
 for i in range(REGIONS.size()):zone_beast_lists.append([])
 for i in range(REGIONS.size()):
  _build_region(i)
 _build_roads()
 _create_player()
 player.position = HERO_START
 for i in range(REGIONS.size()):
  for encounter in SPAWN_RULES.region_encounters(i,REGIONS[i]):
   _spawn_beast(encounter["at"],encounter["kind"],i,encounter["variant"],encounter["boss"])
 _build_rift_encounters()
 _build_world_events()
 camera=Camera3D.new()
 camera.name="NativeIsometricCamera"
 camera.projection=Camera3D.PROJECTION_ORTHOGONAL
 camera.size=19.0
 camera.near=0.05
 camera.far=240.0
 camera.current=true
 add_child(camera)
 _update_camera(1.0)

func _build_roads() -> void:
 for z in [-30.0,30.0]:
  for x in range(-86,87,4):
   _add_box(Vector3(float(x),0.015,z),Vector3(3.8,0.03,2.8),Color("615749"),false,"AncientRoad")
 for x in [-60.0,0.0,60.0]:
  for z in range(-25,27,4):
   _add_box(Vector3(x,0.015,float(z)),Vector3(2.8,0.03,3.8),Color("615749"),false,"ConnectingRoad")

func _build_region(i: int) -> void:
 var entry: Dictionary=REGIONS[i]
 var center: Vector2=entry["at"]
 var zone_root:=Node3D.new()
 zone_root.name=str(entry["name"]).replace(" ","")
 world.add_child(zone_root)
 region_nodes.append(zone_root)
 var initial_world_children:=world.get_child_count()
 _add_box(Vector3(center.x,-0.012,center.y),Vector3(59.5,0.025,59.5),Color(str(entry["ground"])),false,"RegionGround")
 var title:=Label3D.new()
 title.text=str(entry["name"])
 title.font_size=42
 title.pixel_size=0.008
 title.billboard=BaseMaterial3D.BILLBOARD_ENABLED
 title.modulate=Color(str(entry["light"]))
 title.position=Vector3(center.x,4.5,center.y-5.0)
 zone_root.add_child(title)
 # The same physical props render and collide through the shared helpers.
 for j in range(25):
  var offset:=Vector2(rng.randf_range(-27,27),rng.randf_range(-26,26))
  if absf(offset.x)<6.5 or absf(offset.y)<5.0 or offset.length()<9.0:continue
  var p:=Vector3(center.x+offset.x,0,center.y+offset.y)
  match str(entry["land"]):
   "forest":
    if j%4==0:_add_boulder(p,0.55+rng.randf()*0.55)
    else:_add_pine(p,0.70+rng.randf()*0.55)
   "swamp":
    if j%4==0:_add_sphere(p+Vector3(0,0.09,0),rng.randf_range(0.5,1.1),Color("20677d"),false,true)
    else:_add_cylinder(p+Vector3(0,1.3,0),0.26,2.6,Color("344738"),true)
   "quarry":
    if j%3==0:_add_cylinder(p+Vector3(0,1.1,0),0.4,2.2,Color("b39e7a"),true,0.14)
    else:_add_boulder(p,rng.randf_range(0.6,1.25))
   "ruins":
    if j%2==0:_add_box(p+Vector3(0,1.0,0),Vector3(1.1,2.0,1.1),Color("74677f"),true,"BrokenPillar")
    else:_add_boulder(p,0.7)
   "citadel":
    if j%3==0:_add_cylinder(p+Vector3(0,2.1,0),0.56,4.2,Color("553b51"),true,0.35)
    else:_add_box(p+Vector3(0,0.65,0),Vector3(1.6,1.3,1.6),Color("493140"),true,"CitadelStone")
   _:
    if j%3==0:_add_box(p+Vector3(0,0.6,0),Vector3(1.2,1.2,0.65),Color("726454"),true,"OutpostFence")
    else:_add_pine(p,rng.randf_range(0.66,1.0))
 for j in range(7):
  var a :=float(j)*TAU/7.0
  var at :=Vector3(center.x+sin(a)*20.0,0.25,center.y+cos(a)*18.0)
  _add_sphere(at,0.16,Color(str(entry["light"])),false,true)
 _make_shrine(i)
 for clue_no in range(3):
  var clue_pos:=Vector3(center.x-18.0+float(clue_no)*18.0,0.27,center.y+10.0+float(clue_no)*5.0)
  var trace:=_add_sphere(clue_pos,0.24,Color(str(entry["light"])),false,true)
  trace.visible=false
  var clue_id:=clue_data.size()
  clue_data.append({"id":clue_id,"node":trace,"zone":i})
  clues.append(trace)
 # Move this region's renderable props into its visual container.
 # Hiding Node3D visual descendants does not disable their StaticBody3D collisions.
 var new_nodes:Array[Node]=[]
 for index in range(initial_world_children,world.get_child_count()):
  new_nodes.append(world.get_child(index))
 for node in new_nodes:
  node.reparent(zone_root,true)

func _update_region_visibility() -> void:
 if not is_instance_valid(player):return
 var count:=0
 for i in range(region_nodes.size()):
  var center:Vector2=REGIONS[i]["at"]
  var nearby:=absf(player.global_position.x-center.x)<REGION_DRAW_X and absf(player.global_position.z-center.y)<REGION_DRAW_Z
  region_nodes[i].visible=nearby
  if nearby:count+=1
 visible_regions=count
 # Creature nodes remain simulated only in the active zone and render only
 # in nearby sectors; their collision is unchanged by visual visibility.
 for zone in range(zone_beast_lists.size()):
  var visible:=region_nodes[zone].visible
  for creature in zone_beast_lists[zone]:
   var body:CharacterBody3D=creature["body"]
   if is_instance_valid(body) and bool(creature.get("alive",false)):
    body.visible=visible
    if not visible:body.velocity=Vector3.ZERO

func _make_shrine(i: int) -> void:
 var center: Vector2=REGIONS[i]["at"]
 var pos:=Vector3(center.x+3.0,0.0,center.y-7.5)
 var shrine:=Node3D.new()
 shrine.position=pos
 shrine.name="FastTravel_"+str(i)
 world.add_child(shrine)
 region_shrines.append(shrine)
 _add_box(pos+Vector3(0,0.28,0),Vector3(2.8,0.55,2),Color("61516e"),true,"ShrineFoundation")
 _add_box(pos+Vector3(0,1.0,0),Vector3(0.9,1.5,0.7),Color("8b749b"),false,"ShrineColumn")
 _add_sphere(pos+Vector3(0,1.95,0),0.42,Color(str(REGIONS[i]["light"])),false,true)
 var light:=OmniLight3D.new()
 light.light_color=Color(str(REGIONS[i]["light"]))
 light.light_energy=1.0
 light.omni_range=6.5
 light.position=pos+Vector3(0,2.0,0)
 world.add_child(light)
 shrine_lights.append(light)

func _spawn_beast(at: Vector3, kind: String, zone: int, variant: String="normal", boss: bool=false) -> void:
 var body:=CharacterBody3D.new()
 body.name=kind.capitalize()+"_"+variant
 body.position=at+Vector3(0,0.62,0)
 world.add_child(body)
 var collider:=CollisionShape3D.new()
 var capsule:=CapsuleShape3D.new()
 capsule.radius=0.42
 capsule.height=1.24
 collider.shape=capsule
 body.add_child(collider)
 var art:=MeshInstance3D.new()
 var mesh:=CapsuleMesh.new()
 mesh.radius=0.43
 mesh.height=1.35
 art.mesh=mesh
 art.rotation_degrees=Vector3(0,0,90)
 var base_color:=Color("515d78")
 if kind in ["stag","elk","fox"]:base_color=Color("977b67")
 if kind=="serpent":base_color=Color("37775d")
 if kind=="boar":base_color=Color("695054")
 if kind=="panther":base_color=Color("25233e")
 if variant=="moon":base_color=Color("936fff")
 if variant=="dusk":base_color=Color("ee7348")
 if variant=="elder":base_color=Color("69bd71")
 art.material_override=_material(base_color,0.75,variant!="normal")
 body.add_child(art)
 var head:=MeshInstance3D.new()
 var headmesh:=SphereMesh.new()
 headmesh.radius=0.31
 headmesh.height=0.63
 head.mesh=headmesh
 head.position=Vector3(0.5,0.15,0)
 head.material_override=_material(base_color.lightened(0.12))
 body.add_child(head)
 var label:=Label3D.new()
 label.text="ZONE GUARDIAN • "+kind.capitalize() if boss else (variant.capitalize()+" "+kind.capitalize() if variant!="normal" else kind.capitalize())
 label.font_size=24
 label.pixel_size=0.005
 label.billboard=BaseMaterial3D.BILLBOARD_ENABLED
 label.position=Vector3(0,1.35,0)
 label.modulate=Color(str(REGIONS[zone]["light"])) if variant!="normal" else Color("f4dcbb")
 body.add_child(label)
 var def:Dictionary=BEASTS[kind]
 var scaling:float=SPAWN_RULES.health_scale(int(REGIONS[zone]["tier"]),variant,boss)
 var spawn_id:=beasts.size()
 var creature:Dictionary={"body":body,"kind":kind,"variant":variant,"zone":zone,"hp":float(def["hp"])*scaling,"max":float(def["hp"])*scaling,"attack":0.0,"alive":true,"hostile":bool(def["hostile"]),"home":at,"rewarded":false,"boss":boss,"boss_id":spawn_id,"windup":0.0,"fed_until":0.0,"hunting_until":0.0,"scared_until":0.0,"engaged":false,"respawn_at":0.0,"stagger_until":0.0,"stagger_resist_until":0.0,"elite_lunge_cd":2.0,"elite_lunge_windup":0.0,"elite_lunge_target":Vector3.ZERO,"boss_phase":1,"boss_pattern":0,"warning_radius":2.8,"nameplate":label,"hit_visual":art,"hit_react_until":0.0,"hit_react_critical":false,"hit_flash_until":0.0,"attack_swing_until":0.0,"alert_until":0.0,"dungeon_site":-1,"base_hp":float(def["hp"])*scaling,"retreat_until":0.0,"retreat_cooldown":0.0}
 beasts.append(creature)
 zone_beast_lists[zone].append(creature)

func _build_rift_encounters() -> void:
 # Reusable encounter slots: no new physics actors created when starting waves.
 for site in RIFT_SITES:
  var zone:int=int(site["zone"])
  var mid:Vector2=REGIONS[zone]["at"]
  var anchor:=Vector3(mid.x-7.0,0.0,mid.y+16.0)
  var sigil:=_add_sphere(anchor+Vector3(0,0.65,0),0.64,Color("a457ff"),false,true)
  sigil.name="PersistentRiftAnchor"
  var title:=Label3D.new()
  title.text="RIFT • "+str(site["name"])+"\n(G) Start encounter"
  title.font_size=28
  title.pixel_size=0.006
  title.billboard=BaseMaterial3D.BILLBOARD_ENABLED
  title.position=Vector3(0,1.8,0)
  sigil.add_child(title)
  var entry:Dictionary={"name":str(site["name"]),"zone":zone,"anchor":anchor,"marker":sigil,"label":title,"stage":0,"active":false,"cleared":false,"slots":[]}
  var site_id:=rifts.size()
  rifts.append(entry)
  for n in range(3):
   var angle:=float(n)*TAU/3.0
   var creature_at:=anchor+Vector3(cos(angle)*5.0,0,sin(angle)*5.0)
   _spawn_beast(creature_at,str(site["kind"]),zone,"dusk" if n==2 else "moon")
   var creature:Dictionary=beasts[-1]
   creature["dungeon_site"]=site_id
   entry["slots"].append(int(creature["boss_id"]))
   _despawn_native_beast(creature,0.0)
  _set_rift_label(entry)

func _set_rift_label(entry:Dictionary) -> void:
 var label:Label3D=entry["label"]
 if not is_instance_valid(label):return
 if bool(entry["cleared"]):
  label.text="✓ "+str(entry["name"])+" CLEARED"
 elif bool(entry["active"]):
  label.text=str(entry["name"])+" • Wave "+str(entry["stage"])+" / "+str(RIFT_WAVES)
 else:
  label.text="RIFT • "+str(entry["name"])+"\n(G) Start encounter"

func _start_rift(site_id:int) -> void:
 if site_id<0 or site_id>=rifts.size():return
 var entry:Dictionary=rifts[site_id]
 if bool(entry["cleared"]):_show_message("This rift has already been sealed.",2);return
 if active_rift>=0:_show_message("Finish the current rift first.",2);return
 if mounted or _combat_threat():_show_message("Dismount and leave combat to open a rift.",2);return
 if player.global_position.distance_to(entry["anchor"])>5.0:return
 active_rift=site_id
 entry["active"]=true
 entry["stage"]=1
 _activate_rift_wave(site_id)

func _activate_rift_wave(site_id:int) -> void:
 var entry:Dictionary=rifts[site_id]
 var stage:int=int(entry["stage"])
 var slots:Array=entry["slots"]
 for slot in slots:
  var creature:Dictionary=beasts[int(slot)]
  _reset_native_beast(creature)
  var health:=float(creature["base_hp"])*(1.0+float(stage-1)*0.6)
  creature["max"]=health
  creature["hp"]=health
  creature["dungeon_wave"]=stage
  creature["alert_until"]=map_time+90.0
 _set_rift_label(entry)
 _show_message(str(entry["name"])+" • WAVE "+str(stage)+" / 3",3.0)

func _on_rift_enemy_defeated(beast:Dictionary) -> void:
 var site_id:int=int(beast.get("dungeon_site",-1))
 if site_id<0 or site_id!=active_rift:return
 var entry:Dictionary=rifts[site_id]
 for slot in entry["slots"]:
  if bool(beasts[int(slot)].get("alive",false)):return
 if int(entry["stage"])<RIFT_WAVES:
  entry["stage"]=int(entry["stage"])+1
  _activate_rift_wave(site_id)
  return
 entry["active"]=false
 entry["cleared"]=true
 active_rift=-1
 if not (site_id in rifts_cleared):rifts_cleared.append(site_id)
 rifts_completed+=1
 var zone:int=int(entry["zone"])
 gold+=125+zone*30
 _gain_material("Boss Seal",2)
 _award_experience(zone,"elder",true)
 var loot:Dictionary={"name":str(entry["name"])+" Relic","rarity":"Legendary","power":40+zone*12,"affix":AFFIXES[site_id%AFFIXES.size()],"affix_value":0.09+0.02*float(site_id)}
 _create_gear_drop(entry["anchor"],"Rift Relic","dusk",zone,loot)
 _set_rift_label(entry)
 _show_message("RIFT SEALED! Legendary relic, 2 Boss Seals and gold.",4.0)

func _refresh_boss_phase(beast:Dictionary) -> void:
 if not bool(beast.get("boss",false)) or not bool(beast.get("alive",false)):return
 var ratio:=float(beast["hp"])/maxf(1.0,float(beast["max"]))
 var phase:=3 if ratio<=0.32 else 2 if ratio<=0.67 else 1
 if phase<=int(beast.get("boss_phase",1)):return
 beast["boss_phase"]=phase
 beast["boss_pattern"]=0
 beast["windup"]=0.0
 _set_boss_telegraph(beast,false)
 var text_label:Label3D=beast["nameplate"]
 if is_instance_valid(text_label):
  text_label.text="ECLIPSE GUARDIAN • PHASE "+str(phase)
  text_label.modulate=Color("ff648b") if phase==3 else Color("ffb46c")
 bosses_transformed+=1
 _show_message("GUARDIAN TRANSFORMED! PHASE "+str(phase),2.8)

func _perk_rank(id:String) -> int:
 return clampi(int(hero_perks.get(id,0)),0,3)

func _purchase_hero_perk(index:int) -> void:
 if index<0 or index>=HERO_PERKS.size():return
 var perk:Dictionary=HERO_PERKS[index]
 var id:=str(perk["id"])
 var rank:=_perk_rank(id)
 if hero_points<1 or rank>=3:_show_message("Not enough hero points or perk already maxed.",2);return
 if index%3>0 and _perk_rank(str(HERO_PERKS[index-1]["id"]))<1:
  _show_message("Unlock the previous talent in this tree first.",2);return
 hero_points-=1
 hero_perks[id]=rank+1
 if id=="vigor":
  max_hp+=12.0
  hp=minf(max_hp,hp+12.0)
 _show_message(str(perk["name"])+" rank "+str(rank+1)+" / 3",2.0)
 _save_game()

func _gear_affix_value(name:String) -> float:
 if str(equipped.get("affix",""))==name:
  return clampf(float(equipped.get("affix_value",0.0)),0.0,0.35)
 return 0.0

func _day_phase() -> String:
 return WORLD_CYCLE.phase(world_clock,DAY_LENGTH_SECONDS)

func _advance_living_world(delta:float) -> void:
 var state:Dictionary=WORLD_CYCLE.advance(world_clock,world_day,weather_name,weather_remaining,weather_changes,living_visual_tick,delta,rng,DAY_LENGTH_SECONDS,WEATHER_INTERVAL_SECONDS,WEATHER_TYPES)
 world_clock=float(state["clock"])
 world_day=int(state["day"])
 weather_name=str(state["weather"])
 weather_remaining=float(state["remaining"])
 weather_changes=int(state["changes"])
 living_visual_tick=float(state["visual_tick"])
 if bool(state["weather_changed"]):
  _show_message("The weather shifts: " + weather_name + ".",2.4)
 if bool(state["refresh_lighting"]):_refresh_world_lighting()

func _refresh_world_lighting() -> void:
 if living_environment == null:return
 WORLD_CYCLE.apply_lighting(living_environment,get_node_or_null("TwinMoonlight") as DirectionalLight3D,world_clock,DAY_LENGTH_SECONDS,weather_name)

func _build_world_events() -> void:
 for definition in WORLD_EVENT_SITES:
  var zone:int = int(definition["zone"])
  var middle:Vector2 = REGIONS[zone]["at"]
  # Markers sit on the established east-west road where random scenery is excluded.
  var at := Vector3(middle.x + 16.0,0.0,middle.y)
  var marker := _add_sphere(at + Vector3(0,0.48,0),0.43,Color("65c5c0") if str(definition["kind"]) == "cache" else Color("eb9960"),false,true)
  marker.name = "WorldEvent_" + str(zone)
  var label := Label3D.new()
  label.font_size = 23
  label.pixel_size = 0.006
  label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
  label.position = Vector3(0,1.3,0)
  marker.add_child(label)
  var event:Dictionary = {"zone":zone,"name":str(definition["name"]),"kind":str(definition["kind"]),"material":str(definition["material"]),"anchor":at,"marker":marker,"label":label,"completed":false,"active":false}
  world_events.append(event)
  _refresh_event_label(event)

func _refresh_event_label(event:Dictionary) -> void:
 var label:Label3D = event["label"]
 if not is_instance_valid(label):return
 if bool(event["completed"]):label.text = "✓ " + str(event["name"]) + " COMPLETE"
 elif bool(event["active"]):label.text = str(event["name"]) + " (" + str(active_event_kills) + "/" + str(WORLD_EVENT_GOAL) + ")"
 else:label.text = str(event["name"]) + "\n(G) Interact"

func _begin_world_event(zone:int) -> void:
 if zone < 0 or zone >= world_events.size():return
 var event:Dictionary = world_events[zone]
 if bool(event["completed"]):_show_message("This regional event is already complete.",2.0);return
 if mounted or _combat_threat():_show_message("Dismount and leave combat to start the event.",2.0);return
 if player.global_position.distance_to(event["anchor"]) > 4.6:return
 if str(event["kind"]) == "cache":
  _complete_world_event(zone)
  return
 if active_world_event >= 0 and active_world_event != zone:
  _show_message("Finish your active hunt in another region first.",2.0)
  return
 if active_world_event == zone:_show_message("Hunt active: " + str(active_event_kills) + "/3 hostiles.",2.0);return
 active_world_event = zone
 active_event_kills = 0
 event["active"] = true
 _refresh_event_label(event)
 _show_message(str(event["name"]) + " started. Defeat three hostile beasts in this region.",3.0)
 _save_game()

func _complete_world_event(zone:int, defer_save:bool=false) -> void:
 if zone < 0 or zone >= world_events.size():return
 var event:Dictionary = world_events[zone]
 if bool(event["completed"]):return
 event["completed"] = true
 event["active"] = false
 if not (zone in events_completed):events_completed.append(zone)
 if active_world_event == zone:active_world_event = -1
 active_event_kills = 0
 gold += 45 + zone * 22
 _gain_material(str(event["material"]),2)
 if str(event["kind"]) == "hunt":
  var prize:Dictionary={"name":str(event["name"]) + " Trophy","rarity":"Epic","power":20+zone*8}
  _create_gear_drop(event["anchor"],"World Event","moon",zone,prize)
 _refresh_event_label(event)
 _show_message("WORLD EVENT COMPLETED: gold and materials earned!",3.0)
 # Checkpoint save preserves event state and prevents repeated claim rewards.
 if not defer_save:
  auto_save_cooldown = 8.0
  _save_game()

func _world_event_on_defeat(beast:Dictionary) -> bool:
 if active_world_event < 0 or active_world_event >= world_events.size():return false
 if int(beast.get("zone",-1)) != active_world_event:return false
 if not bool(beast.get("hostile",false)) or int(beast.get("dungeon_site",-1)) >= 0:return false
 active_event_kills = mini(WORLD_EVENT_GOAL,active_event_kills + 1)
 if active_event_kills >= WORLD_EVENT_GOAL:
  # Save at the end of the defeat transaction so corpse and loot also persist.
  _complete_world_event(active_world_event,true)
  return true
 _refresh_event_label(world_events[active_world_event])
 _show_message("World hunt: " + str(active_event_kills) + "/3",1.5)
 return false

func _region_here() -> int:
 var best:=0
 var distance:=INF
 for i in range(REGIONS.size()):
  var p:Vector2=REGIONS[i]["at"]
  var d:=Vector2(player.global_position.x-p.x,player.global_position.z-p.y).length_squared()
  if d<distance:
   distance=d
   best=i
 return best

func _build_ui() -> void:
 var ui:=CanvasLayer.new()
 ui.name="NativeAtlasUI"
 add_child(ui)
 var root:=Control.new()
 root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
 root.mouse_filter=Control.MOUSE_FILTER_IGNORE
 ui.add_child(root)
 hud=_ui_label(root,Vector2(18,12),20,Color("f5e3bd"))
 hint=_ui_label(root,Vector2(18,43),15,Color("c6dccf"))
 status=_ui_label(root,Vector2(18,75),17,Color("ffd391"))
 world_hud=_ui_label(root,Vector2(18,111),14,Color("bcabec"))
 var controls:=_ui_label(root,Vector2(18,0),15,Color("d4d7e7"))
 controls.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_LEFT)
 controls.position=Vector2(18,-68)
 controls.text="WASD move • Mouse attack • Space dodge • Q/E/R spells • G interact • T tame • M mount • H pet stance • Tab map • F10/Esc atlas"
 quick_actions=PanelContainer.new()
 quick_actions.name="QuickActions"
 quick_actions.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_RIGHT)
 quick_actions.position=Vector2(-320,-288)
 quick_actions.custom_minimum_size=Vector2(304,210)
 quick_actions.visible=false
 root.add_child(quick_actions)
 var quick_column:=VBoxContainer.new()
 quick_column.add_theme_constant_override("separation",4)
 quick_actions.add_child(quick_column)
 var quick_header:=Label.new()
 quick_header.text="QUICK ACTIONS"
 quick_header.add_theme_font_size_override("font_size",17)
 quick_column.add_child(quick_header)
 for entry in [["INTERACT / LOOT","interact"],["TAME","tame"],["PET STANCE","stance"],["DODGE","dodge"],["SAVE CHECKPOINT","save"]]:
  var button:=Button.new()
  button.text=str(entry[0])
  button.custom_minimum_size=Vector2(280,30)
  button.pressed.connect(_quick_action.bind(str(entry[1])))
  quick_column.add_child(button)
 minimap=Control.new()
 minimap.set_script(MINI_MAP_SCRIPT)
 minimap.set("game",self)
 minimap.name="LiveCircularMinimap"
 minimap.anchor_left=1.0
 minimap.anchor_right=1.0
 minimap.offset_left=-264.0
 minimap.offset_right=-12.0
 minimap.offset_top=10.0
 minimap.offset_bottom=274.0
 minimap.mouse_filter=Control.MOUSE_FILTER_IGNORE
 minimap.visible=false
 root.add_child(minimap)
 start_screen=_make_menu(root,"MOONFALL NATIVE v13 • SIX REGIONS")
 var choices:=HBoxContainer.new()
 choices.position=Vector2(58,108)
 choices.add_theme_constant_override("separation",22)
 start_screen.add_child(choices)
 for entry in [{"id":"z","name":"Z • Void Reaper","art":Z_ART},{"id":"nyra","name":"Nyra • Eclipse Queen","art":NYRA_ART}]:
  var column:=VBoxContainer.new()
  column.custom_minimum_size=Vector2(335,355)
  choices.add_child(column)
  var portrait:=TextureRect.new()
  portrait.texture=entry["art"]
  portrait.custom_minimum_size=Vector2(310,260)
  portrait.expand_mode=TextureRect.EXPAND_IGNORE_SIZE
  portrait.stretch_mode=TextureRect.STRETCH_KEEP_ASPECT_CENTERED
  column.add_child(portrait)
  var launch:=Button.new()
  launch.text="NEW GAME AS "+str(entry["name"])
  launch.custom_minimum_size=Vector2(310,56)
  launch.pressed.connect(_start_game.bind(str(entry["id"])))
  column.add_child(launch)
  if not is_instance_valid(hero_start_button):hero_start_button=launch
 if not _read_save().is_empty():
  var continue_btn:=Button.new()
  continue_btn.position=Vector2(145,530)
  continue_btn.size=Vector2(460,48)
  continue_btn.text="CONTINUE NATIVE SIX-REGION CHECKPOINT"
  continue_btn.pressed.connect(_load_from_main_menu)
  start_screen.add_child(continue_btn)
 menu_panel=_make_menu(root,"RAVEN ATLAS • NATIVE")
 menu_panel.visible=false
 pause_screen=menu_panel
 var frame:=PanelContainer.new()
 frame.position=Vector2(48,115)
 frame.size=Vector2(1090,550)
 menu_panel.add_child(frame)
 var layout:=VBoxContainer.new()
 layout.add_theme_constant_override("separation",8)
 frame.add_child(layout)
 var tabs:=HBoxContainer.new()
 tabs.add_theme_constant_override("separation",7)
 layout.add_child(tabs)
 for tab in ["Character","World","Wallet","Inventory","Materials","Hero Skills","Tamer","Professions","Hunter","Bestiary","System"]:
  var tab_button:=Button.new()
  tab_button.text=tab
  tab_button.custom_minimum_size=Vector2(86,43)
  tab_button.add_theme_font_size_override("font_size",12)
  tab_button.pressed.connect(_menu_tab.bind(tab))
  tabs.add_child(tab_button)
  if not is_instance_valid(atlas_first_tab):atlas_first_tab=tab_button
 var contents:=HBoxContainer.new()
 contents.add_theme_constant_override("separation",14)
 layout.add_child(contents)
 var guide_column:=VBoxContainer.new()
 guide_column.custom_minimum_size=Vector2(300,450)
 contents.add_child(guide_column)
 beast_guide=TextureRect.new()
 beast_guide.custom_minimum_size=Vector2(295,200)
 beast_guide.expand_mode=TextureRect.EXPAND_IGNORE_SIZE
 beast_guide.stretch_mode=TextureRect.STRETCH_KEEP_ASPECT_CENTERED
 guide_column.add_child(beast_guide)
 inventory_search_input=LineEdit.new()
 inventory_search_input.placeholder_text="Search equipment or materials..."
 inventory_search_input.custom_minimum_size=Vector2(295,34)
 inventory_search_input.max_length=80
 inventory_search_input.visible=false
 inventory_search_input.text_changed.connect(_on_inventory_query_changed)
 guide_column.add_child(inventory_search_input)
 var action_scroll:=ScrollContainer.new()
 action_scroll.custom_minimum_size=Vector2(295,175)
 guide_column.add_child(action_scroll)
 menu_buttons=VBoxContainer.new()
 menu_buttons.add_theme_constant_override("separation",6)
 action_scroll.add_child(menu_buttons)
 var scroller:=ScrollContainer.new()
 scroller.custom_minimum_size=Vector2(735,452)
 scroller.size_flags_horizontal=Control.SIZE_EXPAND_FILL
 contents.add_child(scroller)
 menu_body=RichTextLabel.new()
 menu_body.bbcode_enabled=true
 menu_body.fit_content=true
 menu_body.scroll_active=true
 menu_body.size_flags_horizontal=Control.SIZE_EXPAND_FILL
 menu_body.add_theme_font_size_override("normal_font_size",17)
 scroller.add_child(menu_body)
 _menu_tab("Character")
 if is_instance_valid(hero_start_button):hero_start_button.call_deferred("grab_focus")
 _update_hud()

func _on_inventory_query_changed(query:String)->void:
 inventory_query=query.substr(0,80)
 inventory_page=0
 if current_tab in ["Inventory","Materials"]:_menu_tab(current_tab)

func _inventory_action(action:String)->void:
 match action:
  "inventory_toggle":
   inventory_show_vault=not inventory_show_vault
   inventory_page=0
  "inventory_filter":
   var filters: Array[String]=["All","Common","Uncommon","Rare","Epic","Legendary","Mythic","Crafted"]
   inventory_filter=filters[(filters.find(inventory_filter)+1)%filters.size()]
   inventory_page=0
  "inventory_sort":
   var sorts:Array[String]=["Default","Power","Rarity","Name"]
   inventory_sort=sorts[(sorts.find(inventory_sort)+1)%sorts.size()]
   inventory_page=0
  "inventory_page_next":inventory_page+=1
  "inventory_page_prev":inventory_page=maxi(0,inventory_page-1)
  "inventory_upgrade":
   var cost:int=INVENTORY_RULES.upgrade_cost(bag_capacity)
   if cost<=0 or not _wallet_can_afford(cost*MONEY_RULES.COPPER_PER_GOLD):
    _show_message("Not enough gold or bag already full-sized.",2)
    return
   _spend_coins(cost*MONEY_RULES.COPPER_PER_GOLD)
   bag_capacity=mini(INVENTORY_RULES.BAG_MAX_CAPACITY,bag_capacity+INVENTORY_RULES.BAG_UPGRADE_STEP)
   _show_message("Bag expanded to "+str(bag_capacity)+" slots!",2)
   _save_game()
  _:
   if action.begins_with("inventory_deposit:"):
    var idx:int=int(action.get_slice(":",1))
    if INVENTORY_RULES.transfer_gear(gear_items,vault_items,idx,INVENTORY_RULES.VAULT_LIMIT):
     _save_game()
    else:_show_message("Vault full or item unavailable.",2)
   elif action.begins_with("inventory_withdraw:"):
    var idx:int=int(action.get_slice(":",1))
    if INVENTORY_RULES.transfer_gear(vault_items,gear_items,idx,bag_capacity):
     _save_game()
    else:_show_message("Bag full or item unavailable.",2)

func _quick_action(action:String) -> void:
 if not hero_playing or menu_open:return
 match action:
  "interact":_interact()
  "tame":_tame()
  "senses":_toggle_senses()
  "dodge":_dodge()
  "save":_save_game()
  "stance":_cycle_companion_stance()

func _menu_tab(which: String) -> void:
 current_tab=which
 if not is_instance_valid(menu_body):return
 if is_instance_valid(inventory_search_input):
  inventory_search_input.visible=which in ["Inventory","Materials"]
 for child in menu_buttons.get_children():child.queue_free()
 beast_guide.texture=null
 match which:
  "Character":
   beast_guide.texture=NYRA_ART if hero=="nyra" else Z_ART
   menu_body.text="[b]"+("NYRA" if hero=="nyra" else "Z")+" • NATIVE ADVENTURER[/b]\n\nHealth: "+str(int(hp))+" / "+str(int(max_hp))+"\nLevel: "+str(hero_level)+"  • XP: "+str(hero_xp)+"/"+str(hero_xp_to_next)+"\nDamage multiplier: "+str(snappedf(_hero_damage_multiplier(),0.01))+"x\nCoins: "+_wallet_text()+"\nEquipment power: "+str(_equipment_power())+"\nBattle power: "+str(_current_power())+" (region recommended: "+str(_recommended_power(region_index))+")\nBeasts defeated: "+str(kills_total)+"\nDiscovered species: "+str(discovered_species.size())+"\nUnlocked shrines: "+str(shrine_discovery.size())+"/6\n\n[b]No mounted combat.[/b] Riding is for travel only, with auto-dismount after 20s of sustained aggro."
   _atlas_action("OPEN WALLET & MERCHANT","wallet_open")
   _atlas_action("HEAL AT SHRINE (IF NEARBY)","shrine")
  "Wallet":
   var text:="[b]MOONFALL TREASURY[/b]\n\n"+_wallet_text()+"\n\n[b]Exchange[/b]\n100 copper = 1 silver • 100 silver = 1 gold.\nCoins do not occupy inventory slots. Monster coin bundles are collected automatically when approached.\n\n[b]Hearthstone Outpost merchant[/b]\n"
   if _near_outpost_merchant():
    text+="Merchant available. You may buy healing supplies and sell unwanted equipment.\n"
    _atlas_action("BUY: Healing Draught (2g 50s)","wallet_buy_heal")
    _atlas_action("BUY: Hunter's Oil (4g)","wallet_buy_oil")
    text+="\n[b]Sell equipment[/b]\n"
    for i in range(mini(gear_items.size(),20)):
     var item:Dictionary=gear_items[i]
     var price:int=MONEY_RULES.sell_value(item)
     text+=str(i+1)+". "+str(item.get("name","Gear"))+" • "+MONEY_RULES.format_total(price)+"\n"
     _atlas_action("SELL: "+str(item.get("name","Gear")),"wallet_sell:"+str(i),"Earn "+MONEY_RULES.format_total(price))
    if gear_items.size()>20:text+="Only the first 20 bag items are shown. Use your vault to organize more.\n"
   else:text+="Travel to Hearthstone Outpost's shrine and leave combat to trade.\n"
   menu_body.text=text
  "Hero Skills":
   var text:="[b]HERO SPECIALIZATIONS[/b]   Unspent points: "+str(hero_points)+"\nAssault, Survival and Arcana • 3 ranks per talent.\nEvery level grants 1 Hero point and 1 Tamer point.\n\n"
   for i in range(HERO_PERKS.size()):
    var perk:Dictionary=HERO_PERKS[i]
    var rank:=_perk_rank(str(perk["id"]))
    text+="[color="+_branch_hex(str(perk["branch"]))+"]"+str(perk["branch"])+"[/color] • "+str(perk["name"])+" "+str(rank)+"/3 • "+str(perk["tip"])+"\n"
    if rank<3:_atlas_action("LEARN: "+str(perk["name"]),"hero_perk:"+str(i),str(perk["tip"]))
   menu_body.text=text
  "World":
   var text:="[b]SIX CONNECTED 3D REGIONS[/b]\n"
   for i in range(REGIONS.size()):text+="\n"+str(i+1)+". "+str(REGIONS[i]["name"])+("  ✓ SHRINE LIT" if i in shrine_discovery else "  ○ UNKNOWN SHRINE")
   text+="\n\n[b]WORLD RIFTS:[/b] "+str(rifts_cleared.size())+" / "+str(rifts.size())+" sealed. Activate a rift at its glowing marker with G; clear three escalating waves in the open world."
   for entry in rifts:text+="\n • "+str(entry["name"])+"  "+("SEALED" if bool(entry["cleared"]) else "WAVE "+str(entry["stage"]) if bool(entry["active"]) else "AVAILABLE")
   text+="\n\n[b]LIVING WORLD[/b] • Day " + str(world_day) + " / " + _day_phase() + " • Weather " + weather_name + "."
   text+="\n[b]Regional events:[/b] " + str(events_completed.size()) + "/6 completed."
   for event in world_events:
    text+="\n • " + str(event["name"]) + ": " + ("DONE" if bool(event["completed"]) else str(active_event_kills) + "/3 KILLS" if bool(event["active"]) else "AVAILABLE")
   text+="\nGo to a world-event marker and press G. Hunts require three hostile kills in that region."
   text+="\n\nYour power: "+str(_current_power())+". Recommended region power: "+str(_recommended_power(region_index))+".\nThe power difference gently adjusts damage; no region is locked."
   text+="\n\nTravel along the ancient roads, or use an awakened shrine to fast travel while out of combat."
   menu_body.text=text
   for i in range(REGIONS.size()):
    if i in shrine_discovery:_atlas_action("TRAVEL: "+str(REGIONS[i]["name"]),"travel:"+str(i))
  "Inventory":
   var source:Array= vault_items if inventory_show_vault else gear_items
   var capacity:int=INVENTORY_RULES.VAULT_LIMIT if inventory_show_vault else bag_capacity
   var text:="[b]"+("PERSONAL VAULT" if inventory_show_vault else "EQUIPMENT BAG")+"[/b]  "+str(source.size())+" / "+str(capacity)
   text+="\nMaterials use separate storage. Search, filter, or sort gear below.\n"
   text+="Filter: "+inventory_filter+"  • Sort: "+inventory_sort+"\n"
   _atlas_action("OPEN "+("BAG" if inventory_show_vault else "VAULT (1000)"),"inventory_toggle")
   _atlas_action("FILTER: "+inventory_filter,"inventory_filter")
   _atlas_action("SORT: "+inventory_sort,"inventory_sort")
   if not inventory_show_vault and bag_capacity<INVENTORY_RULES.BAG_MAX_CAPACITY:
    var upgrade_cost:int=INVENTORY_RULES.upgrade_cost(bag_capacity)
    _atlas_action("EXPAND BAG +50 SLOTS ("+MONEY_RULES.format_total(upgrade_cost*MONEY_RULES.COPPER_PER_GOLD)+")","inventory_upgrade")
    text+="\nNext bag expansion: "+MONEY_RULES.format_total(upgrade_cost*MONEY_RULES.COPPER_PER_GOLD)+"."
   var matches:Array[int]=INVENTORY_RULES.matching_gear_indices(source,inventory_query,inventory_filter,inventory_sort)
   var pages:int=INVENTORY_RULES.page_count(matches.size())
   inventory_page=clampi(inventory_page,0,pages-1)
   text+="\nFound "+str(matches.size())+" • Page "+str(inventory_page+1)+" / "+str(pages)+"\n\n"
   for n in range(inventory_page*INVENTORY_RULES.BAG_PAGE_SIZE,mini((inventory_page+1)*INVENTORY_RULES.BAG_PAGE_SIZE,matches.size())):
    var index:int=matches[n]
    var gear:Dictionary=source[index]
    text+=str(index+1)+". "+str(gear.get("rarity","Common"))+" "+str(gear.get("name","Gear"))+" • Power "+str(gear.get("power",0))+"\n"
    if inventory_show_vault:
     _atlas_action("WITHDRAW: "+str(gear.get("name","Gear")),"inventory_withdraw:"+str(index))
    else:
     _atlas_action("EQUIP: "+str(gear.get("name","Gear")),"equip:"+str(index))
     _atlas_action("DEPOSIT: "+str(gear.get("name","Gear")),"inventory_deposit:"+str(index))
   if matches.is_empty():text+="No matching equipment. Defeat enemies or change your search.\n"
   if inventory_page>0:_atlas_action("PREVIOUS PAGE","inventory_page_prev")
   if inventory_page+1<pages:_atlas_action("NEXT PAGE","inventory_page_next")
   text+="\nEquipped: "+str(equipped.get("name","None"))+" • Power "+str(_equipment_power())
   menu_body.text=text
  "Materials":
   var text:="[b]DEDICATED CRAFTING STORAGE[/b]\nMaterials never fill your gear bag. Maximum 999 per displayed stack.\n\n"
   var keys:Array=materials.keys()
   keys.sort()
   var found:int=0
   for key in keys:
    if inventory_query.strip_edges()!="" and not inventory_query.to_lower() in str(key).to_lower():continue
    var amount:int=int(materials[key])
    if amount<=0:continue
    var stacks:Array[int]=INVENTORY_RULES.material_stack_counts(amount)
    text+=str(key)+": "+str(amount)+" ("+str(stacks.size())+" stacks)\n"
    found+=1
   if found==0:text+="No matching materials.\n"
   text+="\nCraft items in the Professions tab."
   menu_body.text=text
  "Tamer":
   var text:="[b]TAMER CONSTELLATION[/b]   Points: "+str(tamer_points)+"\n\n18 perks, each with 3 ranks. Invest in Wild Bond, Hunt, or Warden.\n\n"
   for i in range(TALENTS.size()):
    var perk:Dictionary=TALENTS[i]
    var current:=int(learned_talents.get(str(perk["id"]),0))
    text+="[color="+_branch_hex(str(perk["branch"]))+"]"+str(perk["name"])+"[/color]  "+str(current)+"/3  • "+str(perk["tip"])+"\n"
    if current<3:_atlas_action("LEARN: "+str(perk["name"]),"talent:"+str(i),str(perk["tip"]))
   text+="\nCompanion stance: "+companion_stance+" (Guard attacks nearby enemies, Follow attacks only engaged threats, Passive avoids combat)."
   _atlas_action("CYCLE COMPANION STANCE","stance","Guard -> Follow -> Passive; H key or gamepad D-pad Up")
   text+="\nCompanion: "+(str(companion.get("kind","None")).capitalize() if not companion.is_empty() else "None")+"\nSpecies learned: "+str(discovered_species.size())
   if not companion.is_empty():text+="\nCompanion health: "+str(int(companion.get("hp",0)))+" / "+str(int(companion.get("max",80)))
   menu_body.text=text
  "Professions":
   var text:="[b]PROFESSIONS / CRAFTING[/b]\nSkinning "+str(skinning_skill)+" / 300\nLeatherworking "+str(crafting_skill)+" / 300\n\n[b]Materials:[/b]\n"
   for k in materials.keys():text+=str(k)+": "+str(materials[k])+"\n"
   text+="\n[b]Recipes[/b]\n"
   for i in range(RECIPES.size()):
    var recipe:Dictionary=RECIPES[i]
    text+=str(i+1)+". "+str(recipe["name"])+" • "+str(recipe["cost"])+"\n"
    _atlas_action("CRAFT: "+str(recipe["name"]),"craft:"+str(i))
   menu_body.text=text
  "Hunter":
   var text:="[b]HUNTER'S PATH[/b]\n\nHunter Senses: "+("ACTIVE" if senses_on else "INACTIVE")+"\nContracts completed: "+str(contracts_done.size())+"/6\nHunter Oil: "+str(int(hunter_oil_time))+"s\n"
   if contract_zone>=0:text+="\nACTIVE CONTRACT: "+str(REGIONS[contract_zone]["name"])+" · "+str(contract_kills)+" / 3 hostile beasts.\n"
   else:text+="\nAccept one regional monster contract; hunt three hostile beasts in that region.\n"
   menu_body.text=text
   _atlas_action("TOGGLE HUNTER SENSES","senses")
   for i in range(REGIONS.size()):
    if not (i in contracts_done):_atlas_action("ACCEPT: "+str(REGIONS[i]["name"]),"contract:"+str(i))
   _atlas_action("USE HUNTER'S OIL","oil")
   _atlas_action("USE HEALING DRAUGHT","heal")
  "Bestiary":
   var text:="[b]MOONFALL BESTIARY[/b]\nEight native beast families. Normal, Moon-Touched, Duskborn, and Elder variants may appear.\n\n"
   for k in BEASTS.keys():
    var item:Dictionary=BEASTS[k]
    text+=str(k).capitalize()+"  • "+("hostile" if item["hostile"] else "passive")+" • Skin: "+str(item["skin"])+" • Trophy: "+str(item["drop"])+"\n"
   text+="\n[b]Rare Beast Paintings[/b] \nConcept sheet details appear in the portrait viewer; creatures use simple native 3D placeholders in-world."
   menu_body.text=text
   for k in PORTRAITS.keys():_atlas_action(str(k).replace("_"," ").capitalize(),"portrait:"+str(k))
  "System":
   menu_body.text="[b]MOONFALL NATIVE v13 OPTIONS[/b]\nGodot 4.7.2 native Windows build uses Direct3D 12 where supported, with rendering fallbacks.\nNative Godot project: no browser or WebGL dependency.\n\nSimple controls:\nWASD / arrows move, left click attack, Space dodge, Q/E/R spells, G loot/events/shrine, T tame, M mount, F senses, H pet stance, Tab map, F10 or Esc menu.\nController: left stick move, X attack, A dodge, B interact, Y tame, shoulders spells, D-pad up pet stance, D-pad down minimap, Start menu.\n\nNative save schema 2 is separate from browser v104."
   _atlas_action("SAVE NATIVE CHECKPOINT","save")
   _atlas_action("LOAD NATIVE CHECKPOINT","load")
   _atlas_action("SHOW WORLD DIAGNOSTICS","diagnostics")
   _atlas_action("TOGGLE CIRCULAR MINIMAP","minimap")
   _atlas_action("CYCLE PET STANCE","stance")
   _atlas_action("SMOOTH GRAPHICS (LOW POWER)","smooth")
   _atlas_action("FULL GRAPHICS","full")
   _atlas_action("RESUME GAME","resume")
   _atlas_action("RETURN TO HERO SELECT","title")

func _atlas_action(label: String, action: String, tip: String="") -> void:
 var button:=Button.new()
 button.text=label
 if tip!="":button.tooltip_text=tip
 button.custom_minimum_size=Vector2(270,32)
 button.pressed.connect(_atlas_execute.bind(action))
 menu_buttons.add_child(button)

func _branch_hex(branch: String) -> String:
 if branch=="Bond":return "#d783ee"
 if branch=="Hunt":return "#f0a558"
 return "#7fcce9"

func _atlas_execute(action: String) -> void:
 if action.begins_with("inventory_"):
  _inventory_action(action)
  _menu_tab("Inventory")
  return
 if action=="smooth" or action=="full":
  _apply_graphics_quality("Smooth" if action=="smooth" else "Full")
  _menu_tab("System")
  return
 if action=="minimap":
  _toggle_minimap()
  _menu_tab("System")
  return
 if action=="stance":
  _cycle_companion_stance()
  return
 if action=="diagnostics":
  var alive:=0
  for b in beasts:
   if bool(b.get("alive",false)):alive+=1
  menu_body.text="[b]WORLD HEALTH / NATIVE v13[/b]\n\nFPS: "+str(Engine.get_frames_per_second())+"\nAlive creatures: "+str(alive)+" / "+str(beasts.size())+"\nNatural hunts: "+str(hunts_completed)+"\nHerd alarms: "+str(herds_alerted)+"\nBlocked line-of-sight checks: "+str(blocked_hits)+"\nGear picked up: "+str(pickup_count)+"\nGround gear: "+str(gear_drops.size())+" / "+str(MAX_GROUND_DROPS)+"\nSkinnable carcasses: "+str(carcasses.size())+" / "+str(MAX_CARCASSES)+"\nWildlife respawns: "+str(respawns_total)+"\nRestored dormant beasts: "+str(restored_wildlife)+"\nActive scenery regions: "+str(visible_regions)+" / 6\nRegional AI updates: "+str(ai_updates)+"\nCheckpoint health: "+save_health+"\n\nThis is a prototype diagnostic panel, not a separate game mode."
  return
 if action.begins_with("wallet_"):
  _wallet_action(action)
  _menu_tab("Wallet")
  return
 if action.begins_with("travel:"):
  _travel_to(int(action.get_slice(":",1)))
 elif action.begins_with("hero_perk:"):
  _purchase_hero_perk(int(action.get_slice(":",1)))
 elif action.begins_with("talent:"):
  _buy_talent(int(action.get_slice(":",1)))
 elif action.begins_with("craft:"):
  _craft(int(action.get_slice(":",1)))
 elif action.begins_with("equip:"):
  _equip(int(action.get_slice(":",1)))
 elif action.begins_with("contract:"):
  _take_contract(int(action.get_slice(":",1)))
 elif action.begins_with("portrait:"):
  var path:=str(PORTRAITS.get(action.get_slice(":",1),""))
  if path!="":beast_guide.texture=load(path)
  return
 else:
  match action:
   "shrine":_interact()
   "senses":_toggle_senses()
   "oil":_use_consumable("Hunter's Oil")
   "heal":_use_consumable("Healing Draught")
   "save":_save_game()
   "load":_load_game()
   "resume":_toggle_menu();return
   "title":hero_playing=false;menu_open=false;menu_panel.visible=false;start_screen.visible=true;quick_actions.visible=false;minimap.visible=false;hero_start_button.call_deferred("grab_focus");return
 _menu_tab(current_tab)

func _reset_new_adventure() -> void:
 # The hero-select PLAY buttons start a fresh adventure, not the preceding
 # hero's inventory and slain wildlife. CONTINUE calls _restore_data afterward.
 if not is_instance_valid(player):return
 player.position=HERO_START
 player.velocity=Vector3.ZERO
 hp=max_hp
 gold=0
 silver=0
 copper=0
 _clear_coin_drops()
 kills=0
 kills_total=0
 hero_level=1
 max_hp=100.0
 hp=max_hp
 hero_xp=0
 hero_xp_to_next=75
 hero_points=0
 hero_perks={}
 combat_combo=0
 combat_combo_until=0.0
 world_challenge=0
 companion_stance="Guard"
 skinning_skill=1
 crafting_skill=1
 water_professions={"Fishing":1,"Herbalism":1,"Mining":1,"Cooking":1,"Alchemy":1,"Smithing":1}
 water_site_days={}
 tamer_points=0
 materials={"herb":4}
 gear_items.clear()
 bag_capacity=INVENTORY_RULES.BAG_START_CAPACITY
 vault_items.clear()
 inventory_query=""
 inventory_page=0
 inventory_show_vault=false
 if is_instance_valid(inventory_search_input):inventory_search_input.text=""
 equipped={}
 learned_talents={}
 discovered_species.clear()
 companion={}
 if is_instance_valid(companion_node):companion_node.queue_free()
 companion_node=null
 contract_zone=-1
 contract_kills=0
 contracts_done.clear()
 shrine_discovery.clear()
 found_clues.clear()
 defeated_boss_ids.clear()
 mounted=false
 mount_combat_time=0.0
 attack_cooldown=0.0
 ability_cooldown=0.0
 dodge_time=0.0
 dodge_cooldown=0.0
 hunter_oil_time=0.0
 auto_save_cooldown=0.0
 skinning_target=null
 skinning_left=0.0
 senses_on=false
 map_time=0.0
 ecology_step=0.0
 respawn_refresh_tick=0.0
 region_refresh_tick=0.0
 pet_stunned_until=0.0
 current_threat=false
 herds_alerted=0
 hunts_completed=0
 pickup_count=0
 respawns_total=0
 restored_wildlife=0
 bosses_transformed=0
 rifts_completed=0
 world_clock=240.0
 world_day=1
 weather_name="Clear"
 weather_remaining=WEATHER_INTERVAL_SECONDS
 living_visual_tick=0.0
 weather_changes=0
 enemy_retreats=0
 events_completed.clear()
 active_world_event=-1
 active_event_kills=0
 for event in world_events:
  event["active"]=false
  event["completed"]=false
  _refresh_event_label(event)
 _refresh_world_lighting()
 rifts_cleared.clear()
 active_rift=-1
 for entry in rifts:
  entry["cleared"]=false
  entry["active"]=false
  entry["stage"]=0
  _set_rift_label(entry)
 for drop in gear_drops:
  var drop_node:Node3D=drop["node"]
  if is_instance_valid(drop_node):drop_node.queue_free()
 gear_drops.clear()
 for corpse in carcasses:
  var corpse_node:Node3D=corpse["node"]
  if is_instance_valid(corpse_node):corpse_node.queue_free()
 carcasses.clear()
 for beast in beasts:
  _reset_native_beast(beast)
  if int(beast.get("dungeon_site",-1))>=0:_despawn_native_beast(beast,0.0)
 for clue in clue_data:
  var clue_node:Node3D=clue["node"]
  if is_instance_valid(clue_node):clue_node.hide()
 region_index=0
 _update_region_visibility()
 _update_camera(1.0)

func _start_game(selected: String) -> void:
 _reset_new_adventure()
 hero="nyra" if selected=="nyra" else "z"
 hero_sprite.texture=NYRA_ART if hero=="nyra" else Z_ART
 hero_sprite.pixel_size=2.65/float(hero_sprite.texture.get_height())
 hero_playing=true
 menu_open=false
 region_index=_region_here()
 _update_region_visibility()
 start_screen.visible=false
 menu_panel.visible=false
 if is_instance_valid(quick_actions):quick_actions.visible=true
 if is_instance_valid(minimap):minimap.visible=minimap_visible
 _show_message("Explore all six regions as "+("Nyra" if hero=="nyra" else "Z")+".",4.0)
 _update_hud()

func _load_from_main_menu() -> void:
 var saved:=_read_save()
 if saved.is_empty():_show_message("No valid native save found.",3.0);return
 _start_game(str(saved["hero"]))
 _restore_data(saved)

func _toggle_menu() -> void:
 menu_open=not menu_open
 menu_panel.visible=menu_open
 if is_instance_valid(quick_actions):quick_actions.visible=not menu_open
 if menu_open:
  _menu_tab(current_tab)
  if is_instance_valid(atlas_first_tab):atlas_first_tab.call_deferred("grab_focus")
 _update_hud()

func _unhandled_key_input(event: InputEvent) -> void:
 if not (event is InputEventKey) or not event.pressed or event.echo:return
 if event.keycode in [KEY_ESCAPE,KEY_F10]:
  if hero_playing:_toggle_menu()
  get_viewport().set_input_as_handled()
  return
 if not hero_playing or menu_open:return
 match event.keycode:
  KEY_G:_interact()
  KEY_T:_tame()
  KEY_M:_toggle_mount()
  KEY_F:_toggle_senses()
  KEY_Q:_spell("nova")
  KEY_E:_spell("dash")
  KEY_R:_spell("fire")
  KEY_SPACE:_dodge()
  KEY_H:_cycle_companion_stance()
  KEY_TAB:_toggle_minimap()
 get_viewport().set_input_as_handled()

func _unhandled_input(event: InputEvent) -> void:
 if event is InputEventJoypadButton and event.pressed:
  if event.button_index==JOY_BUTTON_START:
   if hero_playing:_toggle_menu()
   get_viewport().set_input_as_handled()
   return
  if not hero_playing or menu_open:return
  match event.button_index:
   JOY_BUTTON_X:_attack()
   JOY_BUTTON_A:_dodge()
   JOY_BUTTON_B:_interact()
   JOY_BUTTON_Y:_tame()
   JOY_BUTTON_LEFT_SHOULDER:_spell("nova")
   JOY_BUTTON_RIGHT_SHOULDER:_spell("fire")
   JOY_BUTTON_DPAD_UP:_cycle_companion_stance()
   JOY_BUTTON_DPAD_DOWN:_toggle_minimap()
   JOY_BUTTON_DPAD_LEFT:_toggle_senses()
   JOY_BUTTON_DPAD_RIGHT:_toggle_mount()
  get_viewport().set_input_as_handled()
  return
 if not hero_playing or menu_open:return
 if event is InputEventMouseButton and event.button_index==MOUSE_BUTTON_LEFT and event.pressed:_attack()

func _physics_process(delta: float) -> void:
 if not hero_playing or menu_open:return
 attack_cooldown=maxf(0.0,attack_cooldown-delta)
 ability_cooldown=maxf(0.0,ability_cooldown-delta)
 dodge_time=maxf(0.0,dodge_time-delta)
 dodge_cooldown=maxf(0.0,dodge_cooldown-delta)
 hunter_oil_time=maxf(0.0,hunter_oil_time-delta)
 companion_attack_timer=maxf(0.0,companion_attack_timer-delta)
 auto_save_cooldown=maxf(0.0,auto_save_cooldown-delta)
 if status_seconds>0:
  status_seconds-=delta
  if status_seconds<=0:status_message=""
 if map_time > combat_combo_until:combat_combo=0
 var dir:=Vector3.ZERO
 if Input.is_key_pressed(KEY_W) or Input.is_key_pressed(KEY_UP):dir.z-=1.0
 if Input.is_key_pressed(KEY_S) or Input.is_key_pressed(KEY_DOWN):dir.z+=1.0
 if Input.is_key_pressed(KEY_A) or Input.is_key_pressed(KEY_LEFT):dir.x-=1.0
 if Input.is_key_pressed(KEY_D) or Input.is_key_pressed(KEY_RIGHT):dir.x+=1.0
 dir=_map_movement_to_camera((dir+_gamepad_direction()).limit_length(1.0))
 var pads: Array[int] = Input.get_connected_joypads()
 var sprinting: bool = Input.is_key_pressed(KEY_SHIFT) or (not pads.is_empty() and Input.is_joy_button_pressed(pads[0],JOY_BUTTON_LEFT_STICK))
 var speed:=SPRINT_SPEED if sprinting else WALK_SPEED
 if mounted:speed=8.4+float(learned_talents.get("endurance",0))*0.4
 if dodge_time>0:speed*=1.6
 if weather_name=="Rain" and not mounted:speed*=0.93
 player.velocity=dir*speed
 player.move_and_slide()
 player.position.x=clampf(player.position.x,-WORLD_X,WORLD_X)
 player.position.z=clampf(player.position.z,-WORLD_Z,WORLD_Z)
 if dir.length_squared()>0.01:hero_sprite.flip_h=dir.x<0
 region_index=_region_here()
 map_time+=delta
 _advance_living_world(delta)
 ecology_step+=delta
 region_refresh_tick+=delta
 if region_refresh_tick>=REGION_REFRESH_SECONDS:
  region_refresh_tick=0.0
  _update_region_visibility()
 respawn_refresh_tick+=delta
 if respawn_refresh_tick>=RESPAWN_REFRESH_SECONDS:
  respawn_refresh_tick=0.0
  _update_respawns()
 _update_beasts(delta)
 _update_coin_drops(delta)
 _update_companion(delta)
 _update_skinning(delta,dir)
 var threat:=_combat_threat()
 if mounted:
  if threat:
   mount_combat_time+=delta
   if mount_combat_time>=RIDE_COMBAT_SECONDS:
    mounted=false
    mount_combat_time=0
    _show_message("Your mount throws you off after 20 seconds of combat!",3.0)
  else:mount_combat_time=0.0
 else:mount_combat_time=0.0
 current_threat=threat
 ui_snapshot+=delta
 if ui_snapshot>=0.12:
  ui_snapshot=0.0
  _update_hud()

func _process(delta: float) -> void:
 if is_instance_valid(camera):_update_camera(delta)
 scenery_light_tick+=delta
 if scenery_light_tick<0.1:return
 scenery_light_tick=fmod(scenery_light_tick,0.1)
 for i in range(shrine_lights.size()):
  var light:OmniLight3D=shrine_lights[i]
  if is_instance_valid(light):light.light_energy=(2.7 if i in shrine_discovery else 0.8)+0.12*sin(Time.get_ticks_msec()*0.002+float(i))

func _update_camera(delta: float) -> void:
 if not is_instance_valid(player) or not is_instance_valid(camera):return
 var center:=player.global_position+Vector3(0,0.75,0)
 var target:=center+Vector3(16,21,16)
 camera.global_position=target if delta>=1 else camera.global_position.lerp(target,minf(delta*5,1))
 camera.look_at(center,Vector3.UP)

func _combat_threat() -> bool:
 if not is_instance_valid(player):return false
 for enemy in zone_beast_lists[region_index]:
  if not bool(enemy.get("alive",false)) or not bool(enemy.get("hostile",false)):continue
  var body:CharacterBody3D=enemy["body"]
  if not is_instance_valid(body):continue
  if body.global_position.distance_squared_to(player.global_position)>BEAST_THREAT_RANGE*BEAST_THREAT_RANGE:continue
  if _has_clear_sight(body,player):return true
 return false

func _has_clear_sight(a:CollisionObject3D,b:CollisionObject3D) -> bool:
 if not is_instance_valid(a) or not is_instance_valid(b):return false
 var start:=a.global_position+Vector3.UP*0.45
 var end:=b.global_position+Vector3.UP*0.45
 var params:=PhysicsRayQueryParameters3D.create(start,end)
 params.collision_mask=1
 params.exclude=[a.get_rid(),b.get_rid()]
 var hit:=get_world_3d().direct_space_state.intersect_ray(params)
 if not hit.is_empty():blocked_hits+=1
 return hit.is_empty()

func _nearest_prey(hunter:Dictionary,range_limit:float) -> Dictionary:
 var best:Dictionary={}
 var body:CharacterBody3D=hunter["body"]
 if not is_instance_valid(body):return best
 var closest_squared:float=maxf(0.0,range_limit)*maxf(0.0,range_limit)
 for prey in zone_beast_lists[int(hunter["zone"])]:
  if not bool(prey.get("alive",false)) or bool(prey.get("hostile",false)) or prey==hunter:continue
  if int(prey["zone"])!=int(hunter["zone"]):continue
  var target:CharacterBody3D=prey["body"]
  if not is_instance_valid(target):continue
  var d_squared:float=target.global_position.distance_squared_to(body.global_position)
  if d_squared<closest_squared and _has_clear_sight(body,target):best=prey;closest_squared=d_squared
 return best

func _ecology_update(delta:float) -> void:
 # Scan up to the small native population. Avoid spawning and new timers per beast.
 # Squared-distance thresholds avoid repeated square roots in herd/predator loops.
 if ecology_step<0.32:return
 ecology_step=0.0
 for creature in zone_beast_lists[region_index]:
  if not bool(creature.get("alive",false)):continue
  var body:CharacterBody3D=creature["body"]
  if not is_instance_valid(body):continue
  if bool(creature["hostile"]):
   if creature.get("boss",false):continue
   if float(creature.get("fed_until",0.0))>map_time:continue
   if body.global_position.distance_squared_to(player.global_position)<64.0:continue
   var prey:=_nearest_prey(creature,5.8)
   if prey.is_empty():continue
   creature["ecology_target"]=prey
   creature["hunting_until"]=map_time+2.5
   var peer:Dictionary={}
   for ally in zone_beast_lists[region_index]:
    if ally==creature or not bool(ally.get("alive",false)) or str(ally.get("kind",""))!=str(creature["kind"]):continue
    var ab:CharacterBody3D=ally["body"]
    if is_instance_valid(ab) and ab.global_position.distance_squared_to(body.global_position)<30.25:
     peer=ally
     break
   if not peer.is_empty():
    peer["ecology_target"]=prey
    peer["hunting_until"]=map_time+2.3
  else:
   # Proximity alarms spread to the same herbivore family within the current region.
   var scared:=false
   for predator in zone_beast_lists[region_index]:
    if not bool(predator.get("alive",false)) or not bool(predator.get("hostile",false)) or int(predator.get("zone",-1))!=region_index:continue
    var pb:CharacterBody3D=predator["body"]
    if is_instance_valid(pb) and pb.global_position.distance_squared_to(body.global_position)<20.25:scared=true;break
   if scared and map_time>=float(creature.get("scared_until",0.0)):
    creature["scared_until"]=map_time+3.2
    herds_alerted+=1
    for friend in zone_beast_lists[region_index]:
     if friend==creature or not bool(friend.get("alive",false)) or str(friend.get("kind",""))!=str(creature["kind"]):continue
     var fb:CharacterBody3D=friend["body"]
     if is_instance_valid(fb) and fb.global_position.distance_squared_to(body.global_position)<25.0:friend["scared_until"]=map_time+2.6

func _apply_incoming_damage(raw:float) -> void:
 var reduction:=0.0
 reduction+=0.006*float(_equipment_power())
 reduction+=0.04*float(_perk_rank("ward"))
 reduction+=_gear_affix_value("Aegis")
 if not companion.is_empty():
  reduction+=0.035*float(learned_talents.get("ironhide",0))
  reduction+=0.025*float(learned_talents.get("oath",0))
 # Power disparity matters without creating one-hit death spikes.
 var disparity:float = 1.0 / _power_modifier(region_index)
 var incoming:=raw*maxf(0.28,1.0-reduction)*clampf(disparity,0.80,1.38)
 # Guardian Call can intercept attacks, with its own bounded cooldown.
 if not companion.is_empty() and map_time>=pet_stunned_until and rng.randf()<0.09*float(learned_talents.get("guardian",0)):
  var pet_loss:=incoming*maxf(0.25,1.0-0.08*float(learned_talents.get("ironhide",0))-0.05*float(learned_talents.get("oath",0)))
  companion["hp"]=maxf(0.0,float(companion.get("hp",80.0))-pet_loss)
  if float(companion["hp"])<=0.0:
   pet_stunned_until=map_time+8.0
   companion["hp"]=maxf(1.0,float(companion.get("max",80.0))*0.2)
  _show_message("Your companion intercepted the hit!",0.7)
  return
 hp=maxf(0.0,hp-incoming)
 if hp<=0.0:_respawn_player()

func _terrain_grid_steer(_body:CharacterBody3D, desired:Vector3) -> Vector3:
 return desired # v18 overrides this hook; the original game remains unchanged.

func _update_beasts(delta: float) -> void:
 _ecology_update(delta)
 for beast in zone_beast_lists[region_index]:
  _update_beast_step(beast,delta)

func _update_beast_step(beast:Dictionary, delta:float) -> void:
 ai_updates+=1
 if not bool(beast.get("alive",false)):return
 var body:CharacterBody3D=beast["body"]
 if not is_instance_valid(body):return
 if float(beast.get("hit_flash_until",0.0))>0.0 and map_time>=float(beast.get("hit_flash_until",0.0)):
  beast["hit_flash_until"]=0.0
  var hit_label:Label3D=beast.get("nameplate") as Label3D
  if is_instance_valid(hit_label):
   hit_label.modulate=Color(str(REGIONS[int(beast["zone"])]["light"])) if str(beast.get("variant","normal"))!="normal" else Color("f4dcbb")
 var react_until:float=float(beast.get("hit_react_until",0.0))
 var swing_until:float=float(beast.get("attack_swing_until",0.0))
 if react_until>0.0 or swing_until>0.0:
  var hit_mesh:MeshInstance3D=beast.get("hit_visual") as MeshInstance3D
  if is_instance_valid(hit_mesh):
   var pose_degrees:float=90.0
   if map_time<react_until:
    pose_degrees+=V383_FEEDBACK.recoil_degrees(react_until-map_time,bool(beast.get("hit_react_critical",false)))
   elif map_time<swing_until:
    var swing_side:float=1.0 if int(beast["boss_id"])%2==0 else -1.0
    pose_degrees+=swing_side*V383_FEEDBACK.attack_followthrough_degrees(swing_until-map_time)
   hit_mesh.rotation_degrees=Vector3(0.0,0.0,pose_degrees)
  if map_time>=react_until:beast["hit_react_until"]=0.0
  if map_time>=swing_until:beast["attack_swing_until"]=0.0
 var diff:=player.global_position-body.global_position
 diff.y=0.0
 var distance:=diff.length()
 beast["attack"]=maxf(0.0,float(beast.get("attack",0.0))-delta)
 var kind:=str(beast["kind"])
 var staggered:=map_time<float(beast.get("stagger_until",0.0))
 var elite:=str(beast.get("variant","normal"))!="normal" and not bool(beast.get("boss",false))
 beast["elite_lunge_cd"]=maxf(0.0,float(beast.get("elite_lunge_cd",0.0))-delta)
 var move_speed:=float(BEASTS[kind]["speed"])
 var is_hostile:=bool(beast["hostile"])
 var perception:float=AI_RULES.perception(map_time<float(beast.get("alert_until",0.0)),weather_name,_day_phase()=="Night")
 var engaged:=is_hostile and distance<perception and _has_clear_sight(body,player)
 beast["engaged"]=engaged
 # v38.3: role-aware low-health retreat for live hostile fauna.
 # Preserve existing boss telegraphs, ecology, attacks and actor pools.
 var v383_intent:String=""
 if engaged and not bool(beast.get("boss",false)):
  var v383_role:String="ambusher" if kind=="panther" else "bruiser"
  var v383_health:float=float(beast.get("hp",1.0))/maxf(1.0,float(beast.get("max",1.0)))
  v383_intent=V383_COMBAT_RULES.combat_intent(v383_role,distance,v383_health,float(beast["attack"])<=0.0,true)
 var activity:Dictionary=beast.get("ecology_target",{})
 var pursuing:=not activity.is_empty() and map_time<float(beast.get("hunting_until",0.0)) and not engaged and not bool(beast.get("boss",false))
 if staggered:
  body.velocity=Vector3.ZERO
 elif engaged and map_time<float(beast.get("retreat_until",0.0)) and distance<10.0:
  body.velocity=-diff.normalized()*move_speed*0.9
 elif engaged and v383_intent=="retreat":
  # Flee while close; once outside the safe spacing, hold instead of chasing back in.
  body.velocity=-diff.normalized()*move_speed*0.85 if distance<8.0 else Vector3.ZERO
 elif engaged and v383_intent=="strafe" and distance>1.7 and distance<6.0:
  # Stable per-actor orbit side, no random calls or additional physics bodies.
  var flank:Vector3=Vector3(-diff.z,0.0,diff.x).normalized()
  var orbit_side:float=1.0 if int(beast["boss_id"])%2==0 else -1.0
  body.velocity=(flank*orbit_side+diff.normalized()*0.25).normalized()*move_speed*0.7
 elif engaged and distance>1.7:
  body.velocity=AI_RULES.approach(diff,move_speed,int(beast["boss_id"]),bool(beast.get("boss",false)),int(beast.get("boss_phase",1)),distance)
 elif not is_hostile and (distance<6.0 or map_time<float(beast.get("scared_until",0.0))):
  var escape:= -diff.normalized()
  if distance>8.0:
   escape=Vector3(cos(map_time+float(beast["boss_id"])),0,sin(map_time+float(beast["boss_id"])))
  body.velocity=escape*move_speed
 elif pursuing and bool(activity.get("alive",false)):
  var prey_body:CharacterBody3D=activity["body"]
  if is_instance_valid(prey_body):
   var to_prey:=prey_body.global_position-body.global_position
   to_prey.y=0
   if to_prey.length_squared()<1.55*1.55:
    activity["hp"]=float(activity["hp"])-float(BEASTS[kind]["atk"])*0.65
    beast["fed_until"]=map_time+7.0
    beast["hunting_until"]=0.0
    if float(activity["hp"])<=0.0:
     _remove_ecology_prey(activity)
     hunts_completed+=1
    body.velocity=Vector3.ZERO
   else:body.velocity=to_prey.normalized()*move_speed*0.8
  else:body.velocity=Vector3.ZERO
 elif body.global_position.distance_squared_to(beast["home"])>16.0:
  var toward:Vector3=beast["home"]
  toward-=body.global_position
  toward.y=0
  body.velocity=toward.normalized()*move_speed*0.4
 else:body.velocity=Vector3.ZERO
 # Single virtual call avoids per-frame reflection and supports future nav implementations.
 body.velocity=_terrain_grid_steer(body,body.velocity)
 body.move_and_slide()
 # v9 stagger cancels regular attacks. Boss windups remain uninterruptible.
 if staggered:return
 # Elite creatures telegraph a longer reach attack with a fixed target point.
 if elite:
  if float(beast.get("elite_lunge_windup",0.0))>0.0:
   beast["elite_lunge_windup"]=maxf(0.0,float(beast["elite_lunge_windup"])-delta)
   if float(beast["elite_lunge_windup"])<=0.0:
    var strike_pos:Vector3=beast["elite_lunge_target"]
    if COMBAT_RULES.elite_missed(strike_pos,player.global_position,dodge_time):elite_attacks_dodged+=1
    elif _has_clear_sight(body,player):_apply_incoming_damage(float(BEASTS[kind]["atk"])*1.6)
    beast["attack"]=2.0
   return
  if engaged and distance>=1.8 and distance<ELITE_LUNGE_RANGE and float(beast["elite_lunge_cd"])<=0.0:
   beast["elite_lunge_windup"]=0.9
   beast["elite_lunge_target"]=player.global_position
   beast["elite_lunge_cd"]=5.0
   body.velocity=Vector3.ZERO
   _show_message("Elite attack incoming! Dodge the marked position.",1.1)
   return
 # Telegraphs resolve after a real warning; no damage from inside walls.
 if bool(beast.get("boss",false)):
  if float(beast.get("windup",0))>0:
   beast["windup"]=maxf(0.0,float(beast["windup"])-delta)
   _set_boss_telegraph(beast,true)
   if float(beast["windup"])<=0.0:
    _set_boss_telegraph(beast,false)
    var marked:Vector3=beast.get("warning_pos",body.global_position)
    var warning_radius:float=float(beast.get("warning_radius",2.8))
    var near_hit:=Vector2(marked.x-player.global_position.x,marked.z-player.global_position.z).length_squared()<warning_radius*warning_radius
    if near_hit and dodge_time<=0 and _has_clear_sight(body,player):
     _apply_incoming_damage(19.0+7.0*float(int(beast.get("boss_phase",1))))
    beast["attack"]=COMBAT_RULES.guardian_attack_cooldown(int(beast.get("boss_phase",1)))
  elif engaged and distance<(5.0 if int(beast.get("boss_phase",1))>=2 else 3.3) and float(beast["attack"])<=0.0:
   var phase:int=int(beast.get("boss_phase",1))
   var pattern:int=int(beast.get("boss_pattern",0))
   # v32.8: two readable patterns in phase one, then +1 per phase.
   # Keep the original windup, damage, dodge and cooldown systems.
   var pattern_count:int=mini(4,phase+1)
   pattern=posmod(pattern,pattern_count)
   beast["boss_pattern"]=(pattern+1)%pattern_count
   beast["windup"]=maxf(0.75,BOSS_WINDUP_SECONDS-0.25*float(phase-1))
   # Each pattern uses the existing single warning circle and collision test.
   # Pattern 0: focused player mark; 1: wide boss-centered sweep;
   # 2: compact precision mark; 3: medium delayed player mark.
   beast["warning_radius"]=2.8 if pattern==0 else 3.8 if pattern==1 else 2.2 if pattern==2 else 3.1
   beast["warning_pos"]=body.global_position if pattern==1 else player.global_position
   _set_boss_telegraph(beast,true)
   _show_message("⚠ GUARDIAN PHASE "+str(phase)+": dodge the glowing circle!",1.6)
 elif engaged and distance<1.7 and float(beast["attack"])<=0.0:
  if dodge_time<=0 and _has_clear_sight(body,player):
   _apply_incoming_damage(float(BEASTS[kind]["atk"]))
  # Visual follow-through only; preserve established hit timing and damage.
  beast["attack_swing_until"]=map_time+V383_FEEDBACK.ATTACK_SWING_SECONDS
  beast["attack"]=1.5

func _despawn_native_beast(beast:Dictionary,delay:float) -> void:
 beast["alive"]=false
 beast["engaged"]=false
 beast["ecology_target"]={}
 beast["hunting_until"]=0.0
 beast["windup"]=0.0
 beast["respawn_at"]=-1.0 if bool(beast.get("boss",false)) else map_time+maxf(15.0,delay)
 var body:CharacterBody3D=beast["body"]
 if is_instance_valid(body):
  body.velocity=Vector3.ZERO
  body.collision_layer=0
  body.collision_mask=0
  body.hide()
 if bool(beast.get("boss",false)):_set_boss_telegraph(beast,false)

func _reset_native_beast(beast:Dictionary) -> void:
 var body:CharacterBody3D=beast["body"]
 if not is_instance_valid(body):return
 beast["alive"]=true
 beast["hp"]=float(beast["max"])
 beast["attack"]=0.0
 beast["hit_react_until"]=0.0
 beast["hit_react_critical"]=false
 beast["hit_flash_until"]=0.0
 beast["attack_swing_until"]=0.0
 var reset_mesh:MeshInstance3D=beast.get("hit_visual") as MeshInstance3D
 if is_instance_valid(reset_mesh):reset_mesh.rotation_degrees=Vector3(0.0,0.0,90.0)
 if not bool(beast.get("boss",false)):
  var reset_label:Label3D=beast.get("nameplate") as Label3D
  if is_instance_valid(reset_label):
   reset_label.modulate=Color(str(REGIONS[int(beast["zone"])]["light"])) if str(beast.get("variant","normal"))!="normal" else Color("f4dcbb")
 beast["stagger_until"]=0.0
 beast["stagger_resist_until"]=0.0
 beast["boss_phase"]=1
 if bool(beast.get("boss",false)):
  var nameplate:Label3D=beast["nameplate"]
  if is_instance_valid(nameplate):
   nameplate.text="ZONE GUARDIAN • "+str(beast["kind"]).capitalize()
   nameplate.modulate=Color(str(REGIONS[int(beast["zone"])]["light"]))
 beast["boss_pattern"]=0
 beast["warning_radius"]=2.8
 beast["alert_until"]=0.0
 beast["max"]=float(beast.get("base_hp",beast["max"]))
 beast["elite_lunge_cd"]=2.0
 beast["elite_lunge_windup"]=0.0
 beast["retreat_until"]=0.0
 beast["retreat_cooldown"]=0.0
 beast["windup"]=0.0
 beast["engaged"]=false
 beast["ecology_target"]={}
 beast["hunting_until"]=0.0
 beast["scared_until"]=0.0
 beast["fed_until"]=0.0
 beast["respawn_at"]=0.0
 var home_position:Vector3=beast["home"]
 body.position=home_position+Vector3(0,0.62,0)
 body.velocity=Vector3.ZERO
 body.collision_layer=1
 body.collision_mask=1
 body.show()
 if bool(beast.get("boss",false)):_set_boss_telegraph(beast,false)

func _update_respawns() -> void:
 # Spawn from fixed slots, never append new monsters or allocate more physics bodies.
 for beast in beasts:
  if bool(beast.get("alive",false)) or bool(beast.get("boss",false)) or int(beast.get("dungeon_site",-1))>=0:continue
  var ready:=float(beast.get("respawn_at",0.0))
  if ready<0.0 or map_time<ready:continue
  var home:Vector3=beast["home"]
  if player.global_position.distance_squared_to(home)<MIN_RESPAWN_DISTANCE*MIN_RESPAWN_DISTANCE:continue
  _reset_native_beast(beast)
  respawns_total+=1

func _remove_ecology_prey(prey:Dictionary) -> void:
 # Nature kills never produce player loot, gold, skinnable bodies, or contract credit.
 if not bool(prey.get("alive",false)):return
 _despawn_native_beast(prey,ECOLOGY_RESPAWN_SECONDS)

func _set_boss_telegraph(beast:Dictionary,active:bool) -> void:
 var id:=int(beast.get("boss_id",-1))
 if not boss_telegraphs.has(id):
  var warning:=MeshInstance3D.new()
  var disk:=CylinderMesh.new()
  disk.bottom_radius=float(beast.get("warning_radius",2.8))
  disk.top_radius=float(beast.get("warning_radius",2.8))
  disk.height=0.025
  disk.radial_segments=36
  warning.mesh=disk
  warning.material_override=_material(Color(0.9,0.18,0.18,0.55),0.7,true)
  world.add_child(warning)
  boss_telegraphs[id]=warning
 var node:MeshInstance3D=boss_telegraphs[id]
 if not is_instance_valid(node):return
 node.visible=active
 # Warn at the true hit radius; subtle faster opacity pulses in later phases.
 node.transparency=V383_FEEDBACK.boss_telegraph_transparency(map_time,int(beast.get("boss_phase",1))) if active else 0.0
 if active:
  var body:CharacterBody3D=beast["body"]
  if is_instance_valid(body):
   var area:Vector3=beast.get("warning_pos",body.global_position)
   var disk: CylinderMesh=node.mesh as CylinderMesh
   if is_instance_valid(disk):
    var desired_radius:float=float(beast.get("warning_radius",2.8))
    if V383_FEEDBACK.warning_radius_changed(disk.bottom_radius,desired_radius):
     disk.bottom_radius=desired_radius
    if V383_FEEDBACK.warning_radius_changed(disk.top_radius,desired_radius):
     disk.top_radius=desired_radius
   node.position=Vector3(area.x,0.03,area.z)

func _update_companion(delta: float) -> void:
 if companion.is_empty():return
 if not is_instance_valid(companion_node):
  companion_node=_add_sphere(player.position+Vector3(1,0.4,1),0.37,Color("91d8a7"),false,true)
 var at:=player.position+Vector3(1.1,0.2,1.0)
 companion_node.position=companion_node.position.lerp(at,minf(1.0,delta*(4.0+float(learned_talents.get("stride",0)))))
 var heal_rank:=float(learned_talents.get("soulbond",0))+float(learned_talents.get("recover",0))
 if heal_rank>0:
  companion["hp"]=minf(float(companion.get("max",80)),float(companion.get("hp",80))+delta*heal_rank*0.6)
 if mounted or companion_attack_timer>0 or map_time<pet_stunned_until:return
 var target:Dictionary={}
 if companion_stance=="Passive":return
 var distance:float=(3.8 if companion_stance=="Follow" else 5.0)+float(learned_talents.get("guardian",0))*0.5
 for candidate in zone_beast_lists[region_index]:
  if not bool(candidate.get("alive",false)) or not bool(candidate.get("hostile",false)):continue
  if companion_stance=="Follow" and not bool(candidate.get("engaged",false)):continue
  var body:CharacterBody3D=candidate["body"]
  if not is_instance_valid(body):continue
  if body.global_position.distance_to(companion_node.global_position)>=distance:continue
  if not _has_clear_sight(player,body):continue
  target=candidate
  break
 if target.is_empty():return
 var dmg:=9.0+float(learned_talents.get("sharp",0))*2.0+float(learned_talents.get("kinship",0))
 if bool(target.get("boss",false)) or str(target.get("variant","normal"))!="normal":dmg*=1.0+0.10*float(learned_talents.get("predator",0))
 if float(learned_talents.get("apex",0))>0 and rng.randf()<0.06*float(learned_talents.get("apex",0)):dmg*=1.75
 if int(learned_talents.get("alpha",0))>0 and rng.randf()<0.06*float(learned_talents.get("alpha",0)):target["attack"]=maxf(float(target.get("attack",0)),0.75)
 target["hp"]=float(target["hp"])-dmg
 _refresh_boss_phase(target)
 companion_attack_timer=maxf(0.5,1.35-0.14*float(learned_talents.get("frenzy",0)))
 if float(target["hp"])<=0:_defeat_beast(target,true)

func _nearest_beast(radius: float,hostile_only: bool=false,require_sight: bool=false) -> Dictionary:
 var nearest:Dictionary={}
 var dist_squared:float=maxf(0.0,radius)*maxf(0.0,radius)
 for beast in zone_beast_lists[region_index]:
  if not bool(beast["alive"]):continue
  if hostile_only and not bool(beast["hostile"]):continue
  var body:CharacterBody3D=beast["body"]
  if not is_instance_valid(body):continue
  var d_squared:float=player.global_position.distance_squared_to(body.global_position)
  if d_squared<dist_squared and (not require_sight or _has_clear_sight(player,body)):
   dist_squared=d_squared
   nearest=beast
 return nearest

# WoW-style floating damage over the target: bounded, short-lived, no hero-art changes.
const MAX_DAMAGE_FLOATERS_PER_TARGET:int=6

func _show_floating_damage(target:Dictionary,amount:int,critical:bool)->void:
 var body:CharacterBody3D=target.get("body") as CharacterBody3D
 if not is_instance_valid(body):return
 # Existing child ordering tracks age; recycled labels move to the end.
 # Count in one pass instead of allocating a new array on every hit.
 var active_count:int=0
 var oldest:Label3D=null
 for child in body.get_children():
  if child is Label3D and (child.has_meta("damage_float_tween") or child.name.begins_with("DamageFloat")) and not child.is_queued_for_deletion():
   active_count+=1
   if oldest==null:oldest=child as Label3D
 var label:Label3D
 if active_count>=MAX_DAMAGE_FLOATERS_PER_TARGET and is_instance_valid(oldest):
  # Kill the former animation before recycling, preventing its cleanup callback.
  label=oldest
  var old_tween:Tween=label.get_meta("damage_float_tween",null) as Tween
  if old_tween!=null and old_tween.is_valid():old_tween.kill()
  body.move_child(label,body.get_child_count()-1)
 else:
  label=Label3D.new()
  label.name="DamageFloat"
  body.add_child(label)
 # Always restore the baseline before applying critical size and color.
 label.show()
 label.scale=Vector3.ONE
 label.text=("%d!" % amount) if critical else str(amount)
 label.font_size=72 if critical else 46
 label.pixel_size=0.005
 label.outline_size=10 if critical else 8
 label.billboard=BaseMaterial3D.BILLBOARD_ENABLED
 label.no_depth_test=true
 label.modulate=Color("fff8d2") if critical else Color("fff4e8")
 # Use the hit sequence, not the capped label count, to keep rapid hits
 # alternating across sides and vertical lanes after recycling begins.
 var hit_sequence:int=int(target.get("damage_float_sequence",0))
 target["damage_float_sequence"]=hit_sequence+1
 var side:float=-0.3 if hit_sequence%2==0 else 0.3
 label.position=Vector3(side,1.65+0.12*float(hit_sequence%3),0.0)
 # Floaters still self-clean after their short animation. No permanent pools.
 var finish:Vector3=label.position+Vector3(side*0.35,1.05 if critical else 0.6,0.0)
 var tween:Tween=label.create_tween()
 label.set_meta("damage_float_tween",tween)
 tween.set_parallel(true)
 tween.tween_property(label,"position",finish,1.0 if critical else 0.85).set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_OUT)
 tween.tween_property(label,"modulate:a",0.0,0.36).set_delay(0.64 if critical else 0.49)
 if critical:
  label.scale=Vector3.ONE*1.3
  tween.tween_property(label,"scale",Vector3.ONE,0.22).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
  tween.tween_property(label,"modulate:g",0.827,0.24).set_delay(0.10)
  tween.tween_property(label,"modulate:b",0.302,0.24).set_delay(0.10)
 tween.set_parallel(false)
 tween.tween_callback(label.queue_free)

func _apply_player_hit(target:Dictionary, base_damage:float, stagger_strength:float=0.0) -> void:
 if target.is_empty() or not bool(target.get("alive",false)):return
 var critical:=rng.randf()<minf(0.32,0.07+float(hero_level)*0.0015)
 last_hit_critical=critical
 if critical:critical_hits+=1
 var damage:float=COMBAT_RULES.outgoing_damage(base_damage,critical,float(target["hp"])<=float(target["max"])*0.5,_perk_rank("execute"),_gear_affix_value("Fury"),_power_modifier(int(target.get("zone",region_index))))
 target["hp"]=float(target["hp"])-damage
 # Lightweight readable impact feedback on existing non-boss nameplates.
 # Avoid animations, new nodes, or alterations to the hero artwork.
 if not bool(target.get("boss",false)):
  var hit_label:Label3D=target.get("nameplate") as Label3D
  if is_instance_valid(hit_label):
   hit_label.modulate=Color("ffdd7a") if critical else Color("ff877c")
   target["hit_flash_until"]=map_time+0.18
  target["hit_react_until"]=map_time+V383_FEEDBACK.REACTION_SECONDS
  target["hit_react_critical"]=critical
 last_hit_damage=int(round(damage))
 _show_floating_damage(target,last_hit_damage,critical)
 if critical:
  hp=minf(max_hp,hp+3.0*float(_perk_rank("bloodmoon")))
 if _gear_affix_value("Vampiric")>0.0:
  hp=minf(max_hp,hp+damage*_gear_affix_value("Vampiric"))
 target["alert_until"]=map_time+8.0
 for ally in zone_beast_lists[int(target["zone"])]:
  if not bool(ally.get("hostile",false)) or not bool(ally.get("alive",false)):continue
  var ally_body:CharacterBody3D=ally["body"]
  var target_body:CharacterBody3D=target["body"]
  if is_instance_valid(ally_body) and is_instance_valid(target_body) and ally_body.global_position.distance_squared_to(target_body.global_position)<64.0:
   ally["alert_until"]=map_time+5.0
 _refresh_boss_phase(target)
 if AI_RULES.retreat_due(float(target["hp"]),float(target["max"]),bool(target.get("boss",false)),int(target.get("dungeon_site",-1)),map_time,float(target.get("retreat_cooldown",0.0))):
  target["retreat_until"] = map_time + 2.2
  target["retreat_cooldown"] = map_time + 14.0
  enemy_retreats += 1
 if stagger_strength>0.0 and not bool(target.get("boss",false)) and map_time>=float(target.get("stagger_resist_until",0.0)):
  if rng.randf()<stagger_strength:
   target["stagger_until"]=map_time+STAGGER_DURATION
   target["stagger_resist_until"]=map_time+STAGGER_RESIST_SECONDS
   target["elite_lunge_windup"]=0.0
   stagger_events+=1
 if float(target["hp"])<=0.0:_defeat_beast(target,false)
 else:_show_message(("CRITICAL " if critical else "Hit ")+str(target["kind"])+" for "+str(last_hit_damage)+"!",0.8)

func _attack() -> void:
 if mounted:_show_message("Dismount before fighting. Riding is travel only.",1.5);return
 if attack_cooldown>0:return
 attack_cooldown=0.32*(1.0-_gear_affix_value("Haste"))
 var target:=_nearest_beast(3.1,false,true)
 if target.is_empty():_show_message("No visible beast in striking range.",0.6);return
 var damage:=26.0+float(_equipment_power())*0.6
 damage*=_hero_damage_multiplier()*(1.0+0.07*float(_perk_rank("rend")))
 if hunter_oil_time>0:damage*=1.18
 _apply_player_hit(target,damage,0.23)

func _spell(which: String) -> void:
 if mounted:_show_message("No mounted combat. Dismount first.",1.6);return
 if ability_cooldown>0:return
 ability_cooldown=3.5*maxf(0.65,1.0-0.08*float(_perk_rank("celerity")))
 if which=="dash":
  dodge_time=0.55
  _show_message("E: Shadow Dash! Dodge and reposition.",1)
  return
 var power:float=(20.0 if which=="nova" else 35.0)*_hero_damage_multiplier()*(1.0+0.08*float(_perk_rank("focus")))
 var hits:=0
 var spell_radius:float=(5.0 if which=="nova" else 3.8)+0.5*float(_perk_rank("reach"))
 var spell_radius_squared:float=spell_radius*spell_radius
 for beast in zone_beast_lists[region_index]:
  if not bool(beast["alive"]):continue
  var body:CharacterBody3D=beast["body"]
  if is_instance_valid(body) and body.position.distance_squared_to(player.position)<spell_radius_squared and _has_clear_sight(player,body):
   _apply_player_hit(beast,power,0.36 if which=="nova" else 0.18)
   hits+=1
 _show_message(("Moon Nova" if which=="nova" else "Eclipse Flame")+": "+str(hits)+" hits",1.3)

func _dodge() -> void:
 if mounted:_show_message("No dodging on a mount.",1);return
 if dodge_cooldown>0:return
 dodge_time=0.5
 dodge_cooldown=1.6*(1.0-0.5*_gear_affix_value("Haste"))

func _toggle_mount() -> void:
 if mounted:
  mounted=false
  _show_message("Dismounted.",1)
  return
 if _combat_threat():_show_message("Cannot mount while enemies are attacking.",2);return
 mounted=true
 mount_combat_time=0
 _show_message("Mounted: travel only. Combat dismounts after 20 seconds.",2.5)

func _toggle_senses() -> void:
 senses_on=not senses_on
 for clue in clue_data:
  var node:Node3D=clue["node"]
  if is_instance_valid(node):node.visible=senses_on and not (int(clue["id"]) in found_clues)
 _show_message("Hunter Senses "+("ON" if senses_on else "OFF"),1.7)

func _defeat_beast(beast: Dictionary,pet_kill: bool) -> void:
 if not bool(beast["alive"]):return
 var body:CharacterBody3D=beast["body"]
 var at:Vector3=body.global_position
 _despawn_native_beast(beast,NORMAL_RESPAWN_SECONDS)
 kills_total+=1
 var kind:=str(beast["kind"])
 var variant:=str(beast["variant"])
 var zone:=int(beast["zone"])
 gold+=5+zone*3
 # Classic gold rewards remain instant and unchanged. Bonus silver/copper
 # become a small physical pickup near the defeated creature.
 _create_coin_drop(at,MONEY_RULES.bonus_loot(zone,variant,bool(beast.get("boss",false)),rng.randi_range(0,99)))
 hp=minf(max_hp,hp+4.0*float(_perk_rank("renewal")))
 _award_experience(zone,variant,bool(beast.get("boss",false)))
 _gain_material(str(BEASTS[kind]["drop"]),1)
 var event_completed_on_kill := _world_event_on_defeat(beast)
 if variant=="moon":_gain_material("Moon Sigil",1)
 if variant=="dusk":_gain_material("Dusk Cinder",1)
 if variant=="elder":_gain_material("Elder Rootstone",1)
 var loot_chance:=0.42 if variant=="normal" else 1.0
 if rng.randf()<loot_chance:_create_gear_drop(at,kind,variant,zone)
 _make_carcass(at,kind,variant,zone)
 if int(beast.get("dungeon_site",-1))>=0:_on_rift_enemy_defeated(beast)
 if bool(beast.get("boss",false)):
  var boss_id:=int(beast.get("boss_id",-1))
  if not (boss_id in defeated_boss_ids):
   defeated_boss_ids.append(boss_id)
   _gain_material("Boss Seal",1)
   var artifact:Dictionary={"name":"Guardian of "+str(REGIONS[zone]["name"])+" Relic","rarity":"Legendary","power":45+zone*11,"affix":AFFIXES[zone % AFFIXES.size()],"affix_value":0.12+0.02*float(zone%3)}
   _create_gear_drop(at,"Guardian Relic","elder",zone,artifact)
   gold+=70+zone*25
   _show_message("ZONE GUARDIAN DEFEATED! Boss Seal and rare gear earned.",3.5)
 if zone==contract_zone and bool(beast["hostile"]):
  contract_kills+=1
  if contract_kills>=3:
   if not (contract_zone in contracts_done):
    contracts_done.append(contract_zone)
    gold+=40+zone*20
    _gain_material("Moon Sigil",2)
    tamer_points+=1
    _create_gear_drop(at,"Contract Relic","elder",zone)
   contract_zone=-1
   contract_kills=0
   _show_message("MONSTER CONTRACT COMPLETE! Bonus gold, gear and reagents.",3)
  else:_show_message("Contract kill: "+str(contract_kills)+"/3",2)
 else:_show_message(("Companion" if pet_kill else "You")+" defeated a "+variant+" "+kind+". Loot + carcass!",2)
 if auto_save_cooldown<=0.0 or event_completed_on_kill:
  auto_save_cooldown=8.0
  suppress_autosave_message=true
  _save_game()
  suppress_autosave_message=false

func _gain_material(name: String,amount: int) -> void:
 INVENTORY_RULES.add_material(materials,name,amount)

func _create_gear_drop(at: Vector3,kind: String,variant: String,zone: int,existing: Dictionary={}) -> void:
 if gear_drops.size()>=MAX_GROUND_DROPS:
  var discard_index:int=INVENTORY_RULES.first_discardable_drop(gear_drops)
  if discard_index>=0:
   var old:Node3D=gear_drops[discard_index]["node"]
   if is_instance_valid(old):old.queue_free()
   gear_drops.remove_at(discard_index)
  else:
   # Do not silently lose a guaranteed rift or guardian legendary when the ground is full.
   if str(existing.get("rarity","")) in ["Legendary","Mythic"] and INVENTORY_RULES.can_store_gear(gear_items.size(),bag_capacity):
    gear_items.append(_safe_item(existing))
    _show_message("Legendary sent to bag (ground loot full).",2.5)
   elif str(existing.get("rarity","")) in ["Epic","Legendary","Mythic"]:
    _gain_material("Salvage Token",1)
   return
 var rarity:="Common"
 var power:=4+zone*5+rng.randi_range(0,9)
 if zone>1:rarity="Uncommon"
 if zone>3:rarity="Rare"
 if variant=="moon":rarity="Epic";power+=9
 if variant=="dusk":rarity="Legendary";power+=15
 if variant=="elder":rarity="Mythic";power+=20
 var name:String=kind.capitalize()+" "+str(["Charm","Band","Cowl","Boots","Mantle"][rng.randi_range(0,4)])
 var gear:Dictionary={"name":name,"rarity":rarity,"power":power}
 if rarity in ["Legendary","Mythic"]:
  gear["affix"]=AFFIXES[rng.randi_range(0,AFFIXES.size()-1)]
  gear["affix_value"]=0.06+0.02*float(zone)+rng.randf_range(0.0,0.03)
 if not existing.is_empty():gear=existing.duplicate(true)
 var root:=Node3D.new()
 root.position=Vector3(at.x,0,at.z)
 root.name="GroundGear_"+str(gear["rarity"])
 world.add_child(root)
 var color:=Color("dfe6ee")
 match str(gear["rarity"]):
  "Uncommon":color=Color("63efa3")
  "Rare":color=Color("65adff")
  "Epic":color=Color("bc7bff")
  "Legendary":color=Color("ffb345")
  "Mythic":color=Color("fa678e")
 var orb:=MeshInstance3D.new()
 var sphere:=SphereMesh.new()
 sphere.radius=0.22
 sphere.height=0.44
 orb.mesh=sphere
 orb.position=Vector3(0,0.35,0)
 orb.material_override=_material(color,0.3,true)
 root.add_child(orb)
 if str(gear["rarity"]) in ["Epic","Legendary","Mythic"]:
  var light:=OmniLight3D.new()
  light.position=Vector3(0,0.9,0)
  light.omni_range=3.1
  light.light_energy=0.9
  light.light_color=color
  root.add_child(light)
 var beam:=Label3D.new()
 beam.text="◆ "+str(gear["rarity"]).to_upper()+" "+str(gear["name"])+" (G)"
 beam.position=Vector3(0,1.25,0)
 beam.billboard=BaseMaterial3D.BILLBOARD_ENABLED
 beam.modulate=color
 beam.font_size=25
 beam.pixel_size=0.0048
 root.add_child(beam)
 gear_drops.append({"node":root,"data":gear})

func _make_carcass(at: Vector3,kind: String,variant: String,zone: int,remaining: float=90.0) -> void:
 if carcasses.size()>=MAX_CARCASSES:
  var old:Dictionary=carcasses.pop_front()
  var old_node:Node3D=old["node"]
  if is_instance_valid(old_node):old_node.queue_free()
 var node:=Node3D.new()
 node.position=Vector3(at.x,0.13,at.z)
 node.name="Skinnable_"+kind
 world.add_child(node)
 var marker:=MeshInstance3D.new()
 var mesh:=SphereMesh.new()
 mesh.radius=0.4
 mesh.height=0.22
 marker.mesh=mesh
 marker.material_override=_material(Color("654c50"))
 node.add_child(marker)
 var title:=Label3D.new()
 title.text="SKIN "+kind.to_upper()+" · G"
 title.position=Vector3(0,0.78,0)
 title.billboard=BaseMaterial3D.BILLBOARD_ENABLED
 title.font_size=25
 title.pixel_size=0.005
 title.modulate=Color("e7c59a")
 node.add_child(title)
 carcasses.append({"node":node,"kind":kind,"variant":variant,"zone":zone,"remaining":remaining})

func _update_skinning(delta: float,dir: Vector3) -> void:
 for carcass in carcasses.duplicate():
  carcass["remaining"]=float(carcass["remaining"])-delta
  if float(carcass["remaining"])<=0:
   var old_node:Node3D=carcass["node"]
   if is_instance_valid(old_node):old_node.queue_free()
   carcasses.erase(carcass)
 if not is_instance_valid(skinning_target) or skinning_left<=0:return
 if dir.length_squared()>0.01 or _combat_threat() or player.position.distance_to(skinning_origin)>0.25:
  skinning_left=0
  skinning_target=null
  _show_message("Skinning interrupted.",1.4)
  return
 skinning_left-=delta
 if skinning_left>0:return
 for carcass in carcasses.duplicate():
  if carcass["node"]!=skinning_target:continue
  var kind:=str(carcass["kind"])
  var variant:=str(carcass["variant"])
  var zone:=int(carcass["zone"])
  var required:=1+zone*6
  if skinning_skill<required:_show_message("Skinning skill "+str(required)+" required.",2);break
  _gain_material(str(BEASTS[kind]["skin"]),1+rng.randi_range(0,1))
  if variant!="normal":_gain_material(variant.capitalize()+" Hide",1)
  skinning_skill=mini(300,skinning_skill+1)
  var old_node:Node3D=carcass["node"]
  if is_instance_valid(old_node):old_node.queue_free()
  carcasses.erase(carcass)
  _show_message("Skinned "+kind+". Skinning "+str(skinning_skill)+"/300",2)
  _save_game()
  break
 skinning_target=null
 skinning_left=0

func _has_unblocked_interaction(target: Node3D) -> bool:
 # Pickup items are not physics bodies. Cast against solid scenery only.
 if not is_instance_valid(target) or not is_instance_valid(player):return false
 var origin:Vector3=player.global_position+Vector3.UP*0.35
 var end:Vector3=target.global_position+Vector3.UP*0.35
 var query:=PhysicsRayQueryParameters3D.create(origin,end)
 query.collision_mask=1
 query.exclude=[player.get_rid()]
 return get_world_3d().direct_space_state.intersect_ray(query).is_empty()

func _interact() -> void:
 if mounted:_show_message("Dismount to interact.",1);return
 # Nearest valid drop/carcass wins within its class; no first-in-array surprises.
 var closest_gear:Dictionary={}
 var closest_gear_dist:=3.2
 for drop in gear_drops:
  var node:Node3D=drop["node"]
  if not is_instance_valid(node):continue
  var distance:=node.position.distance_to(player.position)
  if distance<closest_gear_dist and _has_unblocked_interaction(node):
   closest_gear_dist=distance
   closest_gear=drop
 if not closest_gear.is_empty():
  if not INVENTORY_RULES.can_store_gear(gear_items.size(),bag_capacity):_show_message("Bag full. Ground equipment preserved.",2);return
  var gear_node:Node3D=closest_gear["node"]
  var collected:Dictionary=closest_gear["data"]
  gear_items.append(collected.duplicate(true))
  gear_node.queue_free()
  gear_drops.erase(closest_gear)
  pickup_count+=1
  _show_message("Picked up "+str(closest_gear["data"]["name"])+"!",1.7)
  _save_game()
  return
 for site_id in range(rifts.size()):
  var entry:Dictionary=rifts[site_id]
  if player.global_position.distance_to(entry["anchor"])<=4.5:
   _start_rift(site_id)
   return
 for event_id in range(world_events.size()):
  var event:Dictionary=world_events[event_id]
  if player.global_position.distance_to(event["anchor"])<=4.5:
   _begin_world_event(event_id)
   return
 var closest_corpse:Dictionary={}
 var closest_corpse_dist:=2.9
 for corpse in carcasses:
  var corpse_node:Node3D=corpse["node"]
  if not is_instance_valid(corpse_node):continue
  var distance:=corpse_node.position.distance_to(player.position)
  if distance<closest_corpse_dist and _has_unblocked_interaction(corpse_node):
   closest_corpse_dist=distance
   closest_corpse=corpse
 if not closest_corpse.is_empty():
  if _combat_threat():_show_message("Cannot skin during active combat.",1.5);return
  var req:=1+int(closest_corpse["zone"])*6
  if skinning_skill<req:_show_message("Skinning "+str(req)+" required. Current: "+str(skinning_skill),2);return
  skinning_target=closest_corpse["node"]
  skinning_left=1.5
  skinning_origin=player.position
  _show_message("Skinning... stand still for 1.5 seconds.",1.6)
  return
 for i in range(region_shrines.size()):
  if player.global_position.distance_to(region_shrines[i].position)<4.5:
   if not (i in shrine_discovery):shrine_discovery.append(i)
   hp=max_hp
   region_index=i
   _save_game()
   _show_message(str(REGIONS[i]["name"])+" shrine awakened! Checkpoint saved.",2.8)
   return
 if senses_on:
  for clue in clue_data:
   var clue_node:Node3D=clue["node"]
   var clue_id:=int(clue["id"])
   if clue_id in found_clues or not is_instance_valid(clue_node):continue
   if clue_node.position.distance_to(player.position)<3.1:
    found_clues.append(clue_id)
    clue_node.visible=false
    _gain_material("herb",1)
    _show_message("Investigation clue discovered! +1 herb",1.8)
    _save_game()
    return
 _show_message("Nothing nearby. G interacts with rifts, world events, loot and shrines.",1.5)

func _tame() -> void:
 if mounted:_show_message("Dismount before taming.",1.6);return
 if not companion.is_empty():_show_message("You already have an active companion. Use the Tamer menu.",2);return
 var reach:=2.6+float(learned_talents.get("gentle",0))*0.4
 var beast:=_nearest_beast(reach,false,true)
 if beast.is_empty():_show_message("No nearby creature to tame.",1.7);return
 var threshold:=0.35+float(learned_talents.get("wildspeak",0))*0.13
 if float(beast["hp"])/float(beast["max"])>threshold:
  _show_message("Weaken the creature before taming (or unlock Wildspeaker).",2)
  return
 var kind:=str(beast["kind"])
 var rarity:=str(beast["variant"])
 _despawn_native_beast(beast,NORMAL_RESPAWN_SECONDS)
 companion={"kind":kind,"variant":rarity,"hp":80.0+float(learned_talents.get("familiar",0))*8.0,"max":80.0+float(learned_talents.get("familiar",0))*8.0}
 if not (kind in discovered_species):
  discovered_species.append(kind)
  tamer_points+=2 if discovered_species.size()==1 else 1
 _show_message("Tamed "+rarity+" "+kind+"! Companion joined. +Tamer points.",3)
 _save_game()

func _take_contract(zone: int) -> void:
 if zone<0 or zone>=REGIONS.size() or zone in contracts_done:return
 contract_zone=zone
 contract_kills=0
 _show_message("Contract accepted: hunt 3 hostile beasts in "+str(REGIONS[zone]["name"]),3)
 _save_game()

func _travel_to(zone: int) -> void:
 if zone<0 or zone>=REGIONS.size() or not (zone in shrine_discovery):return
 if _combat_threat():_show_message("Fast travel blocked during combat.",2);return
 mounted=false
 mount_combat_time=0
 skinning_target=null
 skinning_left=0.0
 var pos:Vector3=region_shrines[zone].position
 player.position=pos+Vector3(0,0.85,4.4)
 region_index=zone
 _update_region_visibility()
 _show_message("Fast traveled to "+str(REGIONS[zone]["name"]),2)
 _save_game()
 _toggle_menu()

func _buy_talent(index: int) -> void:
 if index<0 or index>=TALENTS.size():return
 var perk:Dictionary=TALENTS[index]
 var id:=str(perk["id"])
 var rank:=int(learned_talents.get(id,0))
 if tamer_points<1 or rank>=3:
  _show_message("Insufficient Tamer points or maximum rank.",2)
  return
 var branch:=str(perk["branch"])
 var spent:=0
 for p in TALENTS:
  if str(p["branch"])==branch:spent+=int(learned_talents.get(str(p["id"]),0))
 if index>=15:
  if spent<10 or int(learned_talents.get(str(TALENTS[[4,9,14][index-15]]["id"]),0))<1:
   _show_message("Mastery requires 10 branch points and the final branch perk.",2)
   return
 elif index%5>0:
  if int(learned_talents.get(str(TALENTS[index-1]["id"]),0))<1 or spent<(index%5)*2:
   _show_message("Unlock the previous node and invest "+str((index%5)*2)+" branch points first.",2)
   return
 learned_talents[id]=rank+1
 tamer_points-=1
 if id=="spirit":tamer_points+=1 if rank==0 else 0
 if id=="familiar" and not companion.is_empty():
  var extra:=float(companion.get("max",80))*0.08
  companion["max"]=float(companion["max"])+extra
  companion["hp"]=minf(float(companion["max"]),float(companion["hp"])+extra)
 _show_message(str(perk["name"])+" upgraded to "+str(rank+1)+"/3",2)
 _save_game()

func _craft(index: int) -> void:
 if index<0 or index>=RECIPES.size():return
 if _combat_threat():_show_message("Cannot craft while enemies are attacking.",2);return
 var recipe:Dictionary=RECIPES[index]
 var issue:String=INVENTORY_RULES.craft_issue(recipe,materials,gear_items.size(),bag_capacity)
 if not issue.is_empty():_show_message(issue,2);return
 var result:Dictionary=INVENTORY_RULES.craft_commit(recipe,materials)
 if not result["gear"].is_empty():gear_items.append(result["gear"])
 else:_gain_material(str(result["material"]),1)
 crafting_skill=mini(300,crafting_skill+1)
 _show_message("Crafted "+str(recipe["name"])+"!",2)
 _save_game()

func _use_consumable(item: String) -> void:
 if mounted:_show_message("Dismount to use combat consumables.",1.5);return
 if not INVENTORY_RULES.consume_one(materials,item):_show_message("No "+item+". Craft it under Professions.",2);return
 if item=="Hunter's Oil":hunter_oil_time=25.0
 if item=="Healing Draught":hp=minf(max_hp,hp+max_hp*0.35)
 _show_message(item+" used.",1.5)
 _save_game()

func _equip(index: int) -> void:
 if index<0 or index>=gear_items.size():return
 equipped=INVENTORY_RULES.equip_copy(gear_items,index)
 _show_message("Equipped "+str(equipped["name"]),2)
 _save_game()

func _equipment_power() -> int:
 return INVENTORY_RULES.equipment_power(equipped)

func _respawn_player() -> void:
 hp=max_hp
 mounted=false
 mount_combat_time=0
 skinning_target=null
 skinning_left=0
 var fallback:=0
 if shrine_discovery.size()>0:fallback=shrine_discovery[-1]
 var pos:Vector3=region_shrines[fallback].position
 player.position=pos+Vector3(0,0.85,4.2)
 region_index=_region_here()
 _update_region_visibility()
 _show_message("Defeated. Returned to your last awakened shrine.",3)

func _apply_graphics_quality(mode:String) -> void:
 graphics_quality="Smooth" if mode=="Smooth" else "Full"
 Engine.max_fps=60 if mode=="Smooth" else 0
 get_viewport().msaa_3d=Viewport.MSAA_DISABLED if mode=="Smooth" else Viewport.MSAA_2X
 var light:=get_node_or_null("TwinMoonlight") as DirectionalLight3D
 if is_instance_valid(light):light.shadow_enabled=(mode!="Smooth")
 _show_message("Graphics: "+graphics_quality,1.2)

func _release_label() -> String:
 return "v13"

func _update_hud() -> void:
 if not is_instance_valid(hud):return
 hud.text="MOONFALL NATIVE "+_release_label()+"  •  "+("NYRA" if hero=="nyra" else "Z")+"  ♥ "+str(int(hp))+"/"+str(int(max_hp))+"  ◆ "+_wallet_text()+"  ⚔ "+str(kills_total)+" defeats  ✦ LV "+str(hero_level)+"  XP "+str(hero_xp)+"/"+str(hero_xp_to_next)
 hint.text="📍 "+str(REGIONS[region_index]["name"])+"  • Day "+str(world_day)+" "+_day_phase()+"  • "+weather_name+("  •  MOUNTED" if mounted else "  •  SENSES" if senses_on else "")
 status.text=status_message
 world_hud.text="CRITS "+str(critical_hits)+"  | STAGGERS "+str(stagger_events)+"  | COMBO "+str(combat_combo if map_time<=combat_combo_until else 0)+"  | HERO PTS "+str(hero_points)+"  | TAMER PTS "+str(tamer_points)+"  | POWER "+str(_current_power())+"/"+str(_recommended_power(region_index))+"  | PET "+companion_stance+"  | RIFTS "+str(rifts_cleared.size())+"/3  | EVENTS "+str(events_completed.size())+"/6  | LOOT "+str(gear_drops.size())+("  | SKIN "+str(snappedf(skinning_left,0.1))+"s" if skinning_left>0 else "")

func _save_game() -> void:
 if not hero_playing:return
 var drops: Array=[]
 for d in gear_drops:
  var node:Node3D=d["node"]
  if not is_instance_valid(node):continue
  drops.append({"x":node.position.x,"z":node.position.z,"gear":d["data"]})
 var bodies: Array=[]
 for c in carcasses:
  var node:Node3D=c["node"]
  if not is_instance_valid(node):continue
  bodies.append({"x":node.position.x,"z":node.position.z,"kind":c["kind"],"variant":c["variant"],"zone":c["zone"],"remaining":c["remaining"]})
 var dormant:Array=[]
 for index in range(beasts.size()):
  var creature:Dictionary=beasts[index]
  if bool(creature.get("boss",false)) or bool(creature.get("alive",false)):continue
  var remaining:=maxf(0.0,float(creature.get("respawn_at",0.0))-map_time)
  if remaining>0.0:dormant.append({"id":index,"remaining":remaining})
 var save:Dictionary={
  "schema":2,"saved_at_unix":Time.get_unix_time_from_system(),"hero":hero,"at":[player.position.x,player.position.z],"hp":hp,"gold":gold,"silver":silver,"copper":copper,"coin_drops":_save_coin_drops(),"opened_treasure_chests":opened_treasure_chests.duplicate(),
  "kills":kills_total,"level":hero_level,"xp":hero_xp,"xp_next":hero_xp_to_next,"hero_points":hero_points,"hero_perks":hero_perks.duplicate(true),"rifts":rifts_cleared.duplicate(),"shrines":shrine_discovery.duplicate(),"materials":materials.duplicate(true),
  "skinning":skinning_skill,"crafting":crafting_skill,"water_professions":water_professions.duplicate(true),"water_site_days":water_site_days.duplicate(true),"tamer_points":tamer_points,
  "talents":learned_talents.duplicate(true),"species":discovered_species.duplicate(),
  "companion":companion.duplicate(true),"gear":gear_items.duplicate(true),"bag_capacity":bag_capacity,"vault":vault_items.duplicate(true),"equipped":equipped.duplicate(true),
  "drops":drops,"carcasses":bodies,"contracts_done":contracts_done.duplicate(),
  "contract_zone":contract_zone,"contract_kills":contract_kills,
  "clues":found_clues.duplicate(),"bosses":defeated_boss_ids.duplicate(),"quality":graphics_quality,
  "dormant_wildlife":dormant,"pet_stance":companion_stance,
  "world_clock":world_clock,"world_day":world_day,"weather":weather_name,"weather_remaining":weather_remaining,
  "world_events_completed":events_completed.duplicate(),"world_event_active":active_world_event,"world_event_kills":active_event_kills
 }
 var result:String=SAVE_STORE.commit_checkpoint(save,SAVE_STAGING,SAVE_V2,SAVE_BACKUP,kills_total,hero)
 save_health=result
 if result=="OK":
  if not suppress_autosave_message:_show_message("Native checkpoint saved safely.",2)
 elif result=="WRITE FAILED":_show_message("Cannot open checkpoint staging file.",2)
 elif result=="INVALID STAGING":_show_message("Save rejected; previous checkpoint kept.",2)
 elif result=="BACKUP FAILED":_show_message("Could not back up save; old checkpoint kept.",2)
 else:_show_message("Save commit failed. Check backup file.",2)

func _read_save() -> Dictionary:
 var primary:=_decode_save(SAVE_V2)
 if not primary.is_empty():return primary
 var backup:=_decode_save(SAVE_BACKUP)
 if not backup.is_empty():
  save_health="BACKUP RECOVERED"
  return backup
 return {}

func _decode_save(path:String) -> Dictionary:
 return SAVE_STORE.decode(path)

func _load_game() -> void:
 var data:=_read_save()
 if data.is_empty():_show_message("No valid native v2 checkpoint found.",2.5);return
 if not hero_playing or hero!=str(data["hero"]):_start_game(str(data["hero"]))
 _restore_data(data)

func _restore_data(data: Dictionary) -> void:
 hero=str(data.get("hero","z"))
 hero_sprite.texture=NYRA_ART if hero=="nyra" else Z_ART
 hero_sprite.pixel_size=2.65/float(hero_sprite.texture.get_height())
 var coords:Array=data["at"]
 player.position=Vector3(clampf(float(coords[0]),-WORLD_X,WORLD_X),0.85,clampf(float(coords[1]),-WORLD_Z,WORLD_Z))
 gold=clampi(int(data.get("gold",0)),0,99999999)
 var normalized:Dictionary=MONEY_RULES.split_total(MONEY_RULES.total(gold,clampi(int(data.get("silver",0)),0,99),clampi(int(data.get("copper",0)),0,99)))
 gold=int(normalized["gold"])
 silver=int(normalized["silver"])
 copper=int(normalized["copper"])
 _restore_coin_drops(data.get("coin_drops",[]))
 kills_total=clampi(int(data.get("kills",0)),0,999999)
 hero_level=clampi(int(data.get("level",1)),1,100)
 hero_xp_to_next=75+(hero_level-1)*40
 hero_xp=clampi(int(data.get("xp",0)),0,hero_xp_to_next-1)
 hero_perks={}
 var saved_perks:Variant=data.get("hero_perks",{})
 if saved_perks is Dictionary:
  for perk in HERO_PERKS:
   var perk_id:=str(perk["id"])
   hero_perks[perk_id]=clampi(int(saved_perks.get(perk_id,0)),0,3)
 max_hp=100.0+12.0*float(_perk_rank("vigor"))
 hp=clampf(float(data.get("hp",100.0)),1.0,max_hp)
 hero_points=clampi(int(data.get("hero_points",maxi(0,hero_level-1))),0,100)
 combat_combo=0
 combat_combo_until=0.0
 # Optional v11 fields are ignored by v10 saves; older checkpoints remain valid.
 world_clock=clampf(float(data.get("world_clock",240.0)),0.0,DAY_LENGTH_SECONDS-0.01)
 world_day=clampi(int(data.get("world_day",1)),1,99999)
 weather_name=str(data.get("weather","Clear"))
 if not (weather_name in WEATHER_TYPES):weather_name="Clear"
 weather_remaining=clampf(float(data.get("weather_remaining",WEATHER_INTERVAL_SECONDS)),0.5,WEATHER_INTERVAL_SECONDS)
 living_visual_tick=0.0
 events_completed.clear()
 var saved_events:Variant=data.get("world_events_completed",[])
 if saved_events is Array:
  for value in saved_events:
   var idx:=int(value)
   if idx>=0 and idx<world_events.size() and not (idx in events_completed):events_completed.append(idx)
 active_world_event=clampi(int(data.get("world_event_active",-1)),-1,world_events.size()-1)
 if active_world_event in events_completed:active_world_event=-1
 if active_world_event>=0 and str(world_events[active_world_event]["kind"])!="hunt":active_world_event=-1
 active_event_kills=clampi(int(data.get("world_event_kills",0)),0,WORLD_EVENT_GOAL-1) if active_world_event>=0 else 0
 for i in range(world_events.size()):
  var event:Dictionary=world_events[i]
  event["completed"]=i in events_completed
  event["active"]=i==active_world_event
  _refresh_event_label(event)
 _refresh_world_lighting()
 # Restore a checkpoint into the currently playing hero without stale transient combat state.
 dodge_time=0.0
 dodge_cooldown=0.0
 attack_cooldown=0.0
 ability_cooldown=0.0
 hunter_oil_time=0.0
 current_threat=false
 skinning_left=0.0
 skinning_target=null
 shrine_discovery.clear()
 var shrine_values:Variant=data.get("shrines",[])
 if shrine_values is Array:
  for value in shrine_values:
   var index:=int(value)
   if index>=0 and index<6 and not (index in shrine_discovery):shrine_discovery.append(index)
 materials={}
 var incoming_materials:Variant=data.get("materials",{})
 if incoming_materials is Dictionary:
  for k in incoming_materials.keys():
   if materials.size()>=100:break
   var key:=str(k).substr(0,32)
   if key!="":materials[key]=clampi(int(incoming_materials[k]),0,99999)
 skinning_skill=clampi(int(data.get("skinning",1)),1,300)
 crafting_skill=clampi(int(data.get("crafting",1)),1,300)
 water_professions={"Fishing":1,"Herbalism":1,"Mining":1,"Cooking":1,"Alchemy":1,"Smithing":1}
 var stored_skills:Variant=data.get("water_professions",{})
 if stored_skills is Dictionary:
  for skill in water_professions.keys():
   water_professions[skill]=clampi(int(stored_skills.get(skill,1)),1,300)
 water_site_days={}
 var stored_sites:Variant=data.get("water_site_days",{})
 if stored_sites is Dictionary:
  for site_id in stored_sites.keys():
   var id_string:String=str(site_id)
   if id_string.is_valid_int() and int(id_string)>=0 and int(id_string)<36:
    water_site_days[id_string]=clampi(int(stored_sites[site_id]),0,999999)

 tamer_points=clampi(int(data.get("tamer_points",0)),0,999)
 learned_talents={}
 var ranks:Variant=data.get("talents",{})
 if ranks is Dictionary:
  for perk in TALENTS:
   var id:=str(perk["id"])
   learned_talents[id]=clampi(int(ranks.get(id,0)),0,3)
 discovered_species.clear()
 var species:Variant=data.get("species",[])
 if species is Array:
  for item in species:
   var kind:=str(item)
   if BEASTS.has(kind) and not (kind in discovered_species):discovered_species.append(kind)
 companion_stance=str(data.get("pet_stance","Guard"))
 if not (companion_stance in ["Guard","Follow","Passive"]):companion_stance="Guard"
 companion={}
 var pet:Variant=data.get("companion",{})
 if pet is Dictionary and BEASTS.has(str(pet.get("kind",""))):
  companion={"kind":str(pet["kind"]),"variant":str(pet.get("variant","normal")) if str(pet.get("variant","normal")) in ["normal","moon","dusk","elder"] else "normal","hp":clampf(float(pet.get("hp",80)),1.0,300.0),"max":clampf(float(pet.get("max",80)),1.0,300.0)}
 if is_instance_valid(companion_node):companion_node.queue_free()
 companion_node=null
 bag_capacity=INVENTORY_RULES.valid_capacity(int(data.get("bag_capacity",INVENTORY_RULES.BAG_START_CAPACITY)))
 vault_items.clear()
 var saved_vault:Variant=data.get("vault",[])
 if saved_vault is Array:
  for stored in saved_vault:
   if vault_items.size()>=INVENTORY_RULES.VAULT_LIMIT:break
   if stored is Dictionary:vault_items.append(_safe_item(stored))
 gear_items.clear()
 var gear:Variant=data.get("gear",[])
 if gear is Array:
  for item in gear:
   if gear_items.size()>=bag_capacity:break
   if item is Dictionary:gear_items.append(_safe_item(item))
 equipped={}
 var worn:Variant=data.get("equipped",{})
 if worn is Dictionary and not worn.is_empty():equipped=_safe_item(worn)
 for d in gear_drops:
  var node:Node3D=d["node"]
  if is_instance_valid(node):node.queue_free()
 gear_drops.clear()
 var drops:Variant=data.get("drops",[])
 if drops is Array:
  for d in drops:
   if gear_drops.size()>=40:break
   if not (d is Dictionary) or not (d.get("gear",null) is Dictionary):continue
   var at:=Vector3(clampf(float(d.get("x",0)),-WORLD_X,WORLD_X),0.0,clampf(float(d.get("z",0)),-WORLD_Z,WORLD_Z))
   _create_gear_drop(at,"","normal",0,_safe_item(d["gear"]))
 for c in carcasses:
  var node:Node3D=c["node"]
  if is_instance_valid(node):node.queue_free()
 carcasses.clear()
 var corpse_values:Variant=data.get("carcasses",[])
 if corpse_values is Array:
  for c in corpse_values:
   if carcasses.size()>=35:break
   if not (c is Dictionary):continue
   var kind:=str(c.get("kind",""))
   if not BEASTS.has(kind):continue
   var at:=Vector3(clampf(float(c.get("x",0)),-WORLD_X,WORLD_X),0.0,clampf(float(c.get("z",0)),-WORLD_Z,WORLD_Z))
   _make_carcass(at,kind,str(c.get("variant","normal")),clampi(int(c.get("zone",0)),0,5),clampf(float(c.get("remaining",60)),1,90))
 contracts_done.clear()
 var done:Variant=data.get("contracts_done",[])
 if done is Array:
  for value in done:
   var index:=int(value)
   if index>=0 and index<6 and not (index in contracts_done):contracts_done.append(index)
 contract_zone=clampi(int(data.get("contract_zone",-1)),-1,5)
 if contract_zone in contracts_done:contract_zone=-1
 contract_kills=clampi(int(data.get("contract_kills",0)),0,2)
 found_clues.clear()
 var clue_ids:Variant=data.get("clues",[])
 if clue_ids is Array:
  for item in clue_ids:
   var clue_id:=int(item)
   if clue_id>=0 and clue_id<clue_data.size() and not (clue_id in found_clues):found_clues.append(clue_id)
 for clue in clue_data:
  var clue_node:Node3D=clue["node"]
  if is_instance_valid(clue_node):clue_node.visible=senses_on and not (int(clue["id"]) in found_clues)
 # Always reset old live/dead nodes before applying checkpoint state again.
 for creature in beasts:
  _reset_native_beast(creature)
  if int(creature.get("dungeon_site",-1))>=0:_despawn_native_beast(creature,0.0)
 active_rift=-1
 rifts_cleared.clear()
 var saved_rifts:Variant=data.get("rifts",[])
 if saved_rifts is Array:
  for index in saved_rifts:
   var site_id:=int(index)
   if site_id>=0 and site_id<rifts.size() and not (site_id in rifts_cleared):rifts_cleared.append(site_id)
 rifts_completed=rifts_cleared.size()
 for i in range(rifts.size()):
  var entry:Dictionary=rifts[i]
  entry["cleared"]=i in rifts_cleared
  entry["active"]=false
  entry["stage"]=0
  _set_rift_label(entry)
 restored_wildlife=0
 var dormant_saved:Variant=data.get("dormant_wildlife",[])
 if dormant_saved is Array:
  for dormant in dormant_saved:
   if not (dormant is Dictionary):continue
   var spawn_id:=int(dormant.get("id",-1))
   if spawn_id<0 or spawn_id>=beasts.size():continue
   var saved_beast:Dictionary=beasts[spawn_id]
   if bool(saved_beast.get("boss",false)):continue
   var wait:=clampf(float(dormant.get("remaining",0)),0,180)
   if wait<0.5:continue
   _despawn_native_beast(saved_beast,wait)
   restored_wildlife+=1
 defeated_boss_ids.clear()
 var bosses:Variant=data.get("bosses",[])
 if bosses is Array:
  for item in bosses:
   var boss_id:=int(item)
   if boss_id>=0 and boss_id<beasts.size() and not (boss_id in defeated_boss_ids):defeated_boss_ids.append(boss_id)
 for beast in beasts:
  if bool(beast.get("boss",false)) and int(beast.get("boss_id",-1)) in defeated_boss_ids:
   _despawn_native_beast(beast,NORMAL_RESPAWN_SECONDS)
 mounted=false
 mount_combat_time=0
 pet_stunned_until=0.0
 graphics_quality="Smooth" if str(data.get("quality","Full"))=="Smooth" else "Full"
 _apply_graphics_quality(graphics_quality)
 region_index=_region_here()
 _update_region_visibility()
 _show_message("Native v13 checkpoint restored.",3)
 _update_hud()

func _safe_item(source: Dictionary) -> Dictionary:
 return GEAR_CODEC.sanitize(source,AFFIXES)


# v30.6 Money: Diablo-style loot and vendor transactions with WoW-style
# 100c = 1s, 100s = 1g. The legacy gold field remains unchanged.
func _wallet_text()->String:
 return MONEY_RULES.format_wallet(gold,silver,copper)

func _wallet_can_afford(amount:int)->bool:
 return MONEY_RULES.total(gold,silver,copper)>=amount

func _add_coins(amount:int)->void:
 var result:Dictionary=MONEY_RULES.split_total(MONEY_RULES.total(gold,silver,copper)+maxi(0,amount))
 gold=int(result["gold"])
 silver=int(result["silver"])
 copper=int(result["copper"])

func _spend_coins(amount:int)->bool:
 if amount<0 or not _wallet_can_afford(amount):return false
 var result:Dictionary=MONEY_RULES.split_total(MONEY_RULES.total(gold,silver,copper)-amount)
 gold=int(result["gold"])
 silver=int(result["silver"])
 copper=int(result["copper"])
 return true

func _near_outpost_merchant()->bool:
 return hero_playing and not _combat_threat() and is_instance_valid(player) and region_shrines.size()>0 and player.global_position.distance_to(region_shrines[0].global_position)<10.5

func _wallet_action(action:String)->void:
 if action=="wallet_open":return
 if not _near_outpost_merchant():
  _show_message("Trade only at Hearthstone Outpost when safe.",2)
  return
 if action=="wallet_buy_heal" or action=="wallet_buy_oil":
  var cost:int=25000 if action=="wallet_buy_heal" else 40000
  var item:String="Healing Draught" if action=="wallet_buy_heal" else "Hunter's Oil"
  if int(materials.get(item,0))>=INVENTORY_RULES.MATERIAL_LIMIT:
   _show_message("Consumable storage full.",2)
   return
  if not _spend_coins(cost):
   _show_message("Not enough coins for "+item+".",2)
   return
  _gain_material(item,1)
  _show_message("Bought "+item+" for "+MONEY_RULES.format_total(cost),2)
  _save_game()
 elif action.begins_with("wallet_sell:"):
  var idx:int=int(action.get_slice(":",1))
  if idx<0 or idx>=gear_items.size():return
  var item:Dictionary=gear_items[idx]
  if not equipped.is_empty() and equipped==item:
   _show_message("Unequip that item before selling it.",2)
   return
  var price:int=MONEY_RULES.sell_value(item)
  var item_name:String=str(item.get("name","Gear"))
  gear_items.remove_at(idx)
  _add_coins(price)
  _show_message("Sold "+item_name+" for "+MONEY_RULES.format_total(price),2)
  _save_game()

func _create_coin_drop(at:Vector3,amount:int)->void:
 if amount<=0:return
 if coin_drops.size()>=MAX_COIN_DROPS:
  # No lost money when the ground's visual budget is full.
  _add_coins(amount)
  return
 var mesh:=MeshInstance3D.new()
 var disc:=CylinderMesh.new()
 disc.top_radius=0.24
 disc.bottom_radius=0.24
 disc.height=0.09
 disc.radial_segments=12
 mesh.mesh=disc
 var material:=StandardMaterial3D.new()
 material.albedo_color=Color(0.93,0.70,0.23)
 material.metallic=0.65
 material.roughness=0.28
 material.emission_enabled=true
 material.emission=Color(0.35,0.20,0.04)
 material.emission_energy_multiplier=0.5
 mesh.material_override=material
 mesh.position=Vector3(clampf(at.x,-WORLD_X,WORLD_X),0.50,clampf(at.z,-WORLD_Z,WORLD_Z))
 add_child(mesh)
 coin_drops.append({"node":mesh,"amount":amount})

func _clear_coin_drops()->void:
 for drop in coin_drops:
  var node:Node3D=drop.get("node")
  if is_instance_valid(node):node.queue_free()
 coin_drops.clear()
 coin_scan_time=0.0

func _update_coin_drops(delta:float)->void:
 coin_scan_time+=delta
 if coin_scan_time<0.18:return
 coin_scan_time=0.0
 if not is_instance_valid(player):return
 var picked:int=0
 for idx in range(coin_drops.size()-1,-1,-1):
  var drop:Dictionary=coin_drops[idx]
  var node:Node3D=drop["node"]
  if not is_instance_valid(node):
   coin_drops.remove_at(idx)
   continue
  if node.global_position.distance_squared_to(player.global_position)>8.0:continue
  picked+=int(drop["amount"])
  node.queue_free()
  coin_drops.remove_at(idx)
 if picked>0:
  _add_coins(picked)
  _show_message("Coins collected: "+MONEY_RULES.format_total(picked),1.5)
  _save_game()

func _save_coin_drops()->Array:
 var result:Array=[]
 for drop in coin_drops:
  var node:Node3D=drop.get("node")
  if not is_instance_valid(node):continue
  result.append({"x":node.position.x,"z":node.position.z,"amount":int(drop.get("amount",0))})
 return result

func _restore_coin_drops(saved:Variant)->void:
 _clear_coin_drops()
 if not (saved is Array):return
 for data in saved:
  if coin_drops.size()>=MAX_COIN_DROPS:break
  if not (data is Dictionary):continue
  var amount:int=clampi(int(data.get("amount",0)),0,100000)
  if amount<=0:continue
  var pos:=Vector3(clampf(float(data.get("x",0)),-WORLD_X,WORLD_X),0,clampf(float(data.get("z",0)),-WORLD_Z,WORLD_Z))
  _create_coin_drop(pos,amount)
