extends RefCounted
## Classless, data-only good/evil and local reputation rules.
## No character artwork, modes, or scenes are modified.

const MIN_SCORE := -100
const MAX_SCORE := 100
const DEFAULT_FACTIONS := ["Villagers", "Merchants", "Wardens", "Outlaws"]

static func new_state() -> Dictionary:
 var factions:Dictionary = {}
 for name in DEFAULT_FACTIONS:
  factions[name] = 0
 return {"morality": 0, "factions": factions, "choices": {}}

static func normalize(raw:Variant) -> Dictionary:
 var state:Dictionary = new_state()
 if not raw is Dictionary:
  return state
 state["morality"] = clampi(int(raw.get("morality", 0)), MIN_SCORE, MAX_SCORE)
 var original:Variant = raw.get("factions", {})
 if original is Dictionary:
  for name in original:
   if name is String and not str(name).is_empty():
    state["factions"][name] = clampi(int(original[name]), MIN_SCORE, MAX_SCORE)
 var choices:Variant = raw.get("choices", {})
 if choices is Dictionary:
  for id in choices:
   if id is String and not str(id).is_empty() and bool(choices[id]):
    state["choices"][id] = true
 return state

static func alignment(score:int) -> String:
 if score >= 30:
  return "Good"
 if score <= -30:
  return "Evil"
 return "Neutral"

static func standing(score:int) -> String:
 if score >= 60:
  return "Revered"
 if score >= 20:
  return "Friendly"
 if score <= -60:
  return "Hunted"
 if score <= -20:
  return "Hostile"
 return "Neutral"

static func apply_choice(raw:Variant, choice_id:String, morality_delta:int, faction_deltas:Dictionary = {}) -> Dictionary:
 var state:Dictionary = normalize(raw)
 # Idempotence prevents reloading a saved quest from farming reputation.
 if choice_id.is_empty() or state["choices"].has(choice_id):
  return state
 state["choices"][choice_id] = true
 state["morality"] = clampi(int(state["morality"]) + morality_delta, MIN_SCORE, MAX_SCORE)
 for faction in faction_deltas:
  if not faction is String or str(faction).is_empty():
   continue
  var reputations:Dictionary = state["factions"]
  reputations[faction] = clampi(int(reputations.get(faction, 0)) + int(faction_deltas[faction]), MIN_SCORE, MAX_SCORE)
 return state

static func reaction(raw:Variant, faction:String) -> Dictionary:
 var state:Dictionary = normalize(raw)
 var score:int = int(state["factions"].get(faction, 0))
 return {"standing": standing(score), "will_trade": score > -60, "discount_percent": 10 if score >= 60 else 5 if score >= 20 else 0, "hostile": score <= -20}

static func companion_loyalty(raw:Variant, companion_ethic:String) -> int:
 var morality:int = int(normalize(raw)["morality"])
 var preference:int = -morality if companion_ethic == "Evil" else morality if companion_ethic == "Good" else 0
 return clampi(50 + int(round(float(preference) * 0.4)), 0, 100)

static func weapon_affinity(raw:Variant) -> String:
 return alignment(int(normalize(raw)["morality"]))
