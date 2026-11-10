extends SceneTree
const RING=preload("res://scripts/systems/key_ring.gd")
const GEAR=preload("res://scripts/systems/equipment_inventory.gd")
var failures:int=0

func expect(ok:bool,description:String)->void:
 if not ok:
  failures+=1
  push_error("Key ring: "+description)

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var state:Dictionary=RING.new_state()
 expect(state["keys"].is_empty(),"starts empty")
 expect(not RING.add_key(state,"unrecognized"),"reject unknown keys")
 expect(RING.add_key(state,"wardens_crest"),"collect first key")
 expect(not RING.add_key(state,"wardens_crest"),"deduplicate collected keys")
 expect(RING.add_key(state,"fallen_gate_key"),"automatically grows")
 expect(RING.has_key(state,"fallen_gate_key"),"key access")
 expect(RING.normalize({"keys":["outlaw_seal","outlaw_seal","invalid",9]})["keys"]==["outlaw_seal"],"untrusted save filtered")
 expect(RING.normalize(state)["keys"]==state["keys"],"save roundtrip")
 expect(RING.display_lines(state).contains("Fallen Gate Key"),"key ring display")
 expect(GEAR.gold_for_kill(0)==0 and GEAR.gold_for_kill(1)==3,"gold is only paid for confirmed kills")
 expect(GEAR.gold_for_kill(1000)>GEAR.gold_for_kill(240),"enemy gold scales without previous payout ceiling")
 var gear:Dictionary=GEAR.new_state()
 expect(GEAR.add_gold(gear,12)==12 and gear["gold"]==12,"auto-collected gold added")
 expect(GEAR.add_gold(gear,0)==0 and GEAR.add_gold(gear,-10)==0 and gear["gold"]==12,"invalid gold grants rejected")
 expect(GEAR.add_gold(gear,99999999)==99999999 and gear["gold"]==100000011,"gold exceeds former 99,999,999 cap")
 expect(GEAR.normalize(gear)["gold"]==100000011,"large gold balance survives save normalization")
 gear["gold"]=GEAR.MAX_GOLD-2
 expect(GEAR.add_gold(gear,10)==2 and gear["gold"]==GEAR.MAX_GOLD,"gold overflow guarded at engine integer boundary")
 expect(GEAR.add_gold(gear,7)==0,"no integer wrap when at engine boundary")
 expect(GEAR.normalize({"gold":-30})["gold"]==0,"negative saved balance normalized")
 expect(GEAR.normalize(gear)["gold"]==GEAR.MAX_GOLD,"max representable gold survives save normalization")
 print("KEY RING "+("PASS" if failures==0 else "FAIL"))
 quit(0 if failures==0 else 1)
