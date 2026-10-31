extends SceneTree
var errors := 0
func check(ok:bool, description:String) -> void:
 if ok:
  print("PASS v21: "+description)
 else:
  errors += 1
  push_error("FAIL v21: "+description)
func _initialize() -> void: call_deferred("_run")
func _run() -> void:
 var scene: PackedScene = load("res://scenes/main.tscn")
 check(scene!=null,"Main scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(game.get("v21_landmarks").size()==6,"Six unique regional monuments")
 check(game.get("v21_crests").size()==6,"Six illustrated regional crests")
 check(game.get("v21_setpiece_count")==72,"72 new sculptural environment dioramas")
 check(game.get("v21_scatter_count")==1440,"1440 batched ground details")
 check(game.get("v21_scatter_batches")==18,"18 instanced draw batches")
 check(game.get("v19_scenery_count")==54,"Existing v19 scenery preserved")
 check(game.get("grid_regions").size()==2400,"Invisible navigation grid preserved")
 for zone in range(6):
  var crest:Sprite3D=game.get("v21_crests")[zone]
  check(crest.texture!=null,"Region %d has a loaded PNG crest" % zone)
  var group:Node3D=game.get("v21_region_roots")[zone]
  check(group.get_child_count()==16,"Region %d: 1 monument, 12 dioramas, 3 scatter batches" % zone)
  check(group.get_parent()==game.get("decorative_groups")[zone],"Region %d art is culled with zone" % zone)
 game.call("_start_game","z")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Z_Original.webp"),"Original Z texture")
 game.call("_apply_graphics_quality","Smooth")
 for region_root in game.get("v21_region_roots"):
  check(not region_root.is_visible_in_tree(),"Smooth mode hides optional scenery")
 game.call("_apply_graphics_quality","Full")
 game.call("_start_game","nyra")
 check(load("res://assets/heroes/Nyra_Original.png")!=null,"Original Nyra boss artwork is available")
 game.queue_free()
 if errors==0:print("MOONFALL v21 GRAND ART PASS")
 quit(0 if errors==0 else 1)
