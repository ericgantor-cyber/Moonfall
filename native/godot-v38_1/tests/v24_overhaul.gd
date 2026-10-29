extends SceneTree
var failures := 0
func check(condition:bool, label:String) -> void:
 if condition:print("PASS v24: "+label)
 else:
  failures+=1
  push_error("FAIL v24: "+label)
func _initialize() -> void:call_deferred("_run")
func _run() -> void:
 var packed:PackedScene=load("res://scenes/main.tscn")
 check(packed!=null,"main scene can load")
 if packed==null:quit(1);return
 var game:Node3D=packed.instantiate()
 root.add_child(game)
 await process_frame
 check(game.get("v24_popups").size()==18,"damage-label pool is bounded")
 check(game.get("v24_elite_markers").size()==8,"elite markers are pooled")
 check(game.get("grid_regions").size()==2400,"invisible terrain grid retained")
 check(game.get("v21_landmarks").size()==6,"all region monuments retained")
 game.call("_start_game","z")
 game.call("_v24_set_zoom",2.0)
 check(game.get("camera").projection==Camera3D.PROJECTION_PERSPECTIVE and is_equal_approx(float(game.get("v24_camera_size")),12.0) and is_equal_approx(float(game.get("v36_distance")),6.5),"third-person zoom clamps at minimum")
 game.call("_v24_set_zoom",100.0)
 check(is_equal_approx(float(game.get("v24_camera_size")),31.0) and is_equal_approx(float(game.get("v36_distance")),21.6),"third-person zoom clamps at maximum")
 game.call("_v24_update_lod")
 var monuments_ok:=true
 for m in game.get("v21_landmarks"):
  if not m.visible:monuments_ok=false
 check(monuments_ok,"monuments not culled")
 check(game.get("v24_dioramas_visible")<=72,"optional dioramas culled by distance")
 var elite:Dictionary={}
 for beast in game.get("beasts"):
  if beast.get("variant","normal")!="normal" and not bool(beast.get("boss",false)) and int(beast.get("zone",-1))==0:
   elite=beast;break
 check(not elite.is_empty(),"a regional elite exists")
 if not elite.is_empty():
  elite["elite_lunge_windup"]=0.6
  elite["elite_lunge_target"]=Vector3(-58,0,-28)
  game.call("_v24_sync_elite_markers")
  check(game.get("v24_marker_count")==1,"active elite warning displayed")
  var marker=game.get("v24_elite_markers")[0]
  check(marker.visible and is_equal_approx(marker.position.x,-58),"marker follows locked strike point")
  check(marker.get_child_count()==0,"visual warning does not create a physics collider")
  elite["elite_lunge_windup"]=0.0
  game.call("_v24_sync_elite_markers")
  check(game.get("v24_marker_count")==0,"warning disappears after windup")
 var victim:Dictionary={}
 for beast in game.get("beasts"):
  if int(beast.get("zone",-1))==0 and bool(beast.get("alive",false)) and not bool(beast.get("boss",false)):
   victim=beast;break
 if not victim.is_empty():
  game.get("player").global_position=victim["body"].global_position+Vector3(1.4,0,0)
  victim["hp"]=float(victim["max"])
  game.call("_apply_player_hit",victim,4.0)
  check(game.get("v24_damage_events")>=1,"damage produces accessible readout")
  game.call("_apply_graphics_quality","Smooth")
  check(not game.get("v24_popups")[0].visible,"Smooth quality hides optional damage numbers")
  check(game.get("v24_marker_count")==0,"expired warning remains hidden in Smooth mode")
  if not elite.is_empty():
   elite["elite_lunge_windup"]=0.5
   game.call("_v24_sync_elite_markers")
   check(game.get("v24_marker_count")==1,"essential elite warning remains visible in Smooth mode")
   elite["elite_lunge_windup"]=0.0
  game.call("_apply_graphics_quality","Full")
 game.call("_start_game","nyra")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Nyra_Original.png"),"original Nyra texture preserved")
 check(game.get("v24_marker_count")==0,"hero restart clears elite warnings")
 game.call("_start_game","z")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Z_Original.webp"),"original Z texture preserved")
 if failures==0:print("MOONFALL v24 OVERHAUL PASS")
 game.queue_free()
 quit(0 if failures==0 else 1)
