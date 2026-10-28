extends SceneTree
## Godot 4.7.2 headless regression for v16 visual and performance controls.
var failures := 0
func check(ok:bool, description:String) -> void:
 if ok: print("PASS v16: "+description)
 else:
  failures+=1
  push_error("FAIL v16: "+description)
func _initialize() -> void:
 call_deferred("_run")
func _run() -> void:
 var resource:PackedScene=load("res://scenes/main.tscn")
 check(resource!=null,"v16 main scene parses")
 if resource==null:quit(1);return
 var game:Node3D=resource.instantiate()
 root.add_child(game)
 await process_frame
 # v31.1: verify the legacy flat-material fallback before inspecting its
 # cached textures; high graphics now intentionally uses separate PBR materials.
 var realism_enabled:bool=game.has_method("_realism_set_materials")
 if realism_enabled:
  var visual_settings:Dictionary=(game.get("gfx_settings") as Dictionary).duplicate(true)
  visual_settings["terrain_surface"]="Off"
  game.set("gfx_settings",visual_settings)
  game.call("_gfx_apply",false)
 var textures:Dictionary=game.get("zone_surface_textures")
 check(textures.size()==6,"Six region surfaces have generated textures")
 for zone in range(6):
  var floor:Node=game.get("region_nodes")[zone].get_node("RegionGround")
  var renderer:MeshInstance3D=null
  for child in floor.get_children():
   if child is MeshInstance3D:renderer=child;break
  check(renderer!=null and renderer.material_override is StandardMaterial3D,"Region "+str(zone)+" uses a material")
  if renderer!=null and renderer.material_override is StandardMaterial3D:
   var material:StandardMaterial3D=renderer.material_override
   check(material.albedo_texture==textures[zone],"Region "+str(zone)+" uses its own cached texture")
 game.call("_start_game","z")
 var sprite:Sprite3D=game.get("hero_sprite")
 check(sprite.texture==load("res://assets/heroes/Z_Original.webp"),"Original Z texture referenced")
 game.call("_attack")
 check(str(game.get("combat_pose_kind"))=="attack","Attack pose starts")
 check(game.get("combat_cues").size()==1,"Attack adds one transient slash")
 game.call("_process",0.15)
 check(absf(sprite.scale.x-1.0)>0.01,"Z sprite animates through node transform")
 for f in range(8):game.call("_process",0.10)
 check(game.get("combat_cues").size()==0,"Slash visual expires")
 game.call("_dodge")
 check(str(game.get("combat_pose_kind"))=="dodge","Dodge visual triggers")
 for f in range(9):game.call("_process",0.10)
 check(game.get("combat_cues").size()==0,"Dodge visual expires")
 game.call("_spell","nova")
 check(str(game.get("combat_pose_kind"))=="spell","Spell pose triggers")
 for f in range(9):game.call("_process",0.10)
 check(game.get("combat_cues").size()==0,"Spell visual expires")
 game.call("_start_game","nyra")
 check(sprite.texture==load("res://assets/heroes/Nyra_Original.png"),"Original Nyra texture referenced")
 check(sprite.scale==Vector3.ONE and sprite.rotation==Vector3.ZERO,"Hero pose resets on hero switch")
 for i in range(22):game.call("_cue",Vector3.ZERO,"attack",Color.YELLOW)
 check(game.get("combat_cues").size()<=12,"Effects are bounded to twelve")
 for f in range(9):game.call("_process",0.10)
 check(game.get("combat_cues").is_empty(),"Old cues are reclaimed")
 check(game.get("region_nodes").size()==6 and game.get("beasts").size()==56,"Existing world and wildlife preserved")
 check(game.get("hero_playing"),"Game remains playable after cosmetic updates")
 if failures==0:print("MOONFALL NATIVE v16 VISUALS PASS")
 else:push_error("MOONFALL NATIVE v16 VISUALS FAILED: "+str(failures))
 game.queue_free()
 quit(0 if failures==0 else 1)
