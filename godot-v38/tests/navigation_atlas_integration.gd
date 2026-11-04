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
 # Verify atlas contracts without running expensive procedural world startup.
 # Separate gameplay smoke suites cover scene initialization and world progression.
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null and scene.can_instantiate(),"map-enabled main scene loads")
 var source:String=FileAccess.get_file_as_string("res://scripts/native_v38_2.gd")
 check("func _toggle_atlas(" in source and "func _atlas_select_waypoint(" in source,"atlas controls remain wired")
 check("func _atlas_zoom(" in source,"atlas zoom handler exists")
 check("NAVIGATION_MAP.encode_history(atlas_discovered)" in source and "NAVIGATION_MAP.decode_history(extras.get(" in source,"exploration checkpoint history is saved and restored")
 check("func atlas_set_quest_destination(" in source,"quest map targets remain supported")
 check("atlas_chamber_scan_elapsed+=maxf(0.0,delta)" in source and "_atlas_record_nearby_chambers(here)" in source,"closed-map chamber scanning runs in the world update")
 check("func _atlas_record_nearby_chambers(" in source and "chamber_id in atlas_chambers_seen" in source,"nearby chambers discovered once")
 check("atlas_chambers_seen" in source and "NAVIGATION_MAP.decode_chambers(" in source,"checkpoint saves and restores chamber progress")
 check(NAV.decode_chambers([0,3,3,8,-1,9,"7",NAN],9)==[0,3,8],"chamber checkpoint filters duplicates and invalid IDs")
 check(NAV.decode_chambers([],9).is_empty(),"legacy saves without chamber discoveries still load")
 var chamber_marks:Array[Vector3]=[]
 for chamber_index in range(120):
  chamber_marks.append(Vector3(float(chamber_index),0.0,0.0))
 chamber_marks[5]=Vector3(NAN,0.0,0.0)
 map.call("set_chamber_markers",chamber_marks)
 check((map.get("discovered_chambers") as Array).size()==64,"authored chamber markers have a bounded render budget")
 check((map.get("discovered_chambers") as Array)[5]==Vector3(6,0,0),"invalid chamber map coordinates are discarded")
 var trail_source:String=FileAccess.get_file_as_string("res://scripts/systems/navigation_map.gd")
 check("MAX_TRAIL_SEGMENT_METRES" in trail_source and "draw_line(centre+previous" in trail_source,"exploration markers connect into bounded route trail")
 check("discovered_chambers" in trail_source and "draw_arc(chamber_screen" in trail_source,"discovered chambers have distinct map icons")
 check("set_chamber_markers" in source and "_atlas_chamber_marker_positions()" in source,"game publishes discovered authored chamber positions to atlas")
 check("TEAL: EXPLORED TRAIL" in source,"map legend explains trails and chambers")

 var dungeon_anchors:Array[Vector3]=[Vector3(0,0,0),Vector3(250,0,0)]
 check(NAV.dungeon_site_at(Vector3(10,0,0),dungeon_anchors,-1)==0,"entering a rift area automatically selects dungeon map")
 check(NAV.dungeon_site_at(Vector3(30,0,0),dungeon_anchors,0)==0,"dungeon view stays stable near boundary")
 check(NAV.dungeon_site_at(Vector3(48,0,0),dungeon_anchors,0)==-1,"leaving the dungeon returns to world map")
 check(NAV.dungeon_site_at(Vector3(248,0,0),dungeon_anchors,-1)==1,"other dungeon sites select independently")
 check(NAV.dungeon_site_at(Vector3(NAN,0,0),dungeon_anchors,0)==-1,"invalid hero coordinates disable dungeon context")
 var room_locations:Array[Vector3]=[Vector3(-11,0,-3),Vector3(11,0,-3),Vector3(0,0,11)]
 map.call("set_dungeon_context",true,room_locations)
 check(bool(map.get("dungeon_active")) and (map.get("dungeon_rooms") as Array).size()==3,"dungeon displays its existing three numbered chambers")
 check(NAV.dungeon_room_index_at(Vector3(-10,0,-2),room_locations,3.0)==0,"click near first dungeon room selects chamber waypoint")
 check(NAV.dungeon_room_index_at(Vector3(11,0,-3),room_locations,3.0)==1,"click exactly on second chamber selects it")
 check(NAV.dungeon_room_index_at(Vector3(40,0,40),room_locations,3.0)==-1,"distant click leaves freeform waypoint positioning")
 check(NAV.dungeon_room_index_at(Vector3(NAN,0,0),room_locations,3.0)==-1,"corrupt dungeon map click rejected")
 check(NAV.dungeon_current_room_at(Vector3(-11,0,-3),room_locations)==0,"hero at chamber one is in room one")
 check(NAV.dungeon_current_room_at(Vector3(11,0,-3),room_locations)==1,"hero at chamber two is in room two")
 check(NAV.dungeon_current_room_at(Vector3(0,0,40),room_locations)==-1,"distant hero is between chambers")
 map.call("update_dungeon_room",Vector3(0,0,11))
 check(int(map.get("dungeon_current_room"))==2,"current chamber updates when hero approaches room three")
 map.call("update_dungeon_room",Vector3(0,0,60))
 check(int(map.get("dungeon_current_room"))==-1,"current chamber clears when leaving proximity")
 var room_map_source:String=FileAccess.get_file_as_string("res://scripts/systems/navigation_map.gd")
 check("draw_dashed_line(centre+start_offset" in room_map_source,"schematic room connections remain visually distinct from real corridors")
 check("CURRENT ROOM:" in source and "Between chambers" in source,"atlas readout explains current chamber")
 var already_seen:Array[int]=[0,2]
 check(NAV.next_unvisited_room(0,already_seen)==1,"next room shortcut selects first unvisited chamber")
 check(NAV.next_unvisited_room(1,already_seen)==0,"next room shortcut uses only the current dungeon")
 var completed_rooms:Array[int]=[0,1,2]
 check(NAV.next_unvisited_room(0,completed_rooms)==-1,"fully explored dungeon has no next chamber target")
 check(NAV.next_unvisited_room(-1,already_seen)==-1,"outside dungeon shortcut is disabled")
 map.call("update_dungeon_room",Vector3(-11,0,-3))
 map.call("set_dungeon_context",true,room_locations)
 check(int(map.get("dungeon_current_room"))==0,"unchanged atlas room context preserves live current-room highlight")
 check("func _atlas_mark_next_room(" in source and "NEXT UNVISITED CHAMBER" in source,"next room button is connected to world-space waypoint targeting")
 map.call("set_dungeon_context",false,room_locations)
 check(not bool(map.get("dungeon_active")) and (map.get("dungeon_rooms") as Array).is_empty(),"world view clears dungeon-only room overlay")
 check("func _atlas_update_dungeon_context(" in source and "_atlas_update_dungeon_context(here)" in source,"automatic context updates while walking with map closed")
 check("atlas_dungeon_zoom_radius" in source and "atlas_zoom_radius" in source,"world and dungeon retain separate zoom settings")
 check("DUNGEON MAP:" in source and "MOONFALL WORLD ATLAS" in source,"atlas header identifies current map context")
 map.queue_free()
 print("MOONFALL ATLAS "+("PASS" if failures==0 else "FAIL"))
 quit(0 if failures==0 else 1)
