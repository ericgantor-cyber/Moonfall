extends "res://scripts/native_v32_1.gd"
## v32.2 lean dungeon presentation: cached labels, defensive visibility,
## and no new combat state or save fields.
var v322_label_writes:int=0
var v322_visibility_switches:int=0
var v322_skipped_label_writes:int=0

func _release_label()->String:return "v32.2"

func _v321_update_chambers()->void:
 if not is_instance_valid(player):return
 var here:Vector3=player.global_position
 var quality_low:bool=graphics_quality=="Smooth" or int(gfx_settings.get("environment_detail",6))<=2
 for chamber in v321_chambers:
  var site:int=int(chamber["site"])
  var stage:int=int(chamber["stage"])
  if site<0 or site>=rifts.size():continue
  var root:Node3D=chamber["node"]
  if not is_instance_valid(root):continue
  var visible_now:bool=hero_playing and V321_RULES.room_visible(root.global_position.distance_squared_to(here),quality_low)
  if root.visible!=visible_now:
   root.visible=visible_now
   v322_visibility_switches+=1
  # Offscreen labels need no rebuilding. Refresh when visible again.
  if not visible_now:continue
  var entry:Dictionary=rifts[site]
  var label:Label3D=chamber["label"]
  if not is_instance_valid(label):continue
  var next_text:String
  if bool(entry.get("cleared",false)):
   next_text="✓ "+V321_RULES.chamber_title(stage)+" • SEALED"
  elif bool(entry.get("active",false)):
   next_text=("▶ " if int(entry.get("stage",0))==stage else "◦ ")+V321_RULES.chamber_title(stage)
  else:
   next_text="CHAMBER %d • %s"%[stage,V321_RULES.chamber_title(stage)]
  if label.text!=next_text:
   label.text=next_text
   v322_label_writes+=1
  else:v322_skipped_label_writes+=1
  v321_chamber_checks+=1
 # Each rift's three real walkways are culled separately from chamber decorations.
 # The gameplay collision remains active even when the platform mesh is offscreen.
 for passage in v321_passages:
  var bridge:Node3D=passage.get("node") as Node3D
  if not is_instance_valid(bridge):continue
  var visible_now:bool=hero_playing and bridge.global_position.distance_squared_to(here)<95.0*95.0
  if bridge.visible!=visible_now:
   bridge.visible=visible_now
   v322_visibility_switches+=1

 # Annexes use independent distance culling; their StaticBody3D collisions never deactivate.
 for annex in v321_annexes:
  var room:Node3D=annex.get("node") as Node3D
  if not is_instance_valid(room):continue
  var should_show:bool=hero_playing and room.global_position.distance_squared_to(here)<90.0*90.0
  if room.visible!=should_show:
   room.visible=should_show
   v322_visibility_switches+=1
 for annex_link in v321_annex_passages:
  var link:Node3D=annex_link.get("node") as Node3D
  if not is_instance_valid(link):continue
  var should_show:bool=hero_playing and link.global_position.distance_squared_to(here)<95.0*95.0
  if link.visible!=should_show:
   link.visible=should_show
   v322_visibility_switches+=1

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v32.2"
 report["dungeon_label_writes"]=v322_label_writes
 report["dungeon_label_writes_skipped"]=v322_skipped_label_writes
 report["dungeon_visibility_switches"]=v322_visibility_switches
 report["dungeon_connected_passages"]=v321_passages.size()+v321_annex_passages.size()
 report["dungeon_exploration_annexes"]=v321_annexes.size()
 return report
