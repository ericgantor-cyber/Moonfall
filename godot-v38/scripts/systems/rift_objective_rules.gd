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

## All physical chambers are explorable together. The legacy stage value
## remains a completion sentinel and must never gate chamber prompts.
static func chamber_prompt(entry:Dictionary,chamber_stage:int)->String:
 if chamber_stage<1 or chamber_stage>3:return ""
 if not bool(entry.get("active",false)) or bool(entry.get("cleared",false)):return ""
 return objective(chamber_stage,false)
