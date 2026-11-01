extends SceneTree
var failures:int=0
func verify(condition:bool,description:String)->void:
 if condition:print("PASS safety: "+description)
 else:
  failures+=1
  push_error("FAIL safety: "+description)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 verify(scene!=null,"full scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 var body:CharacterBody3D=CharacterBody3D.new()
 game.add_child(body)
 var boss:Dictionary={"boss_id":13579,"body":body,"boss_phase":2,"boss_pattern":2}
 game.call("_set_boss_telegraph",boss,true)
 var warning:Label3D=game.get("boss_warning_labels").get(13579) as Label3D
 verify(is_instance_valid(warning) and warning.visible,"warning displayed")
 boss["body"]=null
 game.call("_set_boss_telegraph",boss,true)
 verify(is_instance_valid(warning) and not warning.visible,"invalid boss body hides warning")
 game.call("_set_boss_telegraph",boss,false)
 verify(is_instance_valid(warning) and not warning.visible,"inactive warning remains hidden")
 game.queue_free()
 quit(1 if failures>0 else 0)
