extends "res://scripts/native_v37_6.gd"
## v37.7 Playability Overhaul: low-overhead, contextual help and an explicit
## emergency exit from underwater caves. Art, save schema and combat untouched.
const V377_RULES:=preload("res://scripts/systems/playability_v377.gd")
var v377_rescue_count:int=0

func _build_ui()->void:
 super._build_ui()
 # The older menu still called this a v13 isometric slice. Update its
 # PRESENTATION only; do not touch the original Z / Nyra portrait textures.
 if is_instance_valid(start_screen):
  for child in start_screen.find_children("*","Label",true,false):
   var heading:Label=child as Label
   if not is_instance_valid(heading):continue
   if "MOONFALL NATIVE v13" in heading.text:
    heading.text="☾  MOONFALL v37.7 • THE LIVING REALM"
   elif "Native isometric Witchwood vertical slice" in heading.text:
    heading.text="Connected 3D world • Original Z / Nyra art • Godot 4"

func _v377_can_rescue()->bool:
 return V377_RULES.rescue_available(hero_playing,death_active,mounted,v373_depth,v37_swim_zone,current_threat)

func _v377_status()->String:
 var location:String="Unknown"
 if region_index>=0 and region_index<REGIONS.size():location=str(REGIONS[region_index]["name"])
 var text:String="[b]RIGHT NOW[/b] • "+location+" • HP "+V377_RULES.format_percent(hp,max_hp)
 if v373_depth>V377_RULES.MIN_SUBMERGED_DEPTH:
  text+=" • OXYGEN "+str(roundi(v372_oxygen))+"% • DEPTH "+str(snappedf(v373_depth,0.1))
 text+="\n"+V377_RULES.urgent_tip(v373_depth,v372_oxygen,hp,max_hp,current_threat)
 return text

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if not is_instance_valid(menu_body):return
 if which=="System" or which=="World":
  menu_body.text+="\n\n"+_v377_status()
  if _v377_can_rescue():
   _atlas_action("EMERGENCY SURFACE (NO COMBAT)","v377_rescue","Return to the shoreline if trapped underwater. Not usable during combat; no items or gold spent.")
 if which=="System":
  menu_body.text+="\n\n[b]PLAYABILITY[/b]\nF1: searchable help • F10/Esc: Atlas • C: descend while swimming • Space: ascend.\nUse the Raven Atlas Tips tab for more than 100 tips across all systems."
  _atlas_action("OPEN SEARCHABLE TIPS (F1)","v377_help","Browse combat, boss fights, crafting, water, oxygen, wildlife, controls and saving.")
  _atlas_action("SHOW LIVE SYSTEM STATUS","v377_status","Display current HP, air, region, and whether emergency recovery is available.")

func _v376_render()->void:
 super._v376_render()
 if is_instance_valid(menu_body):menu_body.text=_v377_status()+"\n\n"+menu_body.text

func _v377_surface()->void:
 if not _v377_can_rescue():
  _show_message("Emergency surfacing needs a safe underwater location outside combat.",2.5)
  return
 if not is_instance_valid(player):return
 var zone:int=v37_swim_zone
 var destination:Vector3=V377_RULES.shoreline(REGIONS,zone,V362_JOBS.initial_water_offset(zone),V373_RULES.PLAYER_SURFACE_Y)
 # Physically move to dry land FIRST, then restore the collision/oxygen state.
 player.global_position=destination
 player.velocity=Vector3.ZERO
 _v373_reset_depth()
 v37_swimming=false
 v37_swim_zone=-1
 v372_diving=false
 v372_oxygen=V372_OXYGEN.MAX_OXYGEN
 v372_low_air_warned=false
 v372_drowning_elapsed=0.0
 v377_rescue_count+=1
 _v373_refresh_overlay()
 _v372_update_hud()
 _update_camera(1.0)
 _show_message("Back on dry land. Your supplies and progress are safe.",2.7)

func _atlas_execute(action:String)->void:
 if action=="v377_rescue":
  _v377_surface()
  _menu_tab("System")
  return
 if action=="v377_help":
  _menu_tab("Tips")
  if is_instance_valid(v376_search):v376_search.call_deferred("grab_focus")
  return
 if action=="v377_status":
  if is_instance_valid(menu_body):menu_body.text="[b]MOONFALL LIVE STATUS[/b]\n\n"+_v377_status()+"\n\nEmergency surfacing: "+("available" if _v377_can_rescue() else "not needed or blocked by danger")
  return
 super._atlas_execute(action)

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["help_overhaul"]="v37.7 contextual help and underwater emergency recovery"
 report["rescue_count"]=v377_rescue_count
 return report
