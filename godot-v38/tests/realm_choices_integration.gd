extends SceneTree
const MORALITY = preload("res://scripts/systems/morality_reputation.gd")
var failures:int = 0

func check(condition:bool, description:String)->void:
 if condition: print("PASS realm integration: "+description)
 else:
  failures += 1
  push_error("FAIL realm integration: "+description)

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"main scene loads")
 if scene!=null:
  var game:Node3D=scene.instantiate()
  root.add_child(game)
  await process_frame
  check(game.has_method("_choose_realm_action"),"scene has actual moral-choice handler")
  check(game.has_method("_checkpoint_extra_data"),"scene has checkpoint integration")
  var fresh:Dictionary=game.get("realm_ethics")
  check(int(fresh.get("morality",999))==0,"new game begins neutral")
  var chosen:Dictionary=MORALITY.apply_choice(fresh,"realm_village_aid",35,{"Villagers":40})
  game.set("realm_ethics",chosen)
  var extras:Dictionary=game.call("_checkpoint_extra_data")
  check(extras.has("realm_ethics"),"checkpoint includes moral choices")
  check(extras["realm_ethics"]==chosen,"checkpoint round-trips moral state")
  game.queue_free()
 print("REALM INTEGRATION "+("PASS" if failures==0 else "FAIL"))
 quit(0 if failures==0 else 1)
