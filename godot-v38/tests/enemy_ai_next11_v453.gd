extends SceneTree
const R = preload("res://scripts/systems/enemy_combat_rules.gd")
var failures: int = 0
func ck(ok: bool, label: String) -> void:
 if ok: print("PASS next11: " + label)
 else:
  failures += 1
  push_error("FAIL next11: " + label)
func _initialize() -> void:
 call_deferred("run")
func run() -> void:
 ck(R.preferred_spacing("panther") == 2.6, "panther gets more room to flank")
 ck(R.preferred_spacing("serpent") == 2.3, "serpent keeps tighter flank")
 ck(not R.group_attack_ready(1.0e13, 1), "invalid long combat clock rejected")
 ck(not R.tactical_attack_ready("wolf", 1.0e13, 1), "invalid long tactical clock rejected")
 ck(R.attack_timer_after_frame(NAN, 0.016) > 0.0, "corrupt cooldown cannot immediately attack")
 ck(not R.should_circle("wolf", "strafe", 5.8), "wolves stop distant orbiting")
 ck(R.remember_target(true, true, false, -1.0, Vector3(1,0,0), Vector3.ZERO, 0.0)["until"] == 0.0, "negative memory time rejected")
 ck(R.investigate_velocity(true, false, false, -1.0, 1.0, Vector3.ZERO, Vector3(3,0,0), 8.0) == Vector3.ZERO, "negative pursuit time rejected")
 var speed: Vector3 = R.investigate_velocity(true, false, false, 10.0, 12.0, Vector3.ZERO, Vector3(100,0,0), 100.0)
 ck(speed.length() <= 11.71, "enemy investigation speed is capped")
 var near: Vector3 = R.investigate_velocity(true, false, false, 10.0, 12.0, Vector3.ZERO, Vector3(1.5,0,0), 10.0)
 ck(near.length() < speed.length(), "enemy slows near search point")
 ck(R.search_arrived(Vector3.ZERO, Vector3(1,0,0)), "one meter arrival counted")
 ck(not R.search_pause_active(false, -1.0, 0.5), "negative search pause clock rejected")
 ck(R.search_pause_expiry(-1.0) == 0.0, "negative search expiry rejected")
 ck(load("res://scenes/main.tscn") != null, "main scene resource exists")
 print("next11 failures: %d" % failures)
 quit(1 if failures else 0)
