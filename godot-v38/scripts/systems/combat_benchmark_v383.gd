extends RefCounted
## Small, opt-in FPS sample window for the existing F2 status HUD.
## No per-frame allocations or actor/mesh creation.
const SAMPLE_LIMIT:int=40

static func record_fps(samples:Array[int],fps:int)->void:
 samples.append(maxi(0,fps))
 if samples.size()>SAMPLE_LIMIT:samples.pop_front()

static func summarize_fps(samples:Array[int])->Dictionary:
 if samples.is_empty():return {"count":0,"min":0,"max":0,"average":0}
 var min_fps:int=2147483647
 var max_fps:int=0
 var total:int=0
 for fps in samples:
  min_fps=mini(min_fps,fps)
  max_fps=maxi(max_fps,fps)
  total+=fps
 return {"count":samples.size(),"min":min_fps,"max":max_fps,"average":roundi(float(total)/float(samples.size()))}
