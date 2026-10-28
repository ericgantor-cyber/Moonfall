extends SceneTree
const RULES:=preload("res://scripts/systems/oxygen_v372.gd")
var failures:int=0
func _initialize()->void:call_deferred("_run")
func check(ok:bool,what:String)->void:
 if ok:print("PASS oxygen: "+what)
 else:
  failures+=1
  push_error("FAIL oxygen: "+what)
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"active 3D scene imports")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(is_instance_valid(game.get("v372_oxygen_panel")),"oxygen HUD created on startup")
 check(is_equal_approx(float(game.get("v372_oxygen")),100.0),"oxygen begins full")
 check(RULES.can_dive(true,true,false,false,false),"can dive in lakes during gameplay")
 check(not RULES.can_dive(false,true,false,false,false),"cannot dive on land")
 check(not RULES.can_dive(true,true,true,false,false),"pause prevents dives")
 check(not RULES.can_dive(true,true,false,false,true),"death prevents dives")
 var air:float=RULES.MAX_OXYGEN
 for i in range(101):air=RULES.advance(air,0.25,true)
 check(air<=0.0,"underwater air drains to zero")
 check(RULES.is_low(air),"low oxygen warning threshold works")
 for i in range(16):air=RULES.advance(air,0.25,false)
 check(air>75.0,"breathing refills oxygen on surfacing")
 check(is_equal_approx(RULES.advance(0.0,-1.0,true),0.0),"negative frame time cannot drain air")
 check(RULES.damage_tick(100.0)>0.0,"zero oxygen produces damage")
 check(not (game.get("v37_wakes") as Array).is_empty(),"existing swim visuals still present")
 game.queue_free()
 print("MOONFALL v37.2 OXYGEN PASS" if failures==0 else "MOONFALL v37.2 OXYGEN FAIL")
 quit(0 if failures==0 else 1)
