extends SceneTree
const MORALITY = preload("res://scripts/systems/morality_reputation.gd")
var failures:int = 0

func check(condition:bool, label:String) -> void:
 if condition:
  print("PASS morality: " + label)
 else:
  failures += 1
  push_error("FAIL morality: " + label)

func _initialize() -> void:
 var fresh:Dictionary = MORALITY.new_state()
 check(MORALITY.alignment(fresh["morality"]) == "Neutral", "new hero starts neutral")
 var good:Dictionary = MORALITY.apply_choice(fresh, "save-village", 40, {"Villagers": 70, "Outlaws": -30})
 check(MORALITY.alignment(good["morality"]) == "Good", "good choice changes morality")
 check(MORALITY.reaction(good, "Villagers")["discount_percent"] == 10, "friends unlock merchant discount")
 check(MORALITY.reaction(good, "Outlaws")["hostile"], "factions remember choices")
 var repeated:Dictionary = MORALITY.apply_choice(good, "save-village", 40, {"Villagers": 70})
 check(repeated == good, "quest decision cannot be rewarded twice")
 check(int(fresh["morality"]) == 0, "original save object is not mutated")
 var evil:Dictionary = MORALITY.apply_choice(good, "betray-village", -200, {"Villagers": -200})
 check(evil["morality"] == -100 and MORALITY.alignment(evil["morality"]) == "Evil", "evil outcome is clamped")
 check(not MORALITY.reaction(evil, "Villagers")["will_trade"], "hunted hero loses trade access")
 check(MORALITY.companion_loyalty(good, "Good") > MORALITY.companion_loyalty(evil, "Good"), "companions respond to morality")
 check(MORALITY.weapon_affinity(evil) == "Evil", "weapon affinity follows alignment")
 check(MORALITY.normalize({"morality": 900})["morality"] == 100, "invalid save scores clamped")
 check(MORALITY.quest_available(good, "Villagers", 60, "Good"), "good quest can unlock through choices")
 check(not MORALITY.quest_available(evil, "Villagers", 0, "Good"), "evil reputation locks good-only quest")
 check(MORALITY.weapon_evolution(good, 50, 15)["rank"] == 3, "weapons evolve with accomplishments")
 check(MORALITY.weapon_evolution(good, 10000, 10000)["rank"] == 5, "weapon ranks are capped")
 check(MORALITY.sanctuary_trophy_unlocked(good, "save-village"), "quest choice unlocks sanctuary trophy")
 check(not MORALITY.sanctuary_trophy_unlocked(fresh, "save-village"), "trophies require earned decisions")
 check(MORALITY.settlement_status(good, "Villagers")["citizens_help"], "friendly settlements help")
 check(MORALITY.settlement_status(evil, "Villagers")["guards_hostile"], "hostile settlements react")
 print("MORALITY TESTS: " + ("PASS" if failures == 0 else "FAIL"))
 quit(0 if failures == 0 else 1)
