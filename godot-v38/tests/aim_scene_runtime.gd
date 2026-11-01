extends SceneTree
## Runtime integration smoke test for the active Moonfall scene.
var failures:int=0
func _initialize()->void:call_deferred("_run")
func check(ok:bool,label:String)->void:
 if ok:print("PASS runtime: "+label)
 else:
  failures+=1
  push_error("FAIL runtime: "+label)
func _run()->void:
 var packed:PackedScene=load("res://scenes/main.tscn") as PackedScene
 check(packed!=null,"main scene loads")
 if packed==null:quit(1);return
 var game:Node=packed.instantiate()
 root.add_child(game)
 await process_frame
 var marker:Label3D=game.get("aim_marker") as Label3D
 var toggle:CheckButton=game.get("aim_toggle") as CheckButton
 var options:PanelContainer=game.get("aim_options") as PanelContainer
 check(is_instance_valid(marker),"target marker built")
 check(is_instance_valid(toggle),"assist toggle built")
 check(is_instance_valid(marker) and not marker.visible,"aim marker hidden before gameplay")
 check(is_instance_valid(options) and not options.visible,"aim options hidden before gameplay")
 game.queue_free()
 quit(1 if failures>0 else 0)
