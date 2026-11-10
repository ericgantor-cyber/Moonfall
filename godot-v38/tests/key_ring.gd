extends SceneTree
const RING=preload("res://scripts/systems/key_ring.gd")
const GEAR=preload("res://scripts/systems/equipment_inventory.gd")
var failures:int=0

func expect(ok:bool,description:String)->void:
 if not ok:
  failures+=1
  push_error("Key ring: "+description)

func _initialize()->void:
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
 var gear:Dictionary=GEAR.new_state()
 expect(GEAR.add_gold(gear,12)==12 and gear["gold"]==12,"auto-collected gold added")
 expect(GEAR.add_gold(gear,0)==0 and GEAR.add_gold(gear,-10)==0 and gear["gold"]==12,"invalid gold grants rejected")
 expect(GEAR.add_gold(gear,99999999)==99999987 and gear["gold"]==99999999,"currency cap enforced")
 expect(GEAR.add_gold(gear,7)==0,"full purse cannot overflow")
 expect(GEAR.normalize(gear)["gold"]==99999999,"gold survives save normalization")
 print("KEY RING "+("PASS" if failures==0 else "FAIL"))
 quit(0 if failures==0 else 1)
