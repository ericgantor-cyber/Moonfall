extends SceneTree
const RULES:=preload("res://scripts/systems/graphics_rules.gd")
var fails:int=0
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var game:Node3D=(load("res://scenes/main.tscn") as PackedScene).instantiate()
 root.add_child(game)
 await process_frame
 for i in range(1,11):
  var quality:Dictionary=RULES.preset(i)
  game.set("gfx_settings",quality)
  game.call("_gfx_apply",false)
  var flora:Array=game.get("v312_foliage")
  if flora.size()!=6 or quality["projected_telegraphs"]!="Always Visible":fails+=1
  if i<=2 and (flora[0] as MultiMeshInstance3D).visible:fails+=1
  if i>=8 and not (flora[0] as MultiMeshInstance3D).visible:fails+=1
  print("PRESET %d: foliage=%s, combat_fx=%s, scale=%s, render_items=%d" % [i,quality["foliage_quality"],quality["combat_effects"],quality["render_scale"],Performance.get_monitor(Performance.RENDER_TOTAL_OBJECTS_IN_FRAME)])
 game.queue_free()
 print("MOONFALL v31.2 PRESET SWEEP PASS" if fails==0 else "MOONFALL v31.2 PRESET SWEEP FAIL")
 quit(0 if fails==0 else 1)
