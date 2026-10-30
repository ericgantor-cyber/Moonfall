extends SceneTree
const CURRENT_RELEASE = preload("res://tests/current_release.gd")
var failures:int=0
func check(pass_test:bool,description:String)->void:
 if not pass_test:
  failures+=1
  push_error("FAIL ART: "+description)
 else:print("PASS ART: "+description)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 for name in ["rift_warning","moon_surge","treasure_cache","treasure_bearer","forge","boss_phase","rare_loot","dungeon_seal"]:
  var texture:Texture2D=load("res://assets/v31_3_art/"+name+".png")
  check(texture!=null and texture.get_width()==512,"Asset imported: "+name)
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 check(CURRENT_RELEASE.matches_project(game),"Legacy version and saves retained")
 var chests:Array=game.get("v313_art_chests")
 var traps:Array=game.get("v313_art_traps")
 check(chests.size()==6,"Six chest emblems created")
 check(traps.size()==3,"Three rift warning images installed")
 check(is_instance_valid(game.get("v313_art_surge")),"Moon surge HUD emblem created")
 check(game.get("v312_foliage").size()==6,"Six world regions preserved")
 check((game.get("v313_art_dungeon") as Array).size()==3,"Three dungeon seal signs")
 check(is_instance_valid(game.get("v313_art_forge")),"Hearthstone forge sign")
 var entry:Dictionary=(game.get("treasure_chests") as Array)[0]
 (game.get("opened_treasure_chests") as Array).append(0)
 game.call("_refresh_treasure_chests")
 check(not (chests[0] as Sprite3D).visible,"Claimed chest marker hides")
 game.call("_start_game","z")
 await process_frame
 game.set("map_time",166.0)
 game.call("_v313_tick_surge")
 var icon:TextureRect=game.get("v313_art_surge")
 check(icon.visible,"Live Moon Surge HUD art appears")
 game.set("map_time",200.0)
 game.call("_v313_tick_surge")
 check(not icon.visible,"Moon Surge HUD clears after event")
 var drop_count:int=(game.get("gear_drops") as Array).size()
 game.call("_create_gear_drop",Vector3(0,0,0),"Relic","elder",0,{"name":"Test Legendary","rarity":"Mythic","power":50})
 var drops:Array=game.get("gear_drops")
 check(drops.size()==drop_count+1,"Mythic loot still spawns normally")
 if drops.size()>drop_count:
  check((drops[-1]["node"] as Node3D).get_node_or_null("Art_RareLootGlyph")!=null,"Rare loot gets unique artwork")
 check((game.call("_perf_report") as Dictionary).has("art_rift_glyphs"),"Profiler includes art counters")
 game.queue_free()
 print("MOONFALL v31.3 ART PASS" if failures==0 else "MOONFALL v31.3 ART FAIL")
 quit(0 if failures==0 else 1)
