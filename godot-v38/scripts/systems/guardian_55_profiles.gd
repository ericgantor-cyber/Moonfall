extends RefCounted
## 55 distinct dungeon guardian profiles from 11 families x 5 ranks.
## All retain the four native boss species and four trait combat attacks.
## Content-only definitions: no extra bosses, timers, models or wave modes.
const FAMILIES:Array[String]=[
 "Moonfang","Ashen Crown","Venomveil","Duskthorn","Frostbound",
 "Sunscorch","Stormcaller","Ironhide","Hollowroot","Bloodmoon","Starfall"
]
const RANKS:Array[String]=["Sentinel","Reaver","Warden","Champion","Sovereign"]
const PROFILE_COUNT:int=55

static func valid_profile(profile_id:int)->bool:
 return profile_id>=0 and profile_id<PROFILE_COUNT

static func profile(profile_id:int)->Dictionary:
 if not valid_profile(profile_id):return {}
 var family_id:int=int(profile_id/RANKS.size())
 var rank_id:int=profile_id%RANKS.size()
 var trait_id:int=posmod(family_id+rank_id,4)
 return {
  "id":profile_id,
  "name":FAMILIES[family_id]+" "+RANKS[rank_id],
  # Rotate within the 55-profile pool so all native species can roll every trait.
  "species":posmod(profile_id+int(family_id/2)+1,4),
  "trait":trait_id,
  "health_scale":1.0+0.045*float(rank_id)+0.015*float(family_id%4),
  "attack_scale":1.0+0.06*float(rank_id)+0.015*float(family_id%3),
  "cooldown_scale":1.0-0.035*float(rank_id),
  "power_bonus":rank_id*5+family_id%5
 }

static func suitable_profiles(species_index:int)->Array[int]:
 var choices:Array[int]=[]
 for profile_id in range(PROFILE_COUNT):
  if int(profile(profile_id).get("species",-1))==species_index:choices.append(profile_id)
 return choices


# Shared assignment rule: never emit a partially rolled or duplicate roster.
# Uses the game's existing RNG so save/replay behavior remains deterministic.
static func roll_unique_profiles(species_rolls:Array[int], randomizer:RandomNumberGenerator)->Array[int]:
 var assigned:Array[int]=[]
 if randomizer==null or species_rolls.is_empty() or species_rolls.size()>PROFILE_COUNT:return assigned
 var taken:Dictionary={}
 for species_id in species_rolls:
  if species_id<0 or species_id>=4:
   assigned.clear()
   return assigned
  var eligible:Array[int]=[]
  for candidate_id in suitable_profiles(species_id):
   if not taken.has(candidate_id):eligible.append(candidate_id)
  if eligible.is_empty():
   assigned.clear()
   return assigned
  var choice:int=eligible[randomizer.randi_range(0,eligible.size()-1)]
  assigned.append(choice)
  taken[choice]=true
 return assigned

static func guardian_relic(profile_id:int,zone:int,dungeon_id:int,affix:String)->Dictionary:
 var entry:Dictionary=profile(profile_id)
 # Invalid restored region/dungeon IDs must not create negative-power or
 # impossible legendary rewards. Valid 6x5 adventures remain unchanged.
 if entry.is_empty() or zone<0 or zone>=6 or dungeon_id<0 or dungeon_id>=5:return {}
 return {"name":str(entry["name"])+" Crest • Region %d Dungeon %d" % [zone+1,dungeon_id+1],
  "rarity":"Legendary","power":mini(250,56+zone*12+int(entry["power_bonus"])),
  "affix":affix,"affix_value":0.12+0.01*float(int(entry["trait"]))}
