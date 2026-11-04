extends RefCounted
## Play imported GLB Skeleton3D/AnimationPlayer clips as visual feedback only.
## No root-motion, no animation-triggered combat, no changes to Z or Nyra.
const STATE_NAMES:Array[String]=["idle","move","attack","hurt","death","spawn","telegraph","special_1","special_2","victory"]
const BLEND_SECONDS:float=0.12
const MAX_SEARCH_NODES:int=240

static func canonical_clip(raw_name:String)->String:
 var clean:String=raw_name.to_lower().strip_edges()
 if STATE_NAMES.has(clean):return clean
 var tokens:PackedStringArray=clean.replace("|","_").replace("/","_").replace(".","_").replace("-","_").replace(" ","_").split("_",false)
 for i in range(tokens.size()-1,-1,-1):
  var token:String=tokens[i]
  match token:
   "idle","stand","breathing","breathe":return "idle"
   "walk","walking","run","running","sprint","move","locomotion":return "move"
   "attack","attacking","slash","bite","strike","punch","shoot":return "attack"
   "hurt","hit","damage","flinch","stagger","impact":return "hurt"
   "death","die","dying","dead":return "death"
   "spawn","appear","summon":return "spawn"
   "telegraph","warning","chargeup":return "telegraph"
   "spell","special1":return "special_1"
   "special2":return "special_2"
   "victory","taunt":return "victory"
 return ""

static func bind_imported(model:Node3D)->Dictionary:
 if not is_instance_valid(model):return {}
 var stack:Array[Node]=[model]
 var scanned:int=0
 var animation_player:AnimationPlayer=null
 var skeleton_found:bool=false
 while not stack.is_empty():
  var node:Node=stack.pop_back()
  scanned+=1
  if scanned>MAX_SEARCH_NODES:return {}
  if node is Skeleton3D:
   skeleton_found=true
  elif node is AnimationPlayer and animation_player==null:
   animation_player=node as AnimationPlayer
  for child in node.get_children():
   stack.append(child)
 if not skeleton_found or not is_instance_valid(animation_player):return {}
 var mapped:Dictionary={}
 for raw in animation_player.get_animation_list():
  var state:String=canonical_clip(str(raw))
  if not state.is_empty() and not mapped.has(state):
   mapped[state]=str(raw)
 if mapped.is_empty():return {}
 return {"player":animation_player,"clips":mapped,"state":"","last_clip":""}

static func state_for_actor(actor:Dictionary,now:float)->String:
 if not bool(actor.get("alive",false)):return "death"
 if float(actor.get("hit_react_until",0.0))>now:return "hurt"
 if float(actor.get("attack_swing_until",0.0))>now:return "attack"
 if float(actor.get("windup",0.0))>0.0 or float(actor.get("elite_lunge_windup",0.0))>0.0:
  return "telegraph"
 if float(actor.get("trait_warning_until",0.0))>now:return "telegraph"
 var body:CharacterBody3D=actor.get("body") as CharacterBody3D
 if is_instance_valid(body) and Vector2(body.velocity.x,body.velocity.z).length_squared()>0.04:
  return "move"
 return "idle"

static func apply_state(binding:Dictionary,requested:String)->void:
 if binding.is_empty():return
 var player:AnimationPlayer=binding.get("player") as AnimationPlayer
 if not is_instance_valid(player):return
 var clips:Dictionary=binding.get("clips",{})
 var desired:String=requested if clips.has(requested) else "move" if requested=="telegraph" and clips.has("move") else "idle"
 if not clips.has(desired):return
 var clip:String=str(clips[desired])
 # Don't restart attacks/hurt/death every 80ms; imported one-shots must finish.
 if str(binding.get("state",""))==requested:
  if player.is_playing() or desired not in ["idle","move"]:return
  player.play(StringName(clip),BLEND_SECONDS)
  return
 binding["state"]=requested
 binding["last_clip"]=clip
 if str(player.current_animation)!=clip or not player.is_playing():
  player.play(StringName(clip),BLEND_SECONDS)
