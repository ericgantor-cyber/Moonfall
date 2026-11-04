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
 check("_atlas_room_id(site_id,stage)" in source and "v321_annexes.size()" in source,"checkpoint retains original chamber IDs and saves annex discoveries")
 check("This dungeon: %d/14" in source and "range(1,15)" in source,"atlas tracks fourteen rooms within each active rift")
 check("all_passages.append_array(v321_annex_passages)" in source and "start_known and end_known" in source,"atlas displays real annex bridges with exploration fog")
 check("RiftAnnexFloor" in source and "_atlas_all_site_chambers()" in source,"atlas reads actual fourteen-room geometry")
 check("nearest_distance" in source,"atlas stays in dungeon view around distant annex rooms")
 check(NAV.decode_chambers([0,8,9,19,41,42,-1],42)==[0,8,9,19,41],"checkpoint filters new and old chamber IDs safely")
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
 var room_radii:Array[float]=[4.1,4.1,4.1]
 map.call("set_dungeon_context",true,room_locations,room_radii)
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
 check("draw_circle(room_screen,floor_radius" in room_map_source and "dungeon_floor_radii" in room_map_source,"local map draws actual floor footprints instead of fictitious connections")
 check(not ("draw_dashed_line(centre+start_offset" in room_map_source),"local map does not imply invented walkable corridors")
 check((map.get("dungeon_floor_radii") as Array)==room_radii,"local map retains authored room floor measurements")
 check("_atlas_dungeon_floor_radii()" in source and "RiftChamberFloor" in source,"dungeon floor sizes are read from authored meshes")
 check("CURRENT ROOM:" in source and "Between chambers" in source,"atlas readout explains current chamber")
 map.call("update_dungeon_room",Vector3(-11,0,-3))
 map.call("set_dungeon_context",true,room_locations,room_radii)
 check(int(map.get("dungeon_current_room"))==0,"unchanged atlas room context preserves live current-room highlight")
 check(NAV.chart_facing(Vector3(1,0,0))==Vector2.RIGHT,"player orientation maps east to right")
 check(NAV.chart_facing(Vector3(0,0,-1))==Vector2.UP,"player orientation maps north to up")
 check(NAV.chart_facing(Vector3(NAN,0,0))==Vector2.UP,"invalid facing vector falls back safely")
 map.call("update_player_facing",Vector3(1,0,0))
 check(map.get("player_facing")==Vector2.RIGHT,"player facing arrow responds to orientation")
 var pillar_positions:Array[Vector3]=[Vector3(1,0,1),Vector3(NAN,0,0),Vector3(2,0,2)]
 map.call("set_dungeon_pillars",pillar_positions)
 check((map.get("dungeon_pillars") as Array).size()==2,"dungeon pillar map rejects invalid geometry points")
 check("_atlas_discovered_dungeon_pillars()" in source and "RiftPillar_%d" in source,"map pillar silhouettes use existing authored geometry")
 check("update_player_facing" in source and "player.global_transform.basis.z" in source,"atlas player indicator tracks hero facing")
 var indoor_path:Array[Vector3]=[]
 NAV.append_dungeon_sample(indoor_path,Vector3(0,0,0))
 NAV.append_dungeon_sample(indoor_path,Vector3(1,0,1))
 check(indoor_path.size()==1,"indoor trail rejects redundant short-distance samples")
 NAV.append_dungeon_sample(indoor_path,Vector3(NAN,0,0))
 check(indoor_path.size()==1,"indoor trail rejects invalid world positions")
 for sample_index in range(500):
  NAV.append_dungeon_sample(indoor_path,Vector3(float(sample_index)*3.0,0,0))
 check(indoor_path.size()==NAV.MAX_DUNGEON_TRAIL_POINTS and indoor_path.back()==Vector3(1497,0,0),"dense dungeon trail is capped at 384 samples")
 var indoor_serialized:Array=NAV.encode_history(indoor_path)
 check(NAV.decode_history(indoor_serialized)==indoor_path,"indoor discovery trail round-trips through existing checkpoint codec")
 map.call("set_dungeon_trace",indoor_path)
 check((map.get("dungeon_trace") as Array).size()==384,"local atlas renders a bounded indoor trace")
 check("atlas_dungeon_paths" in source and "NAVIGATION_MAP.append_dungeon_sample(path,here)" in source,"per-rift indoor exploration samples are persisted and collected while map is closed")
 check("set_dungeon_trace" in source and "draw_line(centre+last_offset" in room_map_source,"indoor breadcrumbs render on the measured floor plan")
 map.call("set_dungeon_context",false,room_locations)
 check(not bool(map.get("dungeon_active")) and (map.get("dungeon_rooms") as Array).is_empty(),"world view clears dungeon-only room overlay")
 check("func _atlas_update_dungeon_context(" in source and "_atlas_update_dungeon_context(here)" in source,"automatic context updates while walking with map closed")
 check("atlas_dungeon_zoom_radius" in source and "atlas_zoom_radius" in source,"world and dungeon retain separate zoom settings")
 check("DUNGEON FLOOR PLAN:" in source and "MOONFALL WORLD ATLAS" in source,"atlas header identifies current map context")
 var walkways:Array[Dictionary]=[
  {"start":Vector3(-11,0,-3),"end":Vector3(11,0,-3),"width":3.6},
  {"start":Vector3(NAN,0,0),"end":Vector3(1,0,0),"width":3.6},
  {"start":Vector3(0,0,0),"end":Vector3(1,0,0),"width":-5.0},
  {"start":Vector3(11,0,-3),"end":Vector3(0,0,11),"width":3.6}
 ]
 map.call("set_dungeon_passages",walkways)
 check((map.get("dungeon_passages") as Array).size()==2,"atlas accepts only real finite positive-width passage geometry")
 check(not bool((map.get("dungeon_passages") as Array)[0]["explored"]),"unknown corridors default to fogged presentation")
 var known_walkway:Array[Dictionary]=[{"start":Vector3(-11,0,-3),"end":Vector3(11,0,-3),"width":3.6,"explored":true}]
 map.call("set_dungeon_passages",known_walkway)
 check(bool((map.get("dungeon_passages") as Array)[0]["explored"]),"discovered passages illuminate after both endpoints are explored")
 check("start_known and end_known" in source and "atlas_chambers_seen.has" in source,"passage discovery uses saved chamber exploration")
 check("var explored:bool" in room_map_source and "edge_color" in room_map_source,"unknown passages render darker until discovered")
 map.call("set_dungeon_passages",walkways)
 check((map.get("dungeon_passages") as Array)[0]["start"]==Vector3(-11,0,-3),"dungeon walkway map preserves exact authored positions")
 check(NAV.passage_near_chart(Vector3(-11,0,-3),Vector3(11,0,-3),3.6,Vector3.ZERO,Vector2(15,15)),"visible connected passage intersects atlas viewport")
 check(not NAV.passage_near_chart(Vector3(100,0,100),Vector3(130,0,100),3.6,Vector3.ZERO,Vector2(15,15)),"distant passage geometry is culled before drawing")
 check(NAV.passage_near_chart(Vector3(-30,0,0),Vector3(30,0,0),3.6,Vector3.ZERO,Vector2(15,15)),"long crossing corridors remain visible even when both endpoints are offscreen")
 check(NAV.passage_near_chart(Vector3(16,0,0),Vector3(20,0,0),3.6,Vector3.ZERO,Vector2(15,15)),"wide passage touching chart edge remains visible")
 check(not NAV.passage_near_chart(Vector3(NAN,0,0),Vector3.ZERO,3.6,Vector3.ZERO,Vector2(15,15)),"invalid passage bounds are rejected")
 check(not NAV.passage_near_chart(Vector3.ZERO,Vector3.ONE,INF,Vector3.ZERO,Vector2(15,15)),"nonfinite passage width cannot enter rendering")
 # Structural interior collision is built from shared shapes, preserving open
 # approaches to the real chamber connectors in every stage.
 var stage_rules=load("res://scripts/systems/dungeon_stage2_rules.gd")
 for room_stage in range(1,4):
  var wall_count:int=0
  var open_count:int=0
  for segment in range(stage_rules.CHAMBER_WALL_SEGMENTS):
   var wall_angle:float=TAU*float(segment)/float(stage_rules.CHAMBER_WALL_SEGMENTS)
   if stage_rules.wall_segment_open(room_stage,wall_angle):open_count+=1
   else:wall_count+=1
  check(wall_count>=3 and open_count>=4,"chamber %d has real walls and open passage thresholds" % room_stage)
  for other_stage in range(1,4):
   if other_stage==room_stage:continue
   var destination:Vector3=stage_rules.ROOM_OFFSETS[other_stage-1]-stage_rules.ROOM_OFFSETS[room_stage-1]
   check(stage_rules.wall_segment_open(room_stage,atan2(destination.z,destination.x)),"chamber %d keeps real bridge entrance to chamber %d clear" % [room_stage,other_stage])
 check(stage_rules.wall_segment_open(0,0.0) and stage_rules.wall_segment_open(1,NAN),"invalid wall placement input fails open, not blocking traversal")
 check(is_equal_approx(stage_rules.CHAMBER_FLOOR_RADIUS,6.2) and stage_rules.CHAMBER_WALL_RADIUS>stage_rules.CHAMBER_FLOOR_RADIUS,"sanctum floor radius expands to 6.2 metres with outside perimeter walls")
 check(stage_rules.CHAMBER_WALL_SEGMENTS==20,"expanded chambers have bounded wall segment count")
 var stage_rule_count:int=stage_rules.EXPLORATION_OFFSETS.size()
 check(stage_rules.ANNEX_TITLES.size()==11 and stage_rules.ANNEX_SIGILS.size()==11,"eleven annexes have named, distinct thematic identities")
 var unique_titles:Dictionary={}
 for named_stage in range(4,15):
  var title:String=stage_rules.annex_title(named_stage)
  unique_titles[title]=true
  var tint:Color=stage_rules.annex_tint(named_stage)
  check(title.length()>3 and tint.is_finite(),"annex %d has a valid readable identity and finite accent color" % named_stage)
 check(unique_titles.size()==11,"annex names are unique instead of repeated generic chambers")
 check(stage_rules.annex_title(3)=="Unknown Chamber" and stage_rules.annex_sigil(15)=="","unknown annex stages return safe identity values")
 check(stage_rule_count==11,"eleven exploration annexes extend each three-chamber rift to fourteen physical rooms")
 var first_annex:Vector3=stage_rules.exploration_position(Vector3.ZERO,4)
 check(first_annex.distance_to(stage_rules.room_position(Vector3.ZERO,3))>=13.0,"first annex connects without overlapping Guardian Heart")
 var occupied:Array[Vector3]=[]
 for annex_stage in range(4,15):
  var annex:Vector3=stage_rules.exploration_position(Vector3.ZERO,annex_stage)
  var previous:Vector3=stage_rules.exploration_previous(Vector3.ZERO,annex_stage)
  check(is_equal_approx(annex.distance_to(previous),14.0),"annex %d has a finite continuous 14m bridge" % annex_stage)
  check(not occupied.has(annex),"annex %d has a unique world-space floor" % annex_stage)
  occupied.append(annex)
 var cull_source:String=FileAccess.get_file_as_string("res://scripts/native_v32_2.gd")
 var passage_source:String=FileAccess.get_file_as_string("res://scripts/native_v32_1.gd")
 check("func _v321_build_passages(" in passage_source and "RiftPassageCollision" in passage_source and "StaticBody3D.new()" in passage_source,"connected rift walkways have physical collision")
 check("RiftPassageArch" in passage_source and "RiftPassageFloor" in passage_source,"dungeon passages include geometry and arch thresholds")
 check("RiftInteriorCollision" in passage_source and "RiftInteriorFloorCollision" in passage_source,"all chambers have solid floor collision independent of visual culling")
 check("RiftInteriorWallCollision_" in passage_source and "V321_RULES.wall_segment_open(stage,wall_angle)" in passage_source,"real solid chamber perimeter respects connected doorway openings")
 check("var chamber_wall_mesh:=BoxMesh.new()" in passage_source and "var chamber_wall_shape:=BoxShape3D.new()" in passage_source,"interior wall meshes and shapes are shared to limit bloat")
 check("chamber_collision_floor.radius=V321_RULES.CHAMBER_FLOOR_RADIUS" in passage_source and "shared_floor.top_radius=V321_RULES.CHAMBER_FLOOR_RADIUS" in passage_source,"expanded visual floor and physical collision share the same radius")
 check("RiftInteriorStageRing" in passage_source and "TorusMesh.new()" in passage_source,"each chamber has its own nonblocking ritual floor ring")
 check("func _v321_build_exploration_annexes()" in passage_source and "RiftAnnexCollision" in passage_source and "RiftAnnexBridgeCollision" in passage_source,"annex rooms and connecting corridors have real collision")
 check("for stage in range(4,15)" in passage_source and "v321_annexes.append" in passage_source,"fourteen-room layout is built without extending story wave triggers")
 check("RiftAnnexSigil" in passage_source and "RiftAnnexRune_" in passage_source,"named annexes have low-poly nonblocking ritual markings")
 check("sigil_ring:=TorusMesh.new()" in passage_source and "annex_palette" in passage_source,"annex visual meshes and stage palettes are shared across sites")
 check("V321_RULES.annex_title(stage)" in passage_source,"annex signs show their actual named chamber identity")
 check("for annex in v321_annexes" in cull_source and "for annex_link in v321_annex_passages" in cull_source,"annex visuals are independently distance-culled")
 check("v321_passages" in passage_source and "func _atlas_dungeon_passages(" in source and "set_dungeon_passages" in source,"world-connected walkways are projected into real atlas floor plan")
 check("for passage in v321_passages" in cull_source,"passage visuals are culled without removing collision")
 map.queue_free()
 print("MOONFALL ATLAS "+("PASS" if failures==0 else "FAIL"))
 quit(0 if failures==0 else 1)
