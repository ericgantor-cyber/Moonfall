extends RefCounted
## v26 pure progression calculations. This module owns no mutable player state.
static func damage_multiplier(level:int,combo:int,combo_active:bool) -> float:
 var multiplier:=1.0+0.035*float(level-1)
 if combo_active:multiplier+=0.02*float(mini(combo,10))
 return multiplier

static func recommended_power(zone:int,regions:Array) -> int:
 var index:int=clampi(zone,0,regions.size()-1)
 return 12+int(regions[index]["tier"]-1)*32+index*6

static func power_modifier(current:int,recommended:int) -> float:
 return clampf(1.0+float(current-recommended)*0.004,0.70,1.22)
