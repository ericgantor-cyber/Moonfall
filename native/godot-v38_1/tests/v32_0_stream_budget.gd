extends SceneTree
const RULES=preload("res://scripts/systems/world_reborn_rules.gd")
var failures:int=0
func check(ok:bool,message:String)->void:
 if ok:print("PASS v32 budget "+message)
 else:
  failures+=1
  push_error("FAIL v32 budget "+message)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 check(RULES.visible_at(0.0,false,false),"Near sector is visible")
 check(not RULES.visible_at(3000.0,false,false),"Newly approaching sector starts culled")
 check(RULES.visible_at(3000.0,true,false),"Hysteresis keeps active sector visible")
 check(not RULES.visible_at(4000.0,true,false),"Beyond far distance culls")
 check(not RULES.visible_at(0.0,true,true),"Low graphics disables optional detail")
 check(RULES.contract_available(1,[],-1),"Fresh regional NPC contract")
 check(not RULES.contract_available(1,[1],-1),"Completed contract cannot be repeated")
 check(not RULES.contract_available(1,[],0),"Only one active quest")
 check(RULES.standing([],0)=="Unknown Traveler","Legacy saves have starter standing")
 check(RULES.standing([0,1,2,3],3)=="Champion of the Six Realms","Saved progress yields reputation")
 print("MOONFALL v32 STREAM BUDGET PASS" if failures==0 else "MOONFALL v32 STREAM BUDGET FAIL")
 quit(0 if failures==0 else 1)
