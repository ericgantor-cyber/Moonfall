extends RefCounted
## Pure dungeon objective formatting. Existing wave progression and rewards remain authoritative.
static func remaining_enemies(slot_ids:Array, beasts:Array)->int:
 var remaining:int=0
 for raw in slot_ids:
  var id:int=int(raw)
  if id<0 or id>=beasts.size():continue
  var beast:Dictionary=beasts[id]
  if bool(beast.get("alive",false)) and float(beast.get("hp",0.0))>0.0:
   remaining+=1
 return remaining

static func objective(stage:int,remaining:int,cleared:bool)->String:
 if cleared:return "SEALED • LEGENDARY REWARD CLAIMED"
 var count:int=maxi(0,remaining)
 if stage==3:
  return "DEFEAT RIFT CHAMPION • %d REMAINING" % count if count>0 else "GUARDIAN HEART • COMPLETE THE RIFT"
 return "CLEAR CHAMBER %d • %d REMAINING" % [clampi(stage,1,3),count] if count>0 else "CHAMBER CLEAR • PROCEED"

static func stage_active(entry:Dictionary,stage:int)->bool:
 return bool(entry.get("active",false)) and not bool(entry.get("cleared",false)) and int(entry.get("stage",0))==stage
