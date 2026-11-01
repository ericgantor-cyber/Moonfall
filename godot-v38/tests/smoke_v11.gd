extends SceneTree
## Run via: godot --headless --path PROJECT --script res://tests/smoke_v11.gd
## No save files are written or replaced by this check.
var errors := 0

func _check(ok:bool, description:String) -> void:
 if ok:
  print("PASS: "+description)
 else:
  push_error("SMOKE FAIL: "+description)
  errors+=1

func _initialize() -> void:
 call_deferred("_run")

func _run() -> void:
 var resource:PackedScene=load("res://scenes/main.tscn")
 if resource==null:
  push_error("SMOKE FAIL: main scene could not load")
  quit(1)
  return
 var game:Node3D=resource.instantiate()
 root.add_child(game)
 await process_frame
 var regions:Array=game.get("region_nodes")
 var lists:Array=game.get("zone_beast_lists")
 _check(regions.size()==6,"Six connected 3D sectors")
 _check(lists.size()==6,"Six regional AI lists")
 if regions.size()!=6 or lists.size()!=6:
  quit(1)
  return
 var total:=0
 for zone in range(6):
  total+=lists[zone].size()
  _check(regions[zone].get_child_count()>10,"Sector "+str(zone)+" scenery grouped")
 _check(total==game.get("beasts").size(),"Each creature mapped to exactly one sector")
 _check(total==74,"48 wildlife, 5 guardians, 9 rift slots, 12 lunar invasion slots (actual="+str(total)+")")
 var events:Array=game.get("world_events")
 _check(events.size()==6,"Six regional living-world events")
 _check(game.get("events_completed").is_empty(),"Events begin unclaimed")
 _check(game.get("gear_drops").is_empty(),"Fresh world has no leftover loot")
 _check(game.call("_day_phase")=="Day","Adventure begins during daylight")
 game.set("world_clock",400.0)
 _check(game.call("_day_phase")=="Day","Longer cycle retains daylight at 400 seconds")
 game.set("world_clock",960.0*0.85)
 _check(game.call("_day_phase")=="Dusk" or game.call("_day_phase")=="Night","Day/night phase changes")
 game.set("weather_remaining",0.1)
 game.call("_advance_living_world",0.2)
 _check(game.get("weather_name")!="Clear","Weather cycle changes conditions")
 game.call("_refresh_world_lighting")
 # No save writes: gameplay has not started yet.
 game.call("_complete_world_event",0)
 _check(0 in game.get("events_completed"),"Supply cache grants completion")
 var cache_gold:int=game.get("gold")
 game.call("_complete_world_event",0)
 _check(game.get("gold")==cache_gold,"Supply cache cannot be claimed twice")
 game.set("active_world_event",1)
 events[1]["active"]=true
 for n in range(3):
  game.call("_world_event_on_defeat",{"zone":1,"hostile":true,"dungeon_site":-1})
 _check(1 in game.get("events_completed") and game.get("active_world_event")==-1,"Regional hunt completes after three hostiles")
 var retreat_target:Dictionary=game.get("beasts")[0]
 retreat_target["hp"]=float(retreat_target["max"])*0.15
 game.call("_apply_player_hit",retreat_target,0.0,0.0)
 _check(float(retreat_target.get("retreat_until",0.0))>0.0,"Hurt regular creature triggers timed retreat")
 game.call("_reset_native_beast",retreat_target)
 _check(float(retreat_target.get("retreat_until",1.0))==0.0,"Respawn resets retreat state")
 var sites:Array=game.get("rifts")
 _check(sites.size()==3,"Three open-world rift encounter sites")
 for site in sites:
  _check(site["slots"].size()==3,"Each rift reuses exactly three combat slots")
  for slot in site["slots"]:
   _check(not game.get("beasts")[int(slot)]["alive"],"Rift slots dormant until activation")
 var hero_billboard:Sprite3D=game.get("hero_sprite")
 _check(is_instance_valid(hero_billboard) and hero_billboard.texture!=null,"Original hero billboard texture loads")
 _check(game.get("visible_regions")>=1,"Initial outpost is visible")
 _check(not regions[5].visible,"Distant citadel scenery is culled")
 game.call("_start_game","z")
 _check(game.get("hero_playing"),"Start hero gameplay")
 _check(game.get("hero")=="z","Original Z hero selection")
 # Simulate switching heroes: new-game stats must not carry over.
 game.set("gold",777)
 game.set("tamer_points",9)
 game.set("materials",{"fang":99})
 game.call("_start_game","nyra")
 _check(game.get("hero")=="z","Custom adventurer uses legacy combat slot while Nyra remains boss only")
 _check(game.get("hero_sprite").texture!=null,"Nyra artwork loads without modification")
 _check(game.get("gold")==0 and game.get("tamer_points")==0,"Hero selection clears previous run progression")
 _check(game.get("materials").has("herb") and not game.get("materials").has("fang"),"Starter materials restored for new game")
 _check(game.get("kills_total")==0,"Enemy kill progression resets")
 _check(game.get("hero_points")==0 and game.get("hero_perks").is_empty(),"Hero skill tree resets")
 _check(game.get("rifts_cleared").is_empty(),"Rift progress resets")
 _check(game.get("weather_name")=="Clear" and game.get("world_day")==1,"New adventure resets climate and clock")
 _check(game.get("events_completed").is_empty() and game.get("active_world_event")==-1,"New adventure resets regional events")
 var old_hero_hp:float=game.get("max_hp")
 game.set("hero_points",2)
 game.set("hero_perks",{"ward":1})
 _check(game.call("_perk_rank","ward")==1,"Hero skill bonuses read correctly")
 # Tests do not invoke `_save_game` or purchase actions.
 game.call("_start_game","nyra")
 _check(game.get("hero_perks").is_empty() and game.get("max_hp")==old_hero_hp,"New-game skill reset")
 var first_rift:Dictionary=sites[0]
 var rift_player:CharacterBody3D=game.get("player")
 rift_player.position=first_rift["anchor"]+Vector3(0,0.85,0)
 game.set("region_index",1)
 # Clear incidental nearby wildlife so the rift guard does not reject the test.
 for creature in lists[1]:
  if int(creature.get("dungeon_site",-1))<0 and not bool(creature.get("boss",false)):
   game.call("_despawn_native_beast",creature,100.0)
 game.call("_start_rift",0)
 _check(game.get("active_rift")==0 and int(first_rift["stage"])==1,"Rift starts at its in-world altar")
 for slot in first_rift["slots"]:
  _check(bool(game.get("beasts")[int(slot)]["alive"]),"Wave enemy awakens")
 # Resolve all three waves without actually killing actors or writing a checkpoint.
 for wave in range(1,4):
  var final_enemy:Dictionary={}
  for slot in first_rift["slots"]:
   var unit:Dictionary=game.get("beasts")[int(slot)]
   game.call("_despawn_native_beast",unit,0.0)
   final_enemy=unit
  game.call("_on_rift_enemy_defeated",final_enemy)
  if wave<3:
   _check(int(first_rift["stage"])==wave+1,"Rift progresses to wave "+str(wave+1))
  else:
   _check(bool(first_rift["cleared"]) and 0 in game.get("rifts_cleared"),"Rift seals and awards progress")
 var legendary_found:=false
 for drop in game.get("gear_drops"):
  if str(drop["data"].get("rarity",""))=="Legendary" and str(drop["data"].get("affix",""))=="Fury":
   legendary_found=true
   break
 _check(legendary_found,"Rift reward includes named affixed legendary")
 var test_boss:Dictionary={}
 for creature in game.get("beasts"):
  if bool(creature.get("boss",false)):
   test_boss=creature
   break
 if not test_boss.is_empty():
  test_boss["hp"]=float(test_boss["max"])*0.55
  game.call("_refresh_boss_phase",test_boss)
  _check(int(test_boss["boss_phase"])==2,"Guardian phase 2 after 67% HP")
  test_boss["hp"]=float(test_boss["max"])*0.25
  game.call("_refresh_boss_phase",test_boss)
  _check(int(test_boss["boss_phase"])==3,"Guardian phase 3 after 32% HP")
 game.call("_start_game","nyra")
 _check(game.get("active_rift")==-1 and not bool(game.get("beasts")[int(first_rift["slots"][0])]["alive"]),"New game clears ongoing rift")
 var character:CharacterBody3D=game.get("player")
 character.position=Vector3(60.0,0.85,30.0)
 game.call("_update_region_visibility")
 _check(regions[5].visible,"Citadel renders after travel")
 _check(not regions[0].visible,"Far away outpost scenery is hidden")
 var solid_found:=false
 for prop in regions[0].get_children():
  if prop is StaticBody3D:
   for shape in prop.get_children():
    if shape is CollisionShape3D and not shape.disabled:
     solid_found=true
     break
  if solid_found:break
 _check(solid_found,"Distant sector retains active collision shapes")
 character.position=Vector3(-30.0,0.85,-30.0)
 game.call("_update_region_visibility")
 _check(regions[0].visible and regions[1].visible,"Both sides of a sector boundary render")
 character.position=Vector3(-60.0,0.85,-29.0)
 game.call("_update_region_visibility")
 # Test the world-event label state without invoking save-writing actions.
 var first_event:Dictionary=events[0]
 first_event["completed"]=true
 game.call("_refresh_event_label",first_event)
 var event_label:Label3D=first_event["label"]
 _check("COMPLETE" in event_label.text,"Event marker updates after completion")
 game.call("_start_game","z")
 _check(not bool(first_event["completed"]),"Starting over clears claimed world events")
 _check(game.get("save_health")!="COMMIT FAILED","Smoke test has not changed saves")
 # Exercise scene UI only, without writing user:// or invoking save commands.
 game.call("_toggle_menu")
 _check(game.get("menu_open"),"Raven Atlas opens")
 game.call("_toggle_menu")
 _check(not game.get("menu_open"),"Raven Atlas closes")
 if errors==0:
  print("MOONFALL NATIVE v11 SMOKE PASS")
 else:
  push_error("MOONFALL NATIVE v11 SMOKE FAILED "+str(errors)+" checks")
 game.queue_free()
 quit(1 if errors>0 else 0)
