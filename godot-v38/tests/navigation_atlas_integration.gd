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
 map.call("set_discovered",Array([Vector3(10,0,20)],TYPE_VECTOR3,"",null))
 check(map.get("discovered").size()==1,"visited positions render as map markers")
 map.call("update_navigation",Vector3(NAN,0,0),Vector3(INF,0,0),true)
 check(map.get("player_position")==Vector3(10,0,20) and not bool(map.get("has_waypoint")),"corrupt player and waypoint cannot poison atlas")
 map.call("update_navigation",Vector3(10,0,20),Vector3(20,0,30),true)
 check(bool(map.get("has_waypoint")) and map.get("waypoint_position")==Vector3(20,0,30),"valid waypoint recovers after invalid input")
 map.call("set_discovered",Array([Vector3(NAN,0,0),Vector3(2,0,3),Vector3(INF,0,0),Vector3(4,0,5)],TYPE_VECTOR3,"",null))
 check(map.get("discovered").size()==2,"invalid visited positions filtered")
 map.call("set_quest_markers",Array([Vector3.ZERO,Vector3(1.0e30,0,0),Vector3(10,0,10)],TYPE_VECTOR3,"",null))
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

 # Scene instantiation triggers expensive world generation; the complete game is
 # already booted by the other Windows gameplay and installed-game smoke suites.
 # Keep atlas integration focused on its controls and checkpoint wiring.
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null and scene.can_instantiate(),"map-enabled main scene loads")
 var native_source:String=FileAccess.get_file_as_string("res://scripts/native_v38_2.gd")
 check("func _toggle_atlas(" in native_source,"map toggle exists")
 check("func _atlas_select_waypoint(" in native_source,"click-to-mark handler exists")
 check("func _atlas_zoom(" in native_source,"zoom handler exists")
 check("atlas_zoom_radius" in native_source and "atlas_waypoint" in native_source,"save wires zoom and waypoint")
 check("atlas_discovered" in native_source and "atlas_quest_positions" in native_source,"save wires exploration and quest locations")
 check("func atlas_set_quest_destination(" in native_source,"quests can publish map targets")
 map.queue_free()
 print("MOONFALL ATLAS "+("PASS" if failures==0 else "FAIL"))
 quit(0 if failures==0 else 1)
