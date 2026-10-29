extends SceneTree
const TIPS:=preload("res://scripts/systems/tips_codex_v376.gd")
var failures:int=0
func _initialize()->void:call_deferred("_run")
func check(ok:bool,label:String)->void:
 if ok:print("PASS tips: "+label)
 else:
  failures+=1
  push_error("FAIL tips: "+label)
func _run()->void:
 check(TIPS.CATEGORIES.size()>=12,"all major game categories exist")
 check(TIPS.ENTRIES.size()>=100,"100+ useful tips available")
 check(TIPS.matching("Water & Diving").size()>=8,"water and oxygen topics covered")
 check(TIPS.matching("All","oxygen").size()>=3,"keyword search filters actual oxygen tips")
 check(TIPS.matching("All","unlikely_nonexistent_phrase").is_empty(),"unmatched search is safe")
 check(TIPS.action_tip("craft:1","")!="","craft actions explain costs")
 check(TIPS.action_tip("inventory_deposit:0","")!="","bag actions have hover descriptions")
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"game scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 var tips_button:Button=game.get("v376_help_tab") as Button
 var search:LineEdit=game.get("v376_search") as LineEdit
 check(is_instance_valid(tips_button) and tips_button.text=="Tips","Raven Atlas exposes Tips tab")
 check(is_instance_valid(search),"Atlas has dedicated Tips search")
 # Simulate actual play: the live-search handler redraws only while Atlas is open.
 game.call("_start_game","z")
 game.call("_toggle_menu")
 check(game.get("menu_open"),"Atlas opens during gameplay")
 game.call("_menu_tab","Tips")
 var codex_text:String=(game.get("menu_body") as RichTextLabel).text
 check("TIPS CODEX" in codex_text,"Tips panel displays the guide")
 check(is_instance_valid(search) and search.visible,"search field shown only on Tips tab")
 # Programmatic text assignment does not emit LineEdit.text_changed in Godot.
 # Emit the real connected UI signal instead of directly calling the handler.
 if is_instance_valid(search):
  search.text="oxygen"
  search.text_changed.emit("oxygen")
 check(game.get("v376_query")=="oxygen","Tips search input updates the active query")
 check("OXYGEN" in (game.get("menu_body") as RichTextLabel).text.to_upper(),"live search displays matching entries")
 game.call("_atlas_execute","tipcat:Water & Diving")
 check(game.get("v376_topic")=="Water & Diving","topic filter buttons work")
 game.call("_menu_tab","Character")
 check(not search.visible,"Tips search hidden outside Tips tab")
 game.call("_toggle_menu")
 check(not game.get("menu_open"),"Atlas closes after searching")
 game.queue_free()
 print("MOONFALL v37.6 ALL TIPS PASS" if failures==0 else "MOONFALL v37.6 ALL TIPS FAIL")
 quit(0 if failures==0 else 1)
