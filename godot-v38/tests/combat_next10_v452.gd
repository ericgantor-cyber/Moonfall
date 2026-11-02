extends SceneTree
## Ten focused regression checks for Moonfall's existing melee AI rules.
## Run: godot --headless --path godot-v38 --script res://tests/combat_next10_v452.gd
const Rules = preload("res://scripts/systems/enemy_combat_rules.gd")
var failures: int = 0

func check(condition: bool, label: String) -> void:
 if condition:
  print("PASS next10: " + label)
 else:
  failures += 1
  push_error("FAIL next10: " + label)

func _initialize() -> void:
 call_deferred("run")

func run() -> void:
 # 1. Wolves remain melee ambushers, not ranged/kiting enemies.
 check(Rules.role_for("wolf") == "ambusher", "wolf melee ambusher role")
 # 2. Serpents retain the same melee ambusher intent.
 check(Rules.role_for("serpent") == "ambusher", "serpent melee ambusher role")
 # 3. Heavier boars keep their bruiser role.
 check(Rules.role_for("boar") == "bruiser", "boar bruiser role")
 # 4. Missing line of sight never permits a strike.
 check(not Rules.can_attack(1.0, true, false), "no attacks through walls")
 # 5. Out-of-range targets cannot take a melee hit.
 check(not Rules.can_attack(2.0, true, true), "melee range respected")
 # 6. Invalid distances cannot produce attacks.
 check(not Rules.can_attack(-1.0, true, true) and not Rules.can_attack(NAN, true, true), "bad melee distances rejected")
 # 7. An invalid attack timer must not become instantly ready.
 check(not Rules.ready_to_strike(NAN) and not Rules.ready_to_strike(INF), "invalid attack timers rejected")
 # 8. Large frame deltas must not skip an entire cooldown.
 check(is_equal_approx(Rules.attack_timer_after_frame(2.0, 60.0), 1.9), "cooldown frame delta capped")
 # 9. Non-hostiles and bosses must not inherit ordinary-fauna pursuit.
 check(Rules.investigate_velocity(false, false, false, 10.0, 12.0, Vector3.ZERO, Vector3(4, 0, 0), 10.0) == Vector3.ZERO and Rules.investigate_velocity(true, false, true, 10.0, 12.0, Vector3.ZERO, Vector3(4, 0, 0), 10.0) == Vector3.ZERO, "non-hostile and boss pursuit isolated")
 # 10. Lost sight should preserve last seen target, not current hidden hero coordinates.
 var memory: Dictionary = Rules.remember_target(false, true, false, 11.0, Vector3(99, 0, 0), Vector3(5, 0, 0), 12.0)
 check(memory["position"] == Vector3(5, 0, 0), "no omniscient through-wall tracking")
 check(load("res://scenes/main.tscn") != null, "full scene resource loads")
 print("Next10 combat tests finished: %d failures" % failures)
 quit(1 if failures else 0)
