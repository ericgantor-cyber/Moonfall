extends RefCounted
## Reversible depth simulation; never leave a lake or save underwater height.
const MAX_DEPTH:float=3.3
const DIVE_SPEED:float=1.65
const ASCEND_SPEED:float=2.55
const BUOYANCY_SPEED:float=0.52
const SUBMERGED_DEPTH:float=0.32
const CAVE_MIN_DEPTH:float=1.4
const PLAYER_SURFACE_Y:float=0.85
const CACHE_SLOT_START:int=30
const REGION_COUNT:int=6
static func advance_depth(depth:float,delta:float,can_swim:bool,descending:bool,ascending:bool)->float:
 var step:float=clampf(delta,0.0,0.25)
 if not can_swim:return move_toward(depth,0.0,step*ASCEND_SPEED*2.0)
 if ascending:return move_toward(depth,0.0,step*ASCEND_SPEED)
 if descending:return minf(MAX_DEPTH,depth+step*DIVE_SPEED)
 return move_toward(depth,0.0,step*BUOYANCY_SPEED)
static func truly_submerged(depth:float)->bool:
 return depth>SUBMERGED_DEPTH
static func cache_ready(zone:int,day:int,registry:Dictionary)->bool:
 return zone>=0 and zone<REGION_COUNT and day>0 and int(registry.get(str(CACHE_SLOT_START+zone),-1))!=day
static func in_cache_range(hero_position:Vector3,cave_position:Vector3,depth:float)->bool:
 return depth>=CAVE_MIN_DEPTH and hero_position.distance_squared_to(cave_position)<3.2*3.2
