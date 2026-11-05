extends "res://scripts/native_v30.gd"
## v30.1: bounded automatic frame-based performance capture. Gameplay and art unchanged.
## Automatically samples gameplay in 60s windows, overwriting one user:// JSON report.
## F9 saves early; the next active gameplay frame resumes automatic capture.
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
 if not hero_playing or menu_open:return
 if not perf_active:_perf_start(true)
 _perf_sample(delta)

func _perf_start(automatic:bool=false)->void:
 if not hero_playing or menu_open:return
 perf_active=true
 perf_elapsed=0.0
 perf_frame_ms.clear()
 perf_region=str(REGIONS[region_index]["name"])
 perf_quality=graphics_quality
 if not automatic:_show_message("FPS benchmark ON. F9 saves the current recording early.",3.0)

func _perf_sample(delta:float)->void:
 if not perf_active:return
 if not is_finite(delta) or delta<=0.0:return
 # No per-frame disk writes; minimal overhead while capturing.
 perf_elapsed+=delta
 perf_frame_ms.append(delta*1000.0)
 if perf_elapsed>=PERF_SECONDS:_perf_stop(true,true)

func _perf_report()->Dictionary:
 var ordered:Array=perf_frame_ms.duplicate()
 ordered.sort()
 var n:int=ordered.size()
 var average_fps:float=float(n)/maxf(perf_elapsed,0.001)
 var p50_index:int=clampi(ceili(float(n)*0.50)-1,0,maxi(0,n-1))
 var p95_index:int=clampi(ceili(float(n)*0.95)-1,0,maxi(0,n-1))
 var p99_index:int=clampi(ceili(float(n)*0.99)-1,0,maxi(0,n-1))
 var p50_ms:float=float(ordered[p50_index]) if n>0 else 0.0
 var p95_ms:float=float(ordered[p95_index]) if n>0 else 0.0
 var p99_ms:float=float(ordered[p99_index]) if n>0 else 0.0
 var worst_ms:float=float(ordered[n-1]) if n>0 else 0.0
 # The 1% low is FPS derived from the mean slowest 1% of frame times,
 # not a percentile of instantaneous FPS.
 var tail_count:int=maxi(1,ceili(float(n)*0.01))
 var tail_sum_ms:float=0.0
 for i in range(n-tail_count,n):
  tail_sum_ms+=float(ordered[i])
 var low_1pct_fps:float=1000.0*float(tail_count)/tail_sum_ms if tail_sum_ms>0.0 else 0.0
 var ai:Dictionary=v30_diagnostics()
 return {"version":"v30.1","timestamp":Time.get_datetime_string_from_system(),
  "os":OS.get_name(),"display_backend":DisplayServer.get_name(),
  "renderer":RenderingServer.get_video_adapter_name(),"godot_version":str(Engine.get_version_info().get("string","")),
  "region_at_start":perf_region,"graphics_quality_at_start":perf_quality,
  "seconds":perf_elapsed,"frames":n,"average_fps":average_fps,
  "p50_frame_ms":p50_ms,"p95_frame_ms":p95_ms,"p99_frame_ms":p99_ms,
  "worst_frame_ms":worst_ms,"one_percent_low_fps":low_1pct_fps,
  "ai_average_usec":float(ai.get("average_ai_usec",0.0)),
  "ai_worst_usec":int(ai.get("worst_ai_usec",0)),
  "headless":DisplayServer.get_name()=="headless",
  "note":"Frame-rate sample, not a standardized GPU benchmark. Compare same area and settings."}

func _perf_stop(save_report:bool,automatic:bool=false)->void:
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
 if not automatic:_show_message("Benchmark saved: "+str(roundi(float(perf_last_report["average_fps"])))+" average FPS. Find the JSON in Moonfall's user data folder.",5.0)

func _reset_new_adventure()->void:
 if perf_active:_perf_stop(false)
 super._reset_new_adventure()
 perf_active=false
 perf_elapsed=0.0
 perf_frame_ms.clear()
