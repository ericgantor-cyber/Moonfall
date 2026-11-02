extends "res://scripts/native_v38_9.gd"
## Distinct, cached boss warning labels avoid overwriting health/nameplates.
const BOSS_READOUT=preload("res://scripts/systems/boss_telegraph_readout.gd")
const GUARDIAN_RULES=preload("res://scripts/systems/guardian_attack_rules.gd")
var boss_warning_labels:Dictionary={}
const MAX_BOSS_WARNING_LABELS:int=24
var boss_warning_updates:int=0

func _refresh_boss_phase(beast:Dictionary)->void:
 var old_phase:int=clampi(int(beast.get("boss_phase",1)),1,3)
 var old_pattern:int=int(beast.get("boss_pattern",0))
 super._refresh_boss_phase(beast)
 var new_phase:int=clampi(int(beast.get("boss_phase",1)),1,3)
 if new_phase<=old_phase:return
 _show_message("GUARDIAN PHASE "+str(new_phase)+" - NEW ATTACKS!",1.5)
 # Keep the already-telegraphed attack intact when health crosses a
 # phase threshold during its windup.
 if float(beast.get("windup",0.0))>0.0:return
 beast["boss_pattern"]=GUARDIAN_RULES.next_pattern_after_phase_change(old_pattern,new_phase)

func _prune_boss_warning_labels()->void:
 for key in boss_warning_labels.keys():
  var label:Variant=boss_warning_labels.get(key)
  if not is_instance_valid(label) or not is_instance_valid(label.get_parent()):
   boss_warning_labels.erase(key)


func _set_boss_telegraph(beast:Dictionary,active:bool)->void:
 super._set_boss_telegraph(beast,active)
 var id:int=int(beast.get("boss_id",-1))
 if id<0:return
 if boss_warning_labels.size()>=MAX_BOSS_WARNING_LABELS:_prune_boss_warning_labels()
 var warning:Label3D=boss_warning_labels.get(id) as Label3D
 if not active:
  if is_instance_valid(warning):warning.visible=false
  elif boss_warning_labels.has(id):boss_warning_labels.erase(id)
  return
 var body:CharacterBody3D=beast.get("body") as CharacterBody3D
 if not is_instance_valid(body):
  if is_instance_valid(warning):warning.visible=false
  return
 if is_instance_valid(warning) and warning.get_parent()!=body:
  warning.queue_free()
  boss_warning_labels.erase(id)
  warning=null
 if not is_instance_valid(warning):
  if boss_warning_labels.size()>=MAX_BOSS_WARNING_LABELS:return
  warning=Label3D.new()
  warning.name="GuardianAttackWarning"
  warning.font_size=35
  warning.pixel_size=0.004
  warning.billboard=BaseMaterial3D.BILLBOARD_ENABLED
  warning.modulate=Color(1.0,0.75,0.32)
  warning.position=Vector3(0,2.3,0)
  warning.no_depth_test=true
  body.add_child(warning)
  boss_warning_labels[id]=warning
 var phase:int=clampi(int(beast.get("boss_phase",1)),1,3)
 var current_pattern:int=GUARDIAN_RULES.pattern_index(int(beast.get("boss_pattern",0))-1,phase)
 var new_text:String=BOSS_READOUT.warning_text(phase,current_pattern)
 if warning.text!=new_text:
  warning.text=new_text
  boss_warning_updates+=1
 warning.visible=true

## Exploration-first readouts for the existing three-chamber rift.
## No extra enemy spawns, rewards, scene nodes, or save fields.
const RIFT_OBJECTIVES=preload("res://scripts/systems/rift_objective_rules.gd")
func _v321_chamber_label(entry:Dictionary,stage:int)->String:
 var prompt:String=RIFT_OBJECTIVES.chamber_prompt(entry,stage)
 if prompt.is_empty():return super._v321_chamber_label(entry,stage)
 var site:int=rifts.find(entry)
 if site<0:return prompt
 var charged:int=_depth_attunement_count(site)
 var key:String=str(site)+":"+str(stage)
 var glyph_state:String="ATTUNED" if depth_glyph_charged.has(key) else "UNATTUNED"
 return prompt+"\nMOON GLYPH • "+glyph_state+"  ("+str(charged)+"/3)"

## Wave-free rifts: activate each resident only once, across the existing chambers.
## Stage 3 is the legacy completion sentinel, not a new timed wave.
func _activate_rift_wave(site_id:int)->void:
 if site_id<0 or site_id>=rifts.size():return
 var entry:Dictionary=rifts[site_id]
 entry["stage"]=3
 super._activate_rift_wave(site_id)
 var slots:Array=entry.get("slots",[])
 for i in range(mini(slots.size(),3)):
  var id:int=int(slots[i])
  if id<0 or id>=beasts.size():continue
  var resident:Dictionary=beasts[id]
  var home:Vector3=V321_RULES.enemy_position(entry["anchor"],i+1,i)
  resident["home"]=home
  var body:CharacterBody3D=resident.get("body") as CharacterBody3D
  if is_instance_valid(body):body.position=home+Vector3(0,0.62,0)
  resident["alert_until"]=0.0
 _set_rift_label(entry)
 _show_message(str(entry["name"])+" • Explore the sanctum and defeat its guardian.",3.0)

