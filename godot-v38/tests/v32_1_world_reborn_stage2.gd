extends SceneTree
const CURRENT_RELEASE = preload("res://tests/current_release.gd")
const RULES:=preload("res://scripts/systems/dungeon_stage2_rules.gd")
var failures:int=0
func check(ok:bool,message:String)->void:
 if ok:print("PASS v32.1 stage2 "+message)
 else:
  failures+=1
  push_error("FAIL v32.1 stage2 "+message)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var anchor:=Vector3(10,0,20)
 check(RULES.room_position(anchor,1)==Vector3(-1,0,17),"Room 1 stable position")
 check(RULES.room_position(anchor,3)==Vector3(10,0,31),"Room 3 stable position")
 check(RULES.enemy_position(anchor,2,0).distance_to(RULES.room_position(anchor,2))<3.0,"Three chamber spawn layout")
 check(RULES.chamber_title(3)=="Guardian Heart","Named final chamber")
 check(RULES.chamber_count(0,0)>=5 and RULES.chamber_count(0,0)<=14,"Exploration chamber count stays in requested range")
 # Existing site/zone combinations must keep their exact save-stable chamber counts.
 var legacy_counts_stable:bool=true
 for site_id in range(30):
  for zone_id in range(6):
   var expected_count:int=5+posmod(site_id*7+zone_id*3,10)
   if RULES.chamber_count(site_id,zone_id)!=expected_count:
    legacy_counts_stable=false
    break
  if not legacy_counts_stable:break
 check(legacy_counts_stable,"All 180 standard site/zone chamber counts remain unchanged")
 # Modulo-equivalent large identifiers cannot overflow the count calculation.
 var extreme_counts_stable:bool=true
 for site_id in [2147483647,-2147483648,1000000000000,-1000000000000]:
  for zone_id in [2147483647,-2147483648,1000000000000,-1000000000000]:
   var count:int=RULES.chamber_count(site_id,zone_id)
   var reduced_count:int=RULES.chamber_count(posmod(site_id,10),posmod(zone_id,10))
   if count!=reduced_count or count<5 or count>14:
    extreme_counts_stable=false
    break
  if not extreme_counts_stable:break
 check(extreme_counts_stable,"Extreme dungeon IDs stay bounded and agree with their small modulo equivalents")

 check(RULES.room_position(anchor,4)!=RULES.room_position(anchor,3),"Exploration passage connects distinct chamber sites")
 check(RULES.room_visible(2000.0,false) and not RULES.room_visible(9000.0,false),"Chamber LOD culling")
 check(not RULES.room_visible(20.0,true),"Smooth graphics reduces decorative detail")
 check(not RULES.room_visible(-0.1,false),"Negative squared distance cannot display dungeon details")
 check(not RULES.room_visible(NAN,false) and not RULES.room_visible(INF,false) and not RULES.room_visible(-INF,false),"Nonfinite squared distance is always culled")
 check(RULES.room_visible(0.0,false) and RULES.room_visible(5624.0,false) and not RULES.room_visible(5625.0,false),"Valid chamber LOD distance boundary is unchanged")
 check(is_equal_approx(RULES.final_champion_health(100,3,2),165.0),"Final champion health bonus")
 check(is_equal_approx(RULES.final_champion_health(100,2,2),100.0),"Ordinary wave remains balanced")
 check(RULES.final_champion_health(-10.0,3,2)==0.0 and RULES.final_champion_health(0.0,3,2)==0.0,"Negative and zero champion HP are rejected")
 check(RULES.final_champion_health(NAN,3,2)==0.0 and RULES.final_champion_health(INF,3,2)==0.0 and RULES.final_champion_health(-INF,2,1)==0.0,"Nonfinite champion HP is rejected")
 var giant_hp:float=1.0e308
 check(RULES.final_champion_health(giant_hp,3,2)==giant_hp,"Extreme but finite champion HP cannot overflow")
 check(is_equal_approx(RULES.final_champion_health(100.0,3,1),100.0) and is_equal_approx(RULES.final_champion_health(100.0,2,2),100.0),"Only final champion receives health multiplier")
 var item:Dictionary=RULES.named_relic(0,0,"Fury")
 check(item["rarity"]=="Legendary" and item["name"].contains("Vow"),"Named guaranteed set-themed relic")
 for zone_id in [0,1,5,16,17,18,1000,-1,-999,2147483647,-2147483648]:
  var relic:Dictionary=RULES.named_relic(1,zone_id,"Fury")
  var bounded_zone:int=clampi(zone_id,0,17)
  var expected_power:int=mini(250,48+bounded_zone*12)
  check(int(relic["power"])==expected_power and expected_power>=48 and expected_power<=250,"Relic power remains bounded for zone "+str(zone_id))
 check(int(RULES.named_relic(0,0,"Fury")["power"])==48 and int(RULES.named_relic(2,5,"Fury")["power"])==108,"Normal dungeon relic power remains unchanged")
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 check(CURRENT_RELEASE.matches_project(game),"New Stage 2 version active")
 check((game.get("rifts") as Array).size()==30,"Thirty connected rifts, retaining original site IDs")
 check((game.get("v321_chambers") as Array).size()>=150 and (game.get("v321_chambers") as Array).size()<=420,"Five to fourteen chambers in each of thirty rifts")
 check((game.get("region_nodes") as Array).size()==6,"Six original world regions")
 check((game.get("v320_wardens") as Array).size()==6,"Existing NPCs preserved")
 game.call("_start_game","z")
 await process_frame
 var entry:Dictionary=(game.get("rifts") as Array)[0]
 var zone:int=int(entry["zone"])
 var chest_count:int=(game.get("treasure_chests") as Array).size()
 check(chest_count==6,"Original treasure chests preserved")
 game.set("active_rift",0)
 entry["active"]=true
 # The active game uses one continuous encounter, not three wave resets.
 entry["stage"]=1
 game.call("_activate_rift_wave",0)
 check(int(entry["stage"])==3,"Legacy completion sentinel set without wave progression")
 var resident_slots:Array=entry["slots"]
 for i in range(resident_slots.size()):
  var enemy:Dictionary=(game.get("beasts") as Array)[int(resident_slots[i])]
  var body:CharacterBody3D=enemy["body"]
  var expected:Vector3=RULES.enemy_position(entry["anchor"],i+1,i)+Vector3(0,0.62,0)
  check(body.position.distance_to(expected)<0.01,"Resident %d starts in a distinct chamber"%i)
 var champion:Dictionary=(game.get("beasts") as Array)[int(resident_slots[2])]
 check(bool(champion.get("dungeon_champion",false)),"Guardian chamber has actual champion")
 check(float(champion["max"])>float(champion["base_hp"])*2.0,"Champion health exceeds ordinary resident")
 var before_drops:int=(game.get("gear_drops") as Array).size()
 var slots:Array=entry["slots"]
 for slot in slots:
  var enemy:Dictionary=(game.get("beasts") as Array)[int(slot)]
  game.call("_despawn_native_beast",enemy,0.0)
 var defeated:Dictionary=(game.get("beasts") as Array)[int(slots[2])]
 game.call("_on_rift_enemy_defeated",defeated)
 check(bool(entry["cleared"]),"Rift seals after final room")
 check((game.get("rifts_cleared") as Array).has(0),"Seal recorded in existing save-compatible progress")
 check(int(game.get("v321_bonus_relics"))==1,"Exactly one bonus legendary granted")
 var new_drops:int=(game.get("gear_drops") as Array).size()
 check(new_drops>=before_drops+2,"Original relic plus stage2 reward")
 game.call("_on_rift_enemy_defeated",defeated)
 check(int(game.get("v321_bonus_relics"))==1,"Repeat completion cannot duplicate bonus")
 game.call("_save_game")
 var saved:Dictionary=game.call("_read_save")
 check(int(saved.get("schema",-1))==2 and not (saved.get("rifts",[]) as Array).is_empty() and int((saved.get("rifts",[]) as Array)[0])==0,"Schema-2 saved rift progress unchanged")
 var report:Dictionary=game.call("_perf_report")
 check(str(report.get("version","")).begins_with("v3") and int(report.get("dungeon_chambers",0))==(game.get("v321_chambers") as Array).size(),"Debug metrics count every explorable chamber")
 check(int(report.get("dungeon_waves_started",0))==1 and int(report.get("dungeon_champions_spawned",0))==1,"One continuous encounter and one champion recorded")
 game.queue_free()
 print("MOONFALL v32.1 STAGE 2 PASS" if failures==0 else "MOONFALL v32.1 STAGE 2 FAIL")
 quit(0 if failures==0 else 1)
