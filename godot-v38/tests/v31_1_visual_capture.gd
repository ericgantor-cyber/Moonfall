extends SceneTree
func _initialize()->void:call_deferred("_run")
func _capture(filename:String)->void:
 for i in range(12):await process_frame
 await RenderingServer.frame_post_draw
 var image:Image=root.get_viewport().get_texture().get_image()
 print("SCREENSHOT ",filename," ",image.get_width(),"x",image.get_height()," : ",image.save_png(filename))
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 for enemy in game.get("beasts"):enemy["hostile"]=false
 var p:=game.get("player") as CharacterBody3D
 p.global_position=Vector3(-60.0,0.85,-30.0)
 game.call("_update_camera",1.0)
 var prefs:Dictionary=(game.get("gfx_settings") as Dictionary).duplicate(true)
 prefs["terrain_surface"]="PBR Detail"
 prefs["realism_grade"]="Natural"
 prefs["wet_surfaces"]="On"
 prefs["soft_shadows"]="On"
 game.set("gfx_settings",prefs)
 game.call("_gfx_apply",false)
 await _capture("/mnt/data/Moonfall_v31_1_Realistic_Environment.png")
 prefs["terrain_surface"]="Off"
 prefs["realism_grade"]="Classic"
 game.set("gfx_settings",prefs)
 game.call("_gfx_apply",false)
 await _capture("/mnt/data/Moonfall_v31_1_Classic_Environment.png")
 game.queue_free()
 quit(0)
