extends SceneTree
## Optional headless Godot runtime check for Moon Sigil crafting and enchanted inventory.
var failures:int=0
func check(ok:bool,description:String)->void:
 if ok:print("PASS moon sigil: "+description)
 else:
  failures+=1
  push_error("FAIL moon sigil: "+description)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 game.call("_start_game","z")
 # Deterministically keep combat away from the crafting test.
 for group in (game.get("zone_beast_lists") as Array):
  for creature in (group as Array):
   creature["alive"]=false
 var full:Dictionary={"Moon Sigil":90,"Dusk Cinder":50,"Elder Rootstone":50,"Serpent Scale":40,"Fine Hide":40}
 game.set("materials",full)
 game.call("_moon_craft","blade")
 var gear:Array=game.get("gear_items")
 check(gear.size()==1,"legendary blade crafted")
 check(str(gear[0].get("rarity",""))=="Legendary" and int(gear[0].get("power",0))==64,"legendary power and rarity")
 check(int((game.get("materials") as Dictionary).get("Moon Sigil",0))==85,"recipe consumes exactly five Moon Sigils")
 game.call("_equip",0)
 game.call("_moon_enchant",0)
 gear=game.get("gear_items")
 check(int(gear[0].get("moon_enchant_rank",-1))==1 and int(gear[0].get("power",0))==70,"first rank adds six power")
 check(int((game.get("equipped") as Dictionary).get("power",0))==70,"equipped copy stays synchronized")
 game.call("_moon_enchant",0)
 game.call("_moon_enchant",0)
 game.call("_moon_enchant",0)
 check(int((game.get("gear_items") as Array)[0].get("moon_enchant_rank",-1))==3,"enchantment stops at rank three")
 game.call("_moon_craft","oath")
 check(int((game.get("materials") as Dictionary).get("Moonward Oath",0))==1,"one-time permanent oath crafted")
 var old_sigils:int=int((game.get("materials") as Dictionary).get("Moon Sigil",0))
 game.call("_moon_craft","oath")
 check(int((game.get("materials") as Dictionary).get("Moon Sigil",0))==old_sigils,"oath cannot be crafted twice")
 game.call("_moon_craft","feather")
 game.call("_moon_craft","feather")
 check(int((game.get("materials") as Dictionary).get("Moon Phoenix Feather",0))==1,"Phoenix Feather crafting capped at one held")
 game.set("phoenix_grace_until",0.0)
 game.call("_apply_incoming_damage",100000.0)
 check(not bool(game.get("death_active")),"Phoenix Feather prevents the fatal blow")
 check(is_equal_approx(float(game.get("hp")),float(game.get("max_hp"))*0.45),"Moonward Oath buffs Phoenix revival to 45 percent")
 var data:Dictionary=game.call("_read_save")
 check(int((data.get("materials",{}) as Dictionary).get("Moonward Oath",0))==1,"oath survives checkpoint")
 check(int(((data.get("gear",[]) as Array)[0] as Dictionary).get("moon_enchant_rank",-1))==3,"gear enchant survives checkpoint")
 game.queue_free()
 print("MOON SIGIL FORGE PASS" if failures==0 else "MOON SIGIL FORGE FAIL")
 quit(0 if failures==0 else 1)
