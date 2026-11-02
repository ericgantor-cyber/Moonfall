extends RefCounted
## Exploration-first dungeon prompts, without wave counts or kill counters.
## The existing dungeon state machine remains unchanged.
static func objective(stage:int,cleared:bool)->String:
 if cleared:return "RIFT SEALED • REWARD UNLOCKED"
 match clampi(stage,1,3):
  1:return "OUTER SANCTUM • EXPLORE THE CHAMBER"
  2:return "MOONBOUND PASSAGE • FIND THE WAY FORWARD"
  3:return "GUARDIAN HEART • FACE THE RIFT CHAMPION"
 return "EXPLORE THE RIFT"

static func stage_active(entry:Dictionary,stage:int)->bool:
 return bool(entry.get("active",false)) and not bool(entry.get("cleared",false)) and int(entry.get("stage",0))==stage
