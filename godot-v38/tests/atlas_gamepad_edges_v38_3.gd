extends SceneTree
## Run: godot --headless --path godot-v38 --script res://tests/atlas_gamepad_edges_v38_3.gd
var failures:int=0

func check(ok:bool,description:String)->void:
 if ok:print("PASS atlas gamepad: "+description)
 else:
  failures+=1
  push_error("FAIL atlas gamepad: "+description)

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var script:GDScript=load("res://scripts/native_v38_3.gd") as GDScript
 check(script!=null,"atlas gamepad script loads")
 if script==null:
  quit(1)
  return
 var source:String=script.source_code
 check("var pad_atlas_device:int=-1" in source,"controller identity is tracked")
 check("if device!=pad_atlas_device:" in source,"controller handoff resets state")
 check("pad_atlas_previous.clear()" in source,"held edges clear on device loss")
 check("var mark:bool=_pad_pressed" in source and "var close:bool=_pad_pressed" in source,"button edges sampled while map closed")
 check("if close:" in source and "pad_atlas_cursor=Vector2.ZERO" in source,"closing map clears virtual cursor")
 # Exercise the edge state machine without requiring a physical controller.
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 check(bool(game.call("_pad_edge",99,"mark",true)),"initial button press fires")
 check(not bool(game.call("_pad_edge",99,"mark",true)),"held button does not repeat")
 check(not bool(game.call("_pad_edge",99,"mark",false)),"button release is silent")
 check(bool(game.call("_pad_edge",99,"mark",true)),"next press after release fires")
 check(bool(game.call("_pad_edge",100,"mark",true)),"controller identities do not share button state")
 game.queue_free()
 quit(1 if failures>0 else 0)
