extends SceneTree
## Optional headless Godot gameplay regression; run with the matching Godot binary.
var failures:int=0
func check(ok:bool,message:String)->void:
 if not ok:
  failures+=1
  push_error("FAIL death system: "+message)
 else:print("PASS death system: "+message)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 game.set("hero_level",12)
 game.set("kills_total",25)
 game.set("gold",44)
 game.call("_apply_incoming_damage",100000.0)
 check(game.get("death_active"),"fatal damage opens the death screen")
 check((game.get("death_overlay") as Control).visible,"death screen visible")
 check(game.get("menu_open"),"combat paused while death screen is open")
 var report:String=game.call("_death_stats_text")
 check(report.contains("Level 12") and report.contains("25"),"death screen uses actual hero stats")
 check(not report.to_lower().contains("survival") and not report.to_lower().contains("time"),"no survival timer")
 game.call("_death_return_to_sanctuary")
 check(not game.get("death_active") and not game.get("menu_open"),"sanctuary return resumes gameplay")
 check(is_equal_approx(float(game.get("hp")),float(game.get("max_hp"))),"sanctuary restores HP")
 var checkpoint:Dictionary=game.call("_read_save")
 check(int(checkpoint.get("schema",0))==2,"existing checkpoint schema remains valid")
 check(int(checkpoint.get("level",0))==12 and int(checkpoint.get("kills",0))==25,"permanent progress persists")
 game.set("materials",{"Moon Phoenix Feather":1})
 # Sanctuary grants a short invulnerability window; simulate its expiry before testing resurrection.
 game.set("phoenix_grace_until",0.0)
 game.call("_apply_incoming_damage",100000.0)
 check(not game.get("death_active"),"rare feather prevents one death")
 check(is_equal_approx(float(game.get("hp")),float(game.get("max_hp"))*0.30),"phoenix returns with thirty percent HP")
 check(int((game.get("materials") as Dictionary).get("Moon Phoenix Feather",0))==0,"feather consumed")
 check(game.get("phoenix_used_this_expedition"),"second automatic revival locked until sanctuary")
 game.set("phoenix_grace_until",0.0)
 game.call("_apply_incoming_damage",100000.0)
 check(game.get("death_active"),"second fatal blow opens death screen")
 game.call("_death_return_to_title")
 check(not game.get("hero_playing") and (game.get("start_screen") as Control).visible,"title return available")
 game.queue_free()
 print("MOONFALL v33.8 DEATH PASS" if failures==0 else "MOONFALL v33.8 DEATH FAIL")
 quit(0 if failures==0 else 1)
