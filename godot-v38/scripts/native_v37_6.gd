extends "res://scripts/native_v37_4.gd"


# v37.6 in-game Tips Codex. All help text is one shared data table, never
# rebuilt per frame. Original character and world textures are untouched.
const V376_TIPS:=preload("res://scripts/systems/tips_codex_v376.gd")
var v376_topic:String="All"
var v376_query:String=""
var v376_page:int=0
var v376_search:LineEdit
var v376_help_tab:Button

func _build_ui()->void:
 super._build_ui()
 # Attach to the existing Raven Atlas tab row, no second fullscreen menu.
 if not is_instance_valid(menu_panel):return
 var tabs:HBoxContainer
 for candidate_node in menu_panel.find_children("*","Button",true,false):
  var candidate:Button=candidate_node as Button
  if not is_instance_valid(candidate):continue
  if candidate.text=="System" and candidate.get_parent() is HBoxContainer:
   tabs=candidate.get_parent() as HBoxContainer
  if candidate.text in V376_TIPS.CATEGORIES:
   candidate.tooltip_text=V376_TIPS.menu_tip(candidate.text)
 if is_instance_valid(tabs):
  tabs.add_theme_constant_override("separation",3)
  for child in tabs.get_children():
   if child is Button:child.custom_minimum_size.x=80.0
  v376_help_tab=Button.new()
  v376_help_tab.name="MoonfallTipsTab"
  v376_help_tab.text="Tips"
  v376_help_tab.custom_minimum_size=Vector2(57,43)
  v376_help_tab.tooltip_text="Search tips for the entire game (F1)."
  v376_help_tab.pressed.connect(_menu_tab.bind("Tips"))
  tabs.add_child(v376_help_tab)
 if is_instance_valid(inventory_search_input):
  v376_search=LineEdit.new()
  v376_search.name="MoonfallTipsSearch"
  v376_search.placeholder_text="Search combat, oxygen, crafting..."
  v376_search.custom_minimum_size=Vector2(295,34)
  v376_search.max_length=80
  v376_search.visible=false
  v376_search.tooltip_text="Type any word to filter the entire Tips Codex."
  inventory_search_input.get_parent().add_child(v376_search)
  v376_search.text_changed.connect(_v376_search_changed)
 # Hover descriptions work for the start screen and quick-action buttons too.
 if is_instance_valid(quick_actions):
  for action_node in quick_actions.find_children("*","Button",true,false):
   var action_button:Button=action_node as Button
   if not is_instance_valid(action_button):continue
   if action_button.text.begins_with("INTERACT"):action_button.tooltip_text=V376_TIPS.action_tip("interact","")
   elif "DODGE" in action_button.text:action_button.tooltip_text=V376_TIPS.action_tip("dodge","")
   elif "SAVE" in action_button.text:action_button.tooltip_text=V376_TIPS.action_tip("save","")
   elif "STANCE" in action_button.text:action_button.tooltip_text=V376_TIPS.action_tip("stance","")
   elif "TAME" in action_button.text:action_button.tooltip_text="Attempt to tame an eligible nearby animal."
 if is_instance_valid(start_screen):
  for choice_node in start_screen.find_children("*","Button",true,false):
   var choice:Button=choice_node as Button
   if not is_instance_valid(choice):continue
   if choice.text.begins_with("NEW GAME"):choice.tooltip_text="Start a new adventure with this hero. Original character artwork is preserved."
   elif choice.text.begins_with("CONTINUE"):choice.tooltip_text="Load your last saved checkpoint."

func _atlas_action(label:String,action:String,tip:String="")->void:
 var description:String=tip if tip!="" else V376_TIPS.action_tip(action,label)
 super._atlas_action(label,action,description)

func _unhandled_key_input(event:InputEvent)->void:
 if event is InputEventKey and event.pressed and not event.echo and event.keycode==KEY_F1 and hero_playing:
  if menu_open and current_tab=="Tips":
   _toggle_menu()
  else:
   if not menu_open:_toggle_menu()
   _menu_tab("Tips")
   if is_instance_valid(v376_search):v376_search.call_deferred("grab_focus")
  get_viewport().set_input_as_handled()
  return
 super._unhandled_key_input(event)

func _v376_search_changed(value:String)->void:
 v376_query=value.substr(0,80)
 v376_page=0
 if menu_open and current_tab=="Tips":_v376_render()

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if is_instance_valid(v376_search):v376_search.visible=which=="Tips"
 if which=="Tips":_v376_render()

func _v376_render()->void:
 if not is_instance_valid(menu_body) or not is_instance_valid(menu_buttons):return
 # Reset action buttons only, not the existing search control or other tabs.
 for child in menu_buttons.get_children():child.queue_free()
 var hits:Array[Dictionary]=V376_TIPS.matching(v376_topic,v376_query)
 var pages:int=maxi(1,ceili(float(hits.size())/float(V376_TIPS.PAGE_SIZE)))
 v376_page=clampi(v376_page,0,pages-1)
 var output:String="[b]MOONFALL • TIPS CODEX[/b]\nF1 opens or closes this guide.\nSearch by keyword, or pick a category at left.\n"
 output+="[b]Topic:[/b] "+v376_topic+"   •   [b]Results:[/b] "+str(hits.size())+"   •   [b]Page:[/b] "+str(v376_page+1)+"/"+str(pages)+"\n\n"
 if hits.is_empty():output+="No tips match. Try another term or choose All.\n"
 for n in range(v376_page*V376_TIPS.PAGE_SIZE,mini((v376_page+1)*V376_TIPS.PAGE_SIZE,hits.size())):
  var item:Dictionary=hits[n]
  output+="[color=#a8dcea]"+str(item["category"])+"[/color] • [b]"+str(item["title"])+"[/b]\n"+str(item["tip"])+"\n\n"
 menu_body.text=output
 _atlas_action("ALL TOPICS"+(" ✓" if v376_topic=="All" else ""),"tipcat:All")
 for category in V376_TIPS.CATEGORIES:
  if category=="All":continue
  _atlas_action(category+(" ✓" if v376_topic==category else ""),"tipcat:"+category)
 if v376_page>0:_atlas_action("← PREVIOUS TIPS","tippage:prev")
 if v376_page+1<pages:_atlas_action("NEXT TIPS →","tippage:next")

func _atlas_execute(action:String)->void:
 if action.begins_with("tipcat:"):
  var proposed:String=action.trim_prefix("tipcat:")
  if proposed in V376_TIPS.CATEGORIES:
   v376_topic=proposed
   v376_page=0
   _v376_render()
  return
 if action=="tippage:prev":
  v376_page=maxi(0,v376_page-1)
  _v376_render()
  return
 if action=="tippage:next":
  v376_page+=1
  _v376_render()
  return
 super._atlas_execute(action)
