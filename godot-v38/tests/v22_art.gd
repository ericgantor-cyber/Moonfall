extends SceneTree
var failures:=0
func check(value:bool, info:String) -> void:
 if value: print("PASS v22: "+info)
 else:
  failures+=1
  push_error("FAIL v22: "+info)
func _initialize() -> void:call_deferred("_run")
func _run() -> void:
 var packed:PackedScene=load("res://scenes/main.tscn")
 check(packed!=null,"Main project loads")
 if packed==null:quit(1);return
 var game:Node3D=packed.instantiate()
 root.add_child(game)
 await process_frame
 check(game.get("v22_beast_details").size()==game.get("beasts").size(),"Every creature gets optional anatomy")
 check(game.get("v22_guardian_headdresses").size()==5,"Five roaming bosses have distinct headdresses")
 check(game.get("v22_arena_roots").size()==3,"Three existing world rifts are dressed as arenas")
 check(game.get("v22_arenas")==3,"Rift arena count")
 check(game.get("v22_visual_parts")>=300,"Hundreds of creature details added")
 var species:Dictionary={}
 for i in range(game.get("beasts").size()):
  var beast:Dictionary=game.get("beasts")[i]
  var kind:String=beast["kind"]
  species[kind]=true
  var adorn:Node3D=game.get("v22_beast_details")[i]
  check(adorn.get_child_count()>=3,"Detailed native creature %d" % i)
 check(species.size()==8,"All eight native species represented")
 for i in range(3):
  var arena:Node3D=game.get("v22_arena_roots")[i]
  check(arena.get_child_count()>=15,"Rift %d has arches and decoration" % i)
  check(arena.get_parent()==game.get("decorative_groups")[int(game.get("rifts")[i]["zone"])],"Rift %d culls with zone" % i)
 for item in game.V22_GEAR_TYPES:
  var tex=load(game.V22_GEAR_DIR+item+".png")
  check(tex!=null,"Icon "+item+" imported")
 check(game.call("_icon_category","Moonward Boots")=="boots","Real gear maps to icon category")
 check(game.call("_icon_category","Wolf Fang Charm")=="charm","Crafted gear maps to icon category")
 game.call("_start_game","z")
 game.get("gear_items").append({"name":"Moonward Boots","rarity":"Epic","power":18})
 game.call("_menu_tab","Inventory")
 check(game.get("beast_guide").texture!=null,"Inventory shows item icon")
 var icons:=0
 for child in game.get("menu_buttons").get_children():
  if child is Button and not child.is_queued_for_deletion() and child.text.begins_with("EQUIP:") and child.icon!=null:icons+=1
 check(icons>=1,"Equip buttons have artwork")
 game.call("_apply_graphics_quality","Smooth")
 check(not game.get("v22_beast_details")[0].visible,"Smooth hides nonessential creature detailing")
 game.call("_apply_graphics_quality","Full")
 check(game.get("v22_beast_details")[0].visible,"Full mode restores detailing")
 check(game.get("grid_regions").size()==2400,"Unseen terrain grid unchanged")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Z_Original.webp"),"Z unchanged")
 game.call("_start_game","nyra")
 check(game.get("hero_sprite").texture==load("res://assets/heroes/Nyra_Original.png"),"Nyra unchanged")
 game.queue_free()
 if failures==0:print("MOONFALL v22 ART PASS")
 quit(0 if failures==0 else 1)
