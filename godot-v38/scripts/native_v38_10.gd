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
func _v321_update_chambers()->void:
 super._v321_update_chambers()
 if not hero_playing:return
 for chamber in v321_chambers:
  var site:int=int(chamber.get("site",-1))
  if site<0 or site>=rifts.size():continue
  var stage:int=int(chamber.get("stage",0))
  var entry:Dictionary=rifts[site]
  if not RIFT_OBJECTIVES.stage_active(entry,stage):continue
  var label:Label3D=chamber.get("label") as Label3D
  if not is_instance_valid(label):continue
  var objective:String=RIFT_OBJECTIVES.objective(stage,false)
  if label.text!=objective:label.text=objective

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
 super._interact()
