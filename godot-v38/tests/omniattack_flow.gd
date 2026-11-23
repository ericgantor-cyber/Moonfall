extends SceneTree
# Engine-level regression for the four-angle shuffle bag. No scene, art or save
# data is needed to test this transient combat policy.
var failures:int=0

func _initialize()->void:
 call_deferred("_run")

func check(ok:bool,description:String)->void:
 if ok:
  print("PASS omniattack: "+description)
 else:
  failures+=1
  push_error("FAIL omniattack: "+description)

func _run()->void:
 var script:GDScript=load("res://scripts/native_v38_0.gd") as GDScript
 if script==null:
  check(false,"combat script loads")
  quit(1)
  return
 var fighter:Node=script.new()
 if fighter==null:
  check(false,"combat instance constructs")
  quit(1)
  return
 var all_legal:bool=true
 var no_back_to_back:bool=true
 var complete_cycles:bool=true
 var previous:int=-1
 for cycle in range(32):
  var seen:Dictionary={}
  for swing in range(4):
   var angle:int=int(fighter.call("_v394_next_auto_angle"))
   all_legal=all_legal and angle>=0 and angle<4
   no_back_to_back=no_back_to_back and angle!=previous
   seen[angle]=true
   previous=angle
  complete_cycles=complete_cycles and seen.size()==4
 check(all_legal,"every angle is within high/right/low/left")
 check(no_back_to_back,"128 swings never repeat consecutively, even between cycles")
 check(complete_cycles,"all four angles occur once in each shuffled cycle")
 fighter.call("_v394_reset_omniattack")
 check(int(fighter.get("v394_omni_last"))==-1,"restart clears previous-angle history")
 check((fighter.get("v394_omni_deck") as Array).is_empty(),"restart clears unused angles")
 check(str(fighter.get("v394_strike_direction_label"))=="READY","restart readout returns to READY")
 fighter.free()
 quit(1 if failures>0 else 0)
