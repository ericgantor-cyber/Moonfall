extends SceneTree
var failures := 0
func check(ok:bool,message:String) -> void:
 if ok:print("PASS v25: "+message)
 else:
  failures+=1
  push_error("FAIL v25: "+message)
func _initialize() -> void:call_deferred("_run")
func _run() -> void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"current scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 game.call("_update_hud")
 check("NATIVE "+str(game.call("_release_label")) in game.get("hud").text,"one release label renders in HUD")
 check(game.get("v24_diorama_cache").size()==72,"all 72 dioramas cached once")
 game.call("_v24_update_lod")
 check(game.get("v24_dioramas_visible")>=0,"cached scenery LOD updates")
 game.call("_build_invisible_grid")
 var blocked:int=game.get("grid_blocked_count")
 game.call("_build_invisible_grid")
 check(game.get("grid_blocked_count")==blocked,"grid rebuild does not accumulate blocked count")
 check(game.get("grid_regions").size()==2400,"invisible grid capacity unchanged")
 # JSON can be valid but numeric values can still be corrupt. Recovery must reject.
 var target:Dictionary={"schema":2,"hero":"z","at":[0.0,0.0],"hp":100.0,"level":1}
 var path: String="user://v25_malformed_test.json"
 var fp:=FileAccess.open(path,FileAccess.WRITE)
 if fp!=null:
  target["hp"]="broken"
  fp.store_string(JSON.stringify(target))
  fp.close()
  check(game.call("_decode_save",path).is_empty(),"malformed numeric checkpoint rejected")
  target["hp"]=100.0
  fp=FileAccess.open(path,FileAccess.WRITE)
  fp.store_string(JSON.stringify(target))
  fp.close()
  check(not game.call("_decode_save",path).is_empty(),"historic schema-2 checkpoint remains readable")
  DirAccess.remove_absolute(ProjectSettings.globalize_path(path))
 else:check(false,"test profile can write checkpoint")
 game.queue_free()
 if failures==0:print("MOONFALL v25 LEGACY MODERNIZATION PASS")
 quit(0 if failures==0 else 1)
