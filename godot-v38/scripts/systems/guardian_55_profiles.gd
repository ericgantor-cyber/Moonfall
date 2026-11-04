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
  "species":posmod(family_id*2+rank_id,4),
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

static func guardian_relic(profile_id:int,zone:int,dungeon_id:int,affix:String)->Dictionary:
 var entry:Dictionary=profile(profile_id)
 if entry.is_empty():return {}
 return {"name":str(entry["name"])+" Crest • Region %d Dungeon %d" % [zone+1,dungeon_id+1],
  "rarity":"Legendary","power":mini(250,56+zone*12+int(entry["power_bonus"])),
  "affix":affix,"affix_value":0.12+0.01*float(int(entry["trait"]))}
