extends SceneTree

# Headless release gate: basic interactions, controls, and spellbook invariants.
func _initialize() -> void:
    call_deferred("_run_tests")

func _run_tests() -> void:
    var scene: PackedScene = load("res://main.tscn")
    if scene == null:
        _fail("Unable to load main scene")
        return
    var game: Node3D = scene.instantiate()
    root.add_child(game)
    await process_frame
    if game.bindings.get("jump") != KEY_SPACE or game.bindings.get("dodge") != KEY_Q:
        _fail("Space jump / Q dodge defaults changed")
        return
    if game.bindings.has("slot_1"):
        _fail("Removed WoW action-bar bindings unexpectedly returned")
        return
    if game.spells.size() != 6 or game.spell_menu == null:
        _fail("Spellbook missing schools or menu")
        return
    game._choose("Z")
    if not game.started or game.hero != "Z":
        _fail("Hero start failed")
        return
    game.spell_menu_open = true
    game.spell_menu.visible = true
    game._render_spell_menu()
    game._equip_spell_hand(4, 0)
    game._equip_spell_hand(5, 1)
    if game.left_spell != 4 or game.right_spell != 5:
        _fail("Independent hand equipping failed")
        return
    game.spell_menu_open = false
    game.spell_menu.visible = false
    game.hp = 50.0
    game.school_cooldowns[3] = 0.0
    var prior_mana: float = game.mana
    if not game._cast_spell(3, 1.0):
        _fail("Restoration cast failed")
        return
    if game.hp <= 50.0 or game.mana >= prior_mana:
        _fail("Restoration did not heal or consume mana")
        return
    if game._cast_spell(3, 1.0):
        _fail("Spell cooldown bypass")
        return
    game.gold = 50
    game.moon_shards = 3
    var old_max_hp: float = game.max_hp
    game._craft_upgrade(false)
    if game.max_hp != old_max_hp + 10.0 or game.gold != 0 or game.moon_shards != 0:
        _fail("Vitality recipe did not grant stats and consume currency")
        return
    game._craft_upgrade(false)
    if game.max_hp != old_max_hp + 10.0:
        _fail("Crafting allowed without resources")
        return
    game.gold = 80
    game.moon_shards = 5
    game.inventory.append({"name": "Test Blade", "rarity": "Common", "slot": "Weapon", "power": 3})
    game._equip_item(game.inventory.size() - 1)
    var old_damage: float = game.damage
    game._forge_weapon()
    if game.damage != old_damage + 3.0 or game.gold != 0 or game.moon_shards != 0:
        _fail("Weapon forge failed to apply upgrade or consume resources")
        return
    game._forge_weapon()
    if game.damage != old_damage + 3.0:
        _fail("Forge allowed without enough resources")
        return
    game.gold = 70
    game.moon_shards = 4
    game.inventory.append({"name": "Test Armor", "rarity": "Common", "slot": "Armor", "power": 5})
    game._equip_item(game.inventory.size() - 1)
    game._forge_armor()
    if game.armor != 7.0 or game.gold != 0 or game.moon_shards != 0:
        _fail("Armor forging did not upgrade armor and spend resources")
        return
    game._forge_armor()
    if game.armor != 7.0:
        _fail("Armor forging bypassed resource requirements")
        return
    print("MOONFALL3D_SMOKE_TEST_PASS")
    quit(0)

func _fail(message: String) -> void:
    push_error("MOONFALL3D_SMOKE_TEST_FAIL: " + message)
    quit(1)
