extends SceneTree
## Run: godot --headless --path godot-v38 --script res://tests/world_loot_pity_v38.gd
var failures:int=0
func check(ok:bool,label:String)->void:
 if ok:print("PASS world loot pity: "+label)
 else:
  failures+=1
  push_error("FAIL world loot pity: "+label)

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var script:GDScript=load("res://scripts/native_v38_6.gd") as GDScript
 var equipment:GDScript=load("res://scripts/systems/equipment_inventory.gd") as GDScript
 check(script!=null and equipment!=null,"gameplay and equipment scripts load")
 if script!=null:
  var source:String=script.source_code
  check("if pity>=80:tier=maxi(tier,3)" in source,"80-miss epic guarantee")
  check("elif pity>=30:tier=maxi(tier,2)" in source,"30-miss rare guarantee")
  check("gear_state[\"loot_pity\"]=0 if tier>=2 else mini(80,pity+1)" in source,"rare drops reset pity; misses are bounded")
  check("if not existing.is_empty() or variant!=\"normal\":" in source,"special and guaranteed drops bypass pity")
  check("tier=maxi(tier,maxi(0,original_tier))" in source,"base item rarity never downgraded")
 if equipment!=null:
  var state:Dictionary=equipment.call("new_state")
  check(state.has("loot_pity"),"existing state stores pity")
  state["loot_pity"]=57
  var restored:Dictionary=equipment.call("normalize",state)
  check(int(restored.get("loot_pity",-1))==57,"pity survives save normalization")
  state["loot_pity"]=9999
  restored=equipment.call("normalize",state)
  check(int(restored.get("loot_pity",-1))==80,"corrupt pity is clamped")
 quit(1 if failures>0 else 0)
