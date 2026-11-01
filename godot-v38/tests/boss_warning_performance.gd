extends SceneTree
var failures:int=0
func check(ok:bool,message:String)->void:
 if ok:print("PASS boss perf: "+message)
 else:
  failures+=1
  push_error("FAIL boss perf: "+message)
func _initialize()->void:call_deferred("run")
func run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"full RPG scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 var body:CharacterBody3D=CharacterBody3D.new()
 game.add_child(body)
 var boss:Dictionary={"boss_id":98713,"body":body,"boss_phase":2,"boss_pattern":2}
 game.call("_set_boss_telegraph",boss,true)
 var label:Label3D=game.get("boss_warning_labels").get(98713) as Label3D
 check(is_instance_valid(label) and label.visible,"warning is visible")
 check(is_instance_valid(label) and label.no_depth_test,"high-contrast warning stays legible")
 var updates:int=game.get("boss_warning_updates")
 for i in range(100):game.call("_set_boss_telegraph",boss,true)
 check(game.get("boss_warning_updates")==updates,"unchanged warning avoids repeated text updates")
 check(game.get("boss_warning_labels").size()==1,"repeated warnings reuse a single node")
 game.call("_set_boss_telegraph",boss,false)
 check(not label.visible,"warning hides after telegraph")
 label.queue_free()
 await process_frame
 game.call("_prune_boss_warning_labels")
 check(game.get("boss_warning_labels").is_empty(),"stale warning reference pruned")
 game.queue_free()
 quit(1 if failures else 0)
