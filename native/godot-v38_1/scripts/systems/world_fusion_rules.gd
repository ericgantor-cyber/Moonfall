extends RefCounted
## Shared guidance and bounded existing-material rewards in one continuous world.
## No extra currency, save schema, modes, or per-frame computations.
const LOOP:String="Explore -> world event -> gather & craft -> improve gear -> rift/guardian -> harder region."

static func next_objective(regions:Array,recommended:Array,done:Array,remaining_rifts:int,power:int)->Dictionary:
 var best:int=-1
 var best_gap:int=2147483647
 for i in range(regions.size()):
  if i in done:continue
  var required:int=int(recommended[i]) if i<recommended.size() else 0
  var gap:int=maxi(0,required-power)
  if gap<best_gap:
   best=i
   best_gap=gap
 if best>=0:
  return {"name":str(regions[best].get("name","Unknown")),"kind":"World event","zone":best,"power_gap":best_gap}
 if remaining_rifts>0:
  return {"name":"An unsealed world rift","kind":"Rift","zone":-1,"power_gap":0}
 return {"name":"Remaining guardians, crafting and exploration","kind":"Adventure","zone":-1,"power_gap":0}

static func event_crafting_bonus(zone:int,professions:Dictionary)->Dictionary:
 var best:int=1
 for name in ["Fishing","Herbalism","Mining","Cooking","Alchemy","Smithing"]:
  best=maxi(best,int(professions.get(name,1)))
 var ingredient:String="herb" if zone<=1 else "Moon Sigil" if zone<=3 else "Dusk Cinder"
 return {"name":ingredient,"amount":2 if best>=25 else 1}

static func guidance(objective:Dictionary,power:int,recommended_power:int)->String:
 var gap:int=maxi(0,recommended_power-power)
 var power_advice:String="Power meets this region's recommendation." if gap==0 else str(gap)+" power below the regional recommendation. Craft and equip gear before difficult fights; no region is locked."
 return "[b]ONE CONNECTED REALM • YOUR NEXT STEP[/b]\n"+power_advice+"\nSuggested objective: "+str(objective.get("name","Explore"))+" ("+str(objective.get("kind","Adventure"))+").\n\n"+LOOP+"\nCombat builds XP and combos; events give existing crafting ingredients; professions improve gear for stronger rifts and guardians."
