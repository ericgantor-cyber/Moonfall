extends SceneTree
func _initialize()->void:call_deferred("_run")
func _snap(filename:String)->void:
 await process_frame
 await RenderingServer.frame_post_draw
 var image:Image=root.get_viewport().get_texture().get_image()
 print("SNAPSHOT ",filename," ",image.get_width(),"x",image.get_height()," error=",image.save_png(filename))
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 for beast in game.get("beasts"):beast["hostile"]=false
 game.get("player").global_position=game.get("region_shrines")[0].global_position+Vector3(0,0.85,3)
 # Ensure a damaged equipped sample item for the blacksmith display.
 game.get("gear_items").append({"name":"Moonbound Blade","rarity":"Rare","power":45,"durability":65,"uid":"preview-1"})
 game.call("_equip",0)
 game.set("gold",50)
 game.call("_toggle_menu")
 game.call("_menu_tab","Wallet")
 for i in range(20):await process_frame
 await _snap("user://Moonfall_v30_8_Blacksmith_Preview.png")
 game.call("_toggle_menu")
 var chest:Dictionary=game.get("treasure_chests")[0]
 game.get("player").global_position=(chest["node"] as Node3D).global_position+Vector3(2.5,0.85,2.2)
 for i in range(20):await process_frame
 await _snap("user://Moonfall_v30_8_Treasure_Preview.png")
 game.queue_free()
 quit(0)
