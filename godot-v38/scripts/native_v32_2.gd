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

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v32.2"
 report["dungeon_label_writes"]=v322_label_writes
 report["dungeon_label_writes_skipped"]=v322_skipped_label_writes
 report["dungeon_visibility_switches"]=v322_visibility_switches
 return report