func _set_rift_label(entry:Dictionary)->void:
 var label:Label3D=entry.get("label") as Label3D
 if not is_instance_valid(label):return
 if bool(entry.get("cleared",false)):
  label.text="✓ "+str(entry.get("name","Rift"))+" SEALED"
 elif bool(entry.get("active",false)):
  label.text=str(entry.get("name","Rift"))+" • EXPLORE & DEFEAT GUARDIAN"
 else:
  label.text="RIFT • "+str(entry.get("name","Rift"))+"\n(G) Explore"

## Continuous rifts allow all three physical moon glyphs to be attuned in any
## order. The older stage index is only a legacy completion sentinel.
func _depth_tick(delta:float)->void:
 super._depth_tick(delta)
 if not hero_playing:return
 for item in depth_glyphs:
  var site:int=int(item["site"])
  if site<0 or site>=rifts.size():continue
  if active_rift!=site or bool(rifts[site].get("cleared",false)):continue
  var label:Label3D=item["label"] as Label3D
  if not is_instance_valid(label):continue
  var key:String=str(site)+":"+str(int(item["stage"]))
  label.text="MOON GLYPH • CHARGED" if depth_glyph_charged.has(key) else "MOON GLYPH • G TO ATTUNE"

func _interact()->void:
 if hero_playing and not death_active and not menu_open and not mounted and is_instance_valid(player):
  for item in depth_glyphs:
   var root:Node3D=item["node"] as Node3D
   if not is_instance_valid(root):continue
   if player.global_position.distance_squared_to(root.global_position)>DEPTH_INTERACT_RADIUS_SQ:continue
   var site:int=int(item["site"])
   if site<0 or site>=rifts.size():return
   if active_rift!=site or bool(rifts[site].get("cleared",false)):
    _show_message("Explore this rift to awaken its glyphs.",2.0)
    return
   var key:String=str(site)+":"+str(int(item["stage"]))
   if depth_glyph_charged.has(key):
    _show_message("This moon glyph is already attuned.",1.5)
    return
   depth_glyph_charged[key]=true
   depth_spell_charges=mini(3,depth_spell_charges+1)
   _show_message("GLYPH ATTUNED! Lunar echo ready ("+str(depth_spell_charges)+" charges).",3.0)
   return
  for secret in rift_secret_signs:
   var marker:Label3D=secret["node"]
   var site:int=int(secret["site"])
   if site!=active_rift or bool(rifts[site].get("cleared",false)):continue
   if player.global_position.distance_squared_to(marker.global_position)>DEPTH_INTERACT_RADIUS_SQ:continue
   if bool(secret["read"]):
    _show_message("Inscription recorded • revisit this chamber for its history.",2.0)
   else:
    secret["read"]=true
    var lore:Array[String]=["The first guardians sealed the crimson moon beneath these stones.","Moonbound travelers followed the three violet glyphs to find sanctuary.","The Guardian Heart remembers those who returned without surrendering."]
    _show_message("ANCIENT INSCRIPTION: "+lore[site%lore.size()],5.0)
   return
 super._interact()

## Replace inherited wave instructions only when the World tab is opened.
## No new menu state or per-frame UI work is required.
func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if which!="World" or not is_instance_valid(menu_body):return
 menu_body.text=menu_body.text.replace("clear three escalating waves in the open world.","explore its three chambers without waves.")
 menu_body.text+="\n\n[b]ANCIENT INSCRIPTIONS[/b]  "+str(_rift_discovery_count())+"/"+str(rift_secret_signs.size())+" discovered. Search each Moonbound Passage for a hidden inscription."
 menu_body.text=menu_body.text.replace("Clear enemies to advance chambers, then defeat the Rift Champion for an additional named legendary.","Explore every chamber freely, attune moon glyphs in any order, and defeat the rift residents to earn a named legendary.")
 for entry in rifts:
  if bool(entry.get("active",false)) and not bool(entry.get("cleared",false)):
   menu_body.text=menu_body.text.replace(str(entry["name"])+"  WAVE "+str(entry["stage"]),str(entry["name"])+"  EXPLORING")

