extends RefCounted
## Describes existing boss patterns without altering their damage or timings.
static func pattern_name(pattern:int)->String:
 match posmod(pattern,4):
  0:return "FOCUSED STRIKE"
  1:return "WIDE SWEEP"
  2:return "PRECISION STRIKE"
  _:return "DELAYED STRIKE"

static func dodge_cue(pattern:int)->String:
 match posmod(pattern,4):
  0:return "DODGE AWAY"
  1:return "CLEAR THE AREA"
  2:return "SIDESTEP"
  _:return "WATCH THE DELAY"

static func warning_text(phase:int,pattern:int)->String:
 var cue:String=dodge_cue(pattern)
 return "GUARDIAN P%d | %s | %s" % [clampi(phase,1,3),pattern_name(pattern),cue]
