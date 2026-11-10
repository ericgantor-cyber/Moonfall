extends RefCounted
## Persistent, deduplicated quest and dungeon keys. Separate from equipment capacity.
const KEY_NAMES:Dictionary = {
 "wardens_crest": "Wardens' Crest",
 "outlaw_seal": "Outlaw Seal",
 "fallen_gate_key": "Fallen Gate Key"
}
const MAX_KEYS:int = 128

static func new_state()->Dictionary:
 return {"keys":[]}

static func add_key(state:Dictionary,key_id:String)->bool:
 if not KEY_NAMES.has(key_id):return false
 var keys:Variant=state.get("keys",[])
 if not keys is Array:return false
 if keys.has(key_id) or keys.size()>=MAX_KEYS:return false
 keys.append(key_id)
 state["keys"]=keys
 return true

static func has_key(state:Dictionary,key_id:String)->bool:
 var keys:Variant=state.get("keys",[])
 return KEY_NAMES.has(key_id) and keys is Array and keys.has(key_id)

static func normalize(saved:Variant)->Dictionary:
 var state:Dictionary=new_state()
 if not saved is Dictionary:return state
 var keys:Variant=saved.get("keys",[])
 if not keys is Array:return state
 for entry in keys:
  if entry is String and state["keys"].size()<MAX_KEYS:
   add_key(state,entry)
 return state

static func display_lines(state:Dictionary)->String:
 var keys:Variant=state.get("keys",[])
 if not keys is Array or keys.is_empty():return "No keys collected"
 var lines:Array[String]=[]
 for id in keys:
  if KEY_NAMES.has(str(id)):lines.append("• "+str(KEY_NAMES[id]))
 return "\n".join(lines) if not lines.is_empty() else "No keys collected"
