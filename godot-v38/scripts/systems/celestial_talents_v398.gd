extends RefCounted
## Persistent talent rules. No scene assets or character art are touched.
const BRANCHES:PackedStringArray=["weapon","warrior","defense","agility","magic","survival"]
const TALENTS:Dictionary={
 "weapon_technique":{"branch":"weapon","cost":1,"max_rank":3,"requires":""},
 "warrior_power":{"branch":"warrior","cost":1,"max_rank":3,"requires":""},
 "defense_guard":{"branch":"defense","cost":1,"max_rank":3,"requires":""},
 "agility_footwork":{"branch":"agility","cost":1,"max_rank":3,"requires":""},
 "magic_focus":{"branch":"magic","cost":1,"max_rank":3,"requires":""},
 "survival_fortitude":{"branch":"survival","cost":1,"max_rank":3,"requires":""},
 "weapon_mastery":{"branch":"weapon","cost":2,"max_rank":1,"requires":"weapon_technique"},
 "warrior_guardbreak":{"branch":"warrior","cost":2,"max_rank":1,"requires":"warrior_power"},
 "defense_riposte":{"branch":"defense","cost":2,"max_rank":1,"requires":"defense_guard"},
 "agility_airborne":{"branch":"agility","cost":2,"max_rank":1,"requires":"agility_footwork"},
 "magic_mastery":{"branch":"magic","cost":2,"max_rank":1,"requires":"magic_focus"},
 "survival_crafting":{"branch":"survival","cost":2,"max_rank":1,"requires":"survival_fortitude"},
 "z_dragon_transformation":{"branch":"warrior","cost":3,"max_rank":1,"requires":"warrior_guardbreak","hero":"z"},
 "nyra_arcane_mastery":{"branch":"magic","cost":3,"max_rank":1,"requires":"magic_mastery","hero":"nyra"}
}
static func points_for_level(level:int)->int:
 return maxi(0,level-1)*2
static func points_spent(ranks:Dictionary)->int:
 var total:int=0
 for id in ranks:
  if TALENTS.has(id):total+=maxi(0,mini(int(ranks[id]),int(TALENTS[id]["max_rank"])))*int(TALENTS[id]["cost"])
 return total
static func can_unlock(hero:String,level:int,ranks:Dictionary,id:String)->bool:
 if not TALENTS.has(id):return false
 var info:Dictionary=TALENTS[id]
 if info.has("hero") and str(info["hero"])!=hero:return false
 if int(ranks.get(id,0))>=int(info["max_rank"]):return false
 var prerequisite:String=str(info["requires"])
 if prerequisite!="" and int(ranks.get(prerequisite,0))<=0:return false
 return points_for_level(level)-points_spent(ranks)>=int(info["cost"])
static func unlock(hero:String,level:int,ranks:Dictionary,id:String)->Dictionary:
 var result:Dictionary=ranks.duplicate(true)
 if can_unlock(hero,level,result,id):result[id]=int(result.get(id,0))+1
 return result
static func reset()->Dictionary:
 return {}
