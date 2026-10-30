extends "res://scripts/native_v32_5.gd"
## v32.6: boss impact emphasis, reusing the existing v32.0 pooled impact meshes.
## Cosmetic only: boss damage, movement, telegraphs and all save data unchanged.
const V326_BOSS_IMPACT_SCALE:float=1.65
var v326_boss_impacts:int=0

func _release_label()->String:return "v32.6"

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0)->void:
 var old_hp:float=float(target.get("hp",0.0))
 super._apply_player_hit(target,base_damage,stagger_strength)
 if v320_impacts.is_empty() or float(target.get("hp",0.0))>=old_hp:return
 # The parent just advanced this ring-buffer cursor for this successful hit.
 var index:int=(v320_impact_index-1+v320_impacts.size())%v320_impacts.size()
 var impact:Dictionary=v320_impacts[index]
 var is_boss:bool=bool(target.get("boss",false))
 impact["v326_boss_hit"]=is_boss
 if is_boss:v326_boss_impacts+=1

func _v320_stream_update()->void:
 super._v320_stream_update()
 # Base class owns the impact lifetime and animation. Apply only a visual
 # size multiplier after its normal update, never cumulatively.
 for impact in v320_impacts:
  if not bool(impact.get("v326_boss_hit",false)):continue
  var pulse:MeshInstance3D=impact.get("node")
  if not is_instance_valid(pulse) or not pulse.visible:continue
  pulse.scale*=V326_BOSS_IMPACT_SCALE

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v32.6"
 report["boss_impact_emphasis_events"]=v326_boss_impacts
 return report
