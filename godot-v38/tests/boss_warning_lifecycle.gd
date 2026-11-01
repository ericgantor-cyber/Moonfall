extends SceneTree
var fails:int=0
func check(ok:bool,s:String)->void:
 if ok:print("PASS boss warning: "+s)
 else:fails+=1;push_error("FAIL boss warning: "+s)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 var body:CharacterBody3D=CharacterBody3D.new()
 game.add_child(body)
 var label:Label3D=Label3D.new()
 label.text="ORIGINAL LABEL"
 body.add_child(label)
 var beast:Dictionary={"boss_id":98765,"body":body,"boss_phase":2,"boss_pattern":2,"nameplate":label,"warning_radius":2.8,"warning_pos":Vector3.ZERO}
 game.call("_set_boss_telegraph",beast,true)
 var warning:Label3D=game.get("boss_warning_labels").get(98765) as Label3D
 check(is_instance_valid(warning),"dedicated label")
 check(is_instance_valid(warning) and warning.visible,"warning visible")
 check(is_instance_valid(warning) and "WIDE SWEEP" in warning.text,"correct attack named")
 check(label.text=="ORIGINAL LABEL","original nameplate preserved")
 game.call("_set_boss_telegraph",beast,true)
 check(game.get("boss_warning_labels").size()==1,"label reused")
 game.call("_set_boss_telegraph",beast,false)
 check(is_instance_valid(warning) and not warning.visible,"warning hidden")
 beast["boss_phase"]=3
 beast["boss_pattern"]=4
 game.call("_set_boss_telegraph",beast,true)
 check(is_instance_valid(warning) and "DELAYED STRIKE" in warning.text,"phase switch works")
 game.queue_free()
 quit(1 if fails else 0)
