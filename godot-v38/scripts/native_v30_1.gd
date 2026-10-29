extends "res://scripts/native_v30.gd"
## v30.1: opt-in, frame-based performance capture. Gameplay and art are unchanged.
## F9 begins or ends capture. Reports live only in user://, not in saves.
const PERF_SECONDS:float=60.0
const PERF_REPORT:String="user://moonfall_v30_1_performance.json"
var perf_active:bool=false
var perf_elapsed:float=0.0
var perf_frame_ms:Array[float]=[]
var perf_region:String=""
var perf_quality:String=""
var perf_last_report:Dictionary={}

func _release_label()->String:
 return "v30.1"

func _unhandled_key_input(event:InputEvent)->void:
 if hero_playing and not menu_open and event is InputEventKey:
  if event.pressed and not event.echo and event.keycode==KEY_F9:
   if perf_active:_perf_stop(true)
   else:_perf_start()
   get_viewport().set_input_as_handled()
   return
 super._unhandled_key_input(event)

func _process(delta:float)->void:
 super._process(delta)
 if perf_active and hero_playing and not menu_open:
  _perf_sample(delta)

func _perf_start()->void:
 if not hero_playing or menu_open:return
 perf_active=true
 perf_elapsed=0.0
 perf_frame_ms.clear()
 perf_region=str(REGIONS[region_index]["name"])
 perf_quality=graphics_quality
 _show_message("FPS benchmark ON. Play normally for 60 seconds; F9 stops early.",3.0)

func _perf_sample(delta:float)->void:
 if not perf_active:return
 if not is_finite(delta) or delta<=0.0:return
 # No per-frame disk writes; minimal overhead while capturing.
 perf_elapsed+=delta
 perf_frame_ms.append(delta*1000.0)
 if perf_elapsed>=PERF_SECONDS:_perf_stop(true)

func _perf_report()->Dictionary:
 var ordered:Array=perf_frame_ms.duplicate()
 ordered.sort()
 var n:int=ordered.size()
 var average_fps:float=float(n)/maxf(perf_elapsed,0.001)
 var p99_index:int=clampi(ceili(float(n)*0.99)-1,0,maxi(0,n-1))
 var p99_ms:float=float(ordered[p99_index]) if n>0 else 0.0
 var worst_ms:float=float(ordered[n-1]) if n>0 else 0.0
 var ai:Dictionary=v30_diagnostics()
 return {"version":"v30.1","timestamp":Time.get_datetime_string_from_system(),
  "os":OS.get_name(),"display_backend":DisplayServer.get_name(),
  "renderer":RenderingServer.get_video_adapter_name(),"godot_version":str(Engine.get_version_info().get("string","")),
  "region_at_start":perf_region,"graphics_quality_at_start":perf_quality,
  "seconds":perf_elapsed,"frames":n,"average_fps":average_fps,
  "p99_frame_ms":p99_ms,"worst_frame_ms":worst_ms,
  "ai_average_usec":float(ai.get("average_ai_usec",0.0)),
  "ai_worst_usec":int(ai.get("worst_ai_usec",0)),
  "headless":DisplayServer.get_name()=="headless",
  "note":"Frame-rate sample, not a standardized GPU benchmark. Compare same area and settings."}

func _perf_stop(save_report:bool)->void:
 if not perf_active:return
 perf_active=false
 if perf_frame_ms.is_empty():
  _show_message("Benchmark stopped: no gameplay frames captured.",2.0)
  return
 perf_last_report=_perf_report()
 if not save_report:return
 var file:FileAccess=FileAccess.open(PERF_REPORT,FileAccess.WRITE)
 if file==null:
  _show_message("Benchmark finished, but the report could not be saved.",3.0)
  push_warning("Moonfall benchmark: cannot write report to "+ProjectSettings.globalize_path(PERF_REPORT))
  return
 file.store_string(JSON.stringify(perf_last_report,"  "))
 file.close()
 var loc:String=ProjectSettings.globalize_path(PERF_REPORT)
 print("MOONFALL PERFORMANCE REPORT: "+loc)
 _show_message("Benchmark saved: "+str(roundi(float(perf_last_report["average_fps"])))+" average FPS. Find the JSON in Moonfall's user data folder.",5.0)

func _reset_new_adventure()->void:
 if perf_active:_perf_stop(false)
 super._reset_new_adventure()
 perf_active=false
 perf_elapsed=0.0
 perf_frame_ms.clear()
