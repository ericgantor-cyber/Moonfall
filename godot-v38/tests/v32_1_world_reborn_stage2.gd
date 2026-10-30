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
 check(RULES.room_visible(2000.0,false) and not RULES.room_visible(9000.0,false),"Chamber LOD culling")
 check(not RULES.room_visible(20.0,true),"Smooth graphics reduces decorative detail")
 check(is_equal_approx(RULES.final_champion_health(100,3,2),165.0),"Final champion health bonus")
 check(is_equal_approx(RULES.final_champion_health(100,2,2),100.0),"Ordinary wave remains balanced")
 var item:Dictionary=RULES.named_relic(0,0,"Fury")
 check(item["rarity"]=="Legendary" and item["name"].contains("Vow"),"Named guaranteed set-themed relic")
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 check(CURRENT_RELEASE.matches_project(game),"New Stage 2 version active")
 check((game.get("rifts") as Array).size()==3,"Exactly three original connected rifts")
 check((game.get("v321_chambers") as Array).size()==9,"Three authored chambers per rift")
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
 for stage in range(1,4):
  entry["stage"]=stage
  game.call("_activate_rift_wave",0)
  var slots:Array=entry["slots"]
  for i in range(slots.size()):
   var enemy:Dictionary=(game.get("beasts") as Array)[int(slots[i])]
   var body:CharacterBody3D=enemy["body"]
   var expected:Vector3=RULES.enemy_position(entry["anchor"],stage,i)+Vector3(0,0.62,0)
   check(body.position.distance_to(expected)<0.01,"Rift wave %d enemy %d moved to its chamber"%[stage,i])
  if stage==3:
   var champion:Dictionary=(game.get("beasts") as Array)[int(slots[2])]
   check(bool(champion.get("dungeon_champion",false)),"Final room has actual champion")
   check(float(champion["max"])>float(champion["base_hp"])*2.0,"Champion health increased above normal final wave")
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
 check(str(report.get("version","")).begins_with("v3") and int(report.get("dungeon_chambers",0))==9,"F9 includes chamber metrics")
 check(int(report.get("dungeon_waves_started",0))>=3 and int(report.get("dungeon_champions_spawned",0))==1,"F9 includes encounter metrics")
 game.queue_free()
 print("MOONFALL v32.1 STAGE 2 PASS" if failures==0 else "MOONFALL v32.1 STAGE 2 FAIL")
 quit(0 if failures==0 else 1)
