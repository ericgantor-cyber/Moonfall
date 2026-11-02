extends RefCounted
## Describes existing boss patterns without altering their damage or timings.
static func pattern_name(pattern:int)->String:
 match posmod(pattern,4):
  0:return "FOCUSED STRIKE"
  1:return "WIDE SWEEP"
  2:return "PRECISION STRIKE"
  _:return "DELAYED STRIKE"

static func warning_text(phase:int,pattern:int)->String:
 var cue:String="DODGE AWAY" if posmod(pattern,4)==0 else ("CLEAR THE AREA" if posmod(pattern,4)==1 else ("SIDESTEP" if posmod(pattern,4)==2 else "WATCH THE DELAY"))
 return "GUARDIAN P%d | %s | %s" % [clampi(phase,1,3),pattern_name(pattern),cue]
