extends SceneTree
var fail:int=0
func check(v:bool,msg:String)->void:
 if v:print("PASS lifecycle: "+msg)
 else:fail+=1;push_error("FAIL lifecycle: "+msg)
func _initialize()->void:call_deferred("run")
func run()->void:
 var packed:PackedScene=load("res://scenes/main.tscn")
 check(packed!=null,"scene loaded")
 if packed==null:quit(1);return
 var g:Node3D=packed.instantiate()
 root.add_child(g)
 await process_frame
 var old_body:CharacterBody3D=CharacterBody3D.new()
 g.add_child(old_body)
 var a:Dictionary={"boss_id":5191,"body":old_body,"boss_phase":1,"boss_pattern":1}
 g.call("_set_boss_telegraph",a,true)
 var first:Label3D=g.get("boss_warning_labels").get(5191)
 check(is_instance_valid(first),"initial warning allocated")
 check(is_instance_valid(first) and first.get_parent()==old_body,"warning belongs to initial boss")
 var new_body:CharacterBody3D=CharacterBody3D.new()
 g.add_child(new_body)
 a["body"]=new_body
 a["boss_phase"]=3
 a["boss_pattern"]=4
 g.call("_set_boss_telegraph",a,true)
 var second:Label3D=g.get("boss_warning_labels").get(5191)
 check(is_instance_valid(second) and second.get_parent()==new_body,"replacement boss owns warning")
 check(is_instance_valid(second) and "DELAYED STRIKE" in second.text,"new phase warning shown")
 check(g.get("boss_warning_labels").size()==1,"one cached warning per boss ID")
 g.call("_set_boss_telegraph",a,false)
 check(is_instance_valid(second) and not second.visible,"warning hidden after attack")
 g.queue_free()
 quit(1 if fail else 0)
