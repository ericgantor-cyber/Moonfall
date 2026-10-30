extends SceneTree
const GEN:=preload("res://scripts/systems/procedural_variants_v382.gd")
var failures:int=0
func check(ok:bool,label:String)->void:
 if ok:print("PASS procedural: "+label)
 else:
  failures+=1
  push_error("FAIL procedural: "+label)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 check(GEN.FAMILIES.size()==11 and GEN.ROLES.size()==11,"family and combat role catalogs bounded")
 check(GEN.MODIFIERS.size()==11 and GEN.ELEMENTS.size()==11,"modifier and element catalogs bounded")
 check(GEN.ATTACKS.size()==12 and GEN.ARENAS.size()==11,"attack and arena catalogs bounded")
 var a:Dictionary=GEN.encounter(142,12)
 var b:Dictionary=GEN.encounter(142,12)
 check(a==b,"identical seed produces identical encounter")
 check(a["enemy"]["level"]==12 and a["boss"]["boss"],"enemy and boss metadata available")
 check(a["boss"]["phase_count"]>=2 and a["boss"]["phase_count"]<=4,"boss phases bounded")
 check(int(a["loot"]["power"])<=250,"loot power follows equipment limit")
 for seed in [-214748,0,1,42,999999]:
  for level in [-10,1,100,10000]:
   var e:Dictionary=GEN.enemy(seed,level)
   var l:Dictionary=GEN.loot(seed,level)
   check(int(e["level"])>=1 and int(e["level"])<=1000,"enemy level clamped "+str(seed)+":"+str(level))
   check(float(e["hp_scale"])<=20.0 and float(e["damage_scale"])<=12.0,"enemy stats bounded "+str(seed)+":"+str(level))
   check(int(l["power"])>=0 and int(l["power"])<=250,"loot bounded "+str(seed)+":"+str(level))
 print("MOONFALL v38.2 PROCEDURAL PASS" if failures==0 else "MOONFALL v38.2 PROCEDURAL FAIL")
 quit(0 if failures==0 else 1)