## Optional discoverable lore in the existing world-connected rift rooms.
## Built once, never spawns enemies or modifies checkpoint/reward state.
var rift_secret_signs:Array[Dictionary]=[]
func _v321_build_chambers()->void:
 super._v321_build_chambers()
 # Three reusable procedural accents per dungeon; no lights or per-frame effects.
 # Violet moon, ember-red ruins, and cool blue magic make the three rooms distinct.
 var tones:Array[Color]=[Color("9159eb"),Color("d44b56"),Color("49b6da")]
 for entry in rifts:
  var center:Vector3=entry["anchor"]
  for chamber in range(1,4):
   var at:Vector3=V321_RULES.room_position(center,chamber)
   var accent:Node3D=_add_sphere(at+Vector3(4.4,0.8,-3.8),0.45,tones[chamber-1],false,true)
   accent.name="RiftMoonEmberAccent"
   # A compact broken-stone shrine silhouette beside each chamber, assembled once.
   # Reuse existing primitives and shared scene materials; no additional lights.
   var stone:Color=Color("373044").lerp(tones[chamber-1],0.18)
   for side in [-1,1]:
    var column:Node3D=_add_box(at+Vector3(float(side)*5.8,0.9,3.4),Vector3(0.9,1.8,0.9),stone,false)
    column.name="RiftBrokenPillar"
    var crown:Node3D=_add_box(at+Vector3(float(side)*5.8,1.95,3.4),Vector3(1.25,0.25,1.25),tones[chamber-1],false)
    crown.name="RiftMoonCrown"
 for site in range(rifts.size()):
  var anchor:Vector3=rifts[site]["anchor"]
  var marker:=Label3D.new()
  marker.name="RiftHiddenInscription"
  marker.text="ANCIENT INSCRIPTION • G TO READ"
  marker.position=V321_RULES.room_position(anchor,2)+Vector3(3.3,1.45,-3.1)
  marker.font_size=24
  marker.pixel_size=0.005
  marker.billboard=BaseMaterial3D.BILLBOARD_ENABLED
  marker.modulate=Color("bb9df2")
  marker.visible=false
  world.add_child(marker)
  rift_secret_signs.append({"site":site,"node":marker,"read":false})

func _process(delta:float)->void:
 super._process(delta)
 if not hero_playing or not is_instance_valid(player):return
 for secret in rift_secret_signs:
  var marker:Label3D=secret["node"]
  var site:int=int(secret["site"])
  var allowed:bool=site>=0 and site<rifts.size() and active_rift==site and not bool(rifts[site].get("cleared",false))
  marker.visible=allowed and player.global_position.distance_squared_to(marker.global_position)<20.0*20.0
  if marker.visible:
   var wanted:String="ANCIENT INSCRIPTION • READ" if bool(secret["read"]) else "ANCIENT INSCRIPTION • G TO READ"
   if marker.text!=wanted:marker.text=wanted

## Exploration discoveries reset on a new adventure, never on ordinary menu use.
## Rift lore does not award farmable currency or introduce a save field.
func _reset_new_adventure()->void:
 super._reset_new_adventure()
 for secret in rift_secret_signs:
  secret["read"]=false

func _rift_discovery_count()->int:
 var total:int=0
 for secret in rift_secret_signs:
  if bool(secret.get("read",false)):total+=1
 return total

## Five continuous rift dungeons per zone. The three original sites retain
## their save indexes; the other 27 are appended for backward compatibility.
const RIFTS_PER_ZONE:int=5
func _build_rift_encounters()->void:
 super._build_rift_encounters()
 var original_count:int=rifts.size()
 for zone in range(REGIONS.size()):
  var existing:int=0
  for site in range(original_count):
   if int(rifts[site]["zone"])==zone:existing+=1
  var region:Dictionary=REGIONS[zone]
  var mid:Vector2=region["at"]
  var kind:String=str(region.get("monster","wolf"))
  if not BEASTS.has(kind):kind="wolf"
  for index in range(existing,RIFTS_PER_ZONE):
   var angle:float=TAU*float(index)/float(RIFTS_PER_ZONE)+0.30
   var radius:float=30.0+float(index%2)*4.0
   var anchor:=Vector3(mid.x+cos(angle)*radius,0.0,mid.y+sin(angle)*radius)
   var sigil:=_add_sphere(anchor+Vector3(0,0.65,0),0.64,Color("a457ff"),false,true)
   sigil.name="PersistentRiftAnchor"
   var sign:=Label3D.new()
   sign.text="RIFT • "+str(region.get("name","Unknown"))+" Sanctuary "+str(index+1)+"\n(G) Explore"
   sign.font_size=28
   sign.pixel_size=0.006
   sign.billboard=BaseMaterial3D.BILLBOARD_ENABLED
   sign.position=Vector3(0,1.8,0)
   sigil.add_child(sign)
   var entry:Dictionary={"name":str(region.get("name","Unknown"))+" Sanctuary "+str(index+1),"zone":zone,"anchor":anchor,"marker":sigil,"label":sign,"stage":0,"active":false,"cleared":false,"slots":[]}
   var site_id:int=rifts.size()
   rifts.append(entry)
   for n in range(3):
    var creature_at:Vector3=anchor+Vector3(cos(float(n)*TAU/3.0)*5.0,0,sin(float(n)*TAU/3.0)*5.0)
    _spawn_beast(creature_at,kind,zone,"dusk" if n==2 else "moon")
    var creature:Dictionary=beasts[beasts.size()-1]
    creature["dungeon_site"]=site_id
    entry["slots"].append(int(creature["boss_id"]))
    _despawn_native_beast(creature,0.0)
   _set_rift_label(entry)
