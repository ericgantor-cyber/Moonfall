extends SceneTree
const NAV=preload("res://scripts/systems/navigation_map.gd")
var failures:int=0

func check(ok:bool,description:String)->void:
 if ok:print("PASS atlas: "+description)
 else:
  failures+=1
  push_error("FAIL atlas: "+description)

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var map:Control=NAV.new()
 root.add_child(map)
 map.size=Vector2(300,240)
 map.call("update_navigation",Vector3(10,0,20),Vector3(10,0,-80),true)
 check(bool(map.get("has_waypoint")),"waypoint enabled")
 check(map.get("waypoint_position")==Vector3(10,0,-80),"waypoint keeps world coordinates")
 map.set("metres_per_half_width",300.0)
 check(is_equal_approx(float(map.get("metres_per_half_width")),300.0),"zoom controls map range")
 map.call("set_discovered",[Vector3(10,0,20)])
 check(map.get("discovered").size()==1,"visited positions render as map markers")
 map.call("update_navigation",Vector3(NAN,0,0),Vector3(INF,0,0),true)
 check(map.get("player_position")==Vector3(10,0,20) and not bool(map.get("has_waypoint")),"corrupt player and waypoint cannot poison atlas")
 map.call("update_navigation",Vector3(10,0,20),Vector3(20,0,30),true)
 check(bool(map.get("has_waypoint")) and map.get("waypoint_position")==Vector3(20,0,30),"valid waypoint recovers after invalid input")
 map.call("set_discovered",[Vector3(NAN,0,0),Vector3(2,0,3),Vector3(INF,0,0),Vector3(4,0,5)])
 check(map.get("discovered").size()==2,"invalid visited positions filtered")
 map.call("set_quest_markers",[Vector3.ZERO,Vector3(1.0e30,0,0),Vector3(10,0,10)])
 check(map.get("quest_markers").size()==2,"invalid quest locations filtered")
 var many_discoveries:Array[Vector3]=[]
 var many_quests:Array[Vector3]=[]
 for i in range(1111):
  many_discoveries.append(Vector3(float(i),0.0,0.0))
  many_quests.append(Vector3(float(i),0.0,0.0))
 map.call("set_discovered",many_discoveries)
 map.call("set_quest_markers",many_quests)
 check(map.get("discovered").size()==256 and map.get("quest_markers").size()==128,"atlas marker collections remain bounded on long sessions")
 check((map.get("discovered") as Array)[0]==Vector3(855,0,0) and (map.get("discovered") as Array)[255]==Vector3(1110,0,0),"atlas retains most recent 256 visited locations in chronological order")
 check((map.get("quest_markers") as Array)[0]==Vector3.ZERO and (map.get("quest_markers") as Array)[127]==Vector3(127,0,0),"atlas retains highest-priority quest marker ordering")

 # Storage can retain a longer validated exploration trail than the 256 UI dots.
 var exploration:Array[Vector3]=[]
 for history_index in range(3000):
  exploration.append(Vector3(float(history_index),0.0,float(history_index%29)))
 exploration.append(Vector3(NAN,0.0,0.0))
 var serialized:Array=NAV.encode_history(exploration)
 check(serialized.size()==2048,"checkpoints retain 2,048 valid discoveries without saving corrupt points")
 check(serialized[0]==[952.0,float(952%29)] and serialized.back()==[2999.0,float(2999%29)],"stored exploration retains the newest chronological discoveries")
 var reloaded:Array[Vector3]=NAV.decode_history(serialized)
 check(reloaded.size()==2048 and reloaded.front()==Vector3(952,0,952%29) and reloaded.back()==Vector3(2999,0,2999%29),"atlas history round-trips through save-shaped arrays")
 var bad_history:Array=[[NAN,2],["bad",0],[INF,1],[1.0e30,0],[7,8]]
 check(NAV.decode_history(bad_history)==[Vector3(7,0,8)],"malformed and nonfinite checkpoint locations cannot poison restored history")
 map.call("set_discovered",reloaded)
 check(map.get("discovered").size()==256,"2,048 saved discoveries render only 256 visible markers")
 check((map.get("discovered") as Array)[0]==Vector3(2744,0,2744%29) and (map.get("discovered") as Array).back()==Vector3(2999,0,2999%29),"render budget displays most recently discovered landmarks")
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"map-enabled main scene loads")
 if scene!=null:
  var game:Node3D=scene.instantiate()
  root.add_child(game)
  await process_frame
  check(game.has_method("_toggle_atlas"),"map toggle exists")
  check(game.has_method("_atlas_select_waypoint"),"click-to-mark handler exists")
  check(game.has_method("_atlas_zoom"),"zoom handler exists")
  var data:Dictionary=game.call("_checkpoint_extra_data")
  check(data.has("atlas_zoom_radius"),"save persists zoom")
  check(data.has("atlas_waypoint"),"save persists waypoint")
  check(data.has("atlas_discovered"),"save persists discovered locations")
  game.set("atlas_discovered",reloaded)
  var exploration_save:Dictionary=game.call("_checkpoint_extra_data")
  check((exploration_save.get("atlas_discovered",[]) as Array).size()==2048,"game checkpoint includes all retained exploration history")
  check(NAV.decode_history(exploration_save.get("atlas_discovered",[])).back()==Vector3(2999,0,2999%29),"game checkpoint restores latest discovery coordinates")

  check(data.has("atlas_quest_positions"),"quest destinations included in saves")
  check(game.has_method("atlas_set_quest_destination"),"quests can publish map targets")
  var atlas_source:String=FileAccess.get_file_as_string("res://scripts/native_v38_2.gd")
  check("atlas_chamber_scan_elapsed+=maxf(0.0,delta)" in atlas_source and "_atlas_record_nearby_chambers(here)" in atlas_source,"closed-atlas exploration scanning stays wired to world update")
  check(NAV.decode_chambers([0,3,3,8,-1,9,"7",NAN],9)==[0,3,8],"chamber save IDs reject duplicates and invalid entries")
  check(NAV.decode_chambers([],9).is_empty(),"older saves without chamber memory stay compatible")
  var chambers:Array=game.get("v321_chambers")
  check(chambers.size()==9,"existing authored chambers remain unchanged")
  for chamber in chambers:
   var chamber_node:Node3D=chamber["node"]
   game.call("_atlas_record_nearby_chambers",chamber_node.global_position)
  var remembered:Array=game.get("atlas_chambers_seen")
  check(remembered.size()==9,"walking near each existing chamber records it once")
  var first_chamber_node:Node3D=chambers[0]["node"]
  game.call("_atlas_record_nearby_chambers",first_chamber_node.global_position)
  check((game.get("atlas_chambers_seen") as Array).size()==9,"revisits never duplicate chamber discoveries")
  var progress_save:Dictionary=game.call("_checkpoint_extra_data")
  check((progress_save.get("atlas_chambers_seen",[]) as Array).size()==9,"all visited chambers persist in existing checkpoint extras")
  check(NAV.decode_chambers(progress_save.get("atlas_chambers_seen",[]),9).size()==9,"checkpoint chamber progress safely round-trips")

  game.queue_free()
 map.queue_free()
 print("MOONFALL ATLAS "+("PASS" if failures==0 else "FAIL"))
 quit(0 if failures==0 else 1)
