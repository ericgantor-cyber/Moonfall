extends RefCounted
## Health-based commander aggression without faster hazard warning or extra enemies.
const PHASE_TWO_EXTRA:float=0.18
const PHASE_THREE_EXTRA:float=0.34
const MAX_EXTRA:float=0.34

static func rest_acceleration(phase:int)->float:
 if phase>=3:return PHASE_THREE_EXTRA
 if phase==2:return PHASE_TWO_EXTRA
 return 0.0

static func recovered_during_hit(old_feather:bool,new_feather:bool,damage_already_counted:bool)->bool:
 # A Phoenix rescue must break a flawless victory even if health increased.
 return not old_feather and new_feather and not damage_already_counted
