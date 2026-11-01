extends "res://scripts/native_v38_0.gd"
## First playable realm-choice integration. All locations stay in the same world.
const REALM_ETHICS := preload("res://scripts/systems/morality_reputation.gd")
var realm_ethics:Dictionary = {}
var realm_panel:PanelContainer
var realm_status:Label
var realm_choice_buttons:Array[Button] = []
var realm_quest_buttons:Array[Button] = []
var realm_progress:Label
var realm_victories:int = 0
var realm_elemental_kills:int = 0

func _ready() -> void:
 realm_ethics = REALM_ETHICS.new_state()
 super._ready()
 _build_realm_choice_panel()

func _start_game(selected_hero:String) -> void:
 super._start_game(selected_hero)
 realm_ethics = REALM_ETHICS.new_state()
 realm_victories = 0
 realm_elemental_kills = 0
 _refresh_realm_choices()

func _checkpoint_extra_data() -> Dictionary:
 var data:Dictionary = super._checkpoint_extra_data()
 data["realm_ethics"] = realm_ethics.duplicate(true)
 data["realm_victories"] = realm_victories
 data["realm_elemental_kills"] = realm_elemental_kills
 return data

func _restore_data(data:Dictionary) -> void:
 super._restore_data(data)
 var extra:Variant = data.get("optional_resources", {})
 realm_ethics = REALM_ETHICS.normalize(extra.get("realm_ethics", {}) if extra is Dictionary else {})
 realm_victories = maxi(0,int(extra.get("realm_victories",0))) if extra is Dictionary else 0
 realm_elemental_kills = maxi(0,int(extra.get("realm_elemental_kills",0))) if extra is Dictionary else 0
 _refresh_realm_choices()

func _build_realm_choice_panel() -> void:
 var layer:CanvasLayer = get_node_or_null("NativeAtlasUI") as CanvasLayer
 if not is_instance_valid(layer):
  return
 realm_panel = PanelContainer.new()
 realm_panel.name = "RealmChoicePanel"
 realm_panel.set_anchors_preset(Control.PRESET_TOP_RIGHT)
 realm_panel.offset_left = -330
 realm_panel.offset_right = -16
 realm_panel.offset_top = 160
 realm_panel.offset_bottom = 520
 layer.add_child(realm_panel)
 var layout:VBoxContainer = VBoxContainer.new()
 realm_panel.add_child(layout)
 realm_status = Label.new()
 realm_status.name = "RealmEthicsStatus"
 realm_status.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
 layout.add_child(realm_status)
 realm_progress=Label.new()
 realm_progress.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART
 layout.add_child(realm_progress)
 for entry in [
  {"id":"realm_village_aid","text":"PROTECT THE VILLAGE","moral":35,"factions":{"Villagers":40,"Wardens":20}},
  {"id":"realm_village_betray","text":"BETRAY THE VILLAGE","moral":-40,"factions":{"Villagers":-45,"Outlaws":30}}
 ]:
  var button:Button = Button.new()
  button.text = str(entry["text"])
  button.pressed.connect(_choose_realm_action.bind(entry))
  layout.add_child(button)
  realm_choice_buttons.append(button)
 for quest in [
  {"id":"realm_wardens_oath","text":"QUEST: HELP THE WARDENS","faction":"Wardens","minimum":20,"alignment":"Good","moral":15,"factions":{"Wardens":25,"Villagers":10}},
  {"id":"realm_outlaw_pact","text":"QUEST: JOIN THE OUTLAWS","faction":"Outlaws","minimum":20,"alignment":"Evil","moral":-15,"factions":{"Outlaws":25,"Wardens":-20}}
 ]:
  var quest_button:Button=Button.new()
  quest_button.text=str(quest["text"])
  quest_button.pressed.connect(_complete_realm_quest.bind(quest))
  layout.add_child(quest_button)
  realm_quest_buttons.append(quest_button)
 _refresh_realm_choices()

func _complete_realm_quest(quest:Dictionary)->void:
 if not hero_playing or death_active or menu_open:return
 var id:String=str(quest["id"])
 if not REALM_ETHICS.quest_available(realm_ethics,str(quest["faction"]),int(quest["minimum"]),str(quest["alignment"])):return
 if REALM_ETHICS.normalize(realm_ethics)["choices"].has(id):return
 realm_ethics=REALM_ETHICS.apply_choice(realm_ethics,id,int(quest["moral"]),quest["factions"])
 _show_message("Quest completed: "+str(quest["text"]),3.0)
 _refresh_realm_choices()

func _choose_realm_action(entry:Dictionary) -> void:
 if not hero_playing or death_active or menu_open:
  return
 # The two outcomes belong to one decision; a second choice cannot reverse it.
 var choices:Dictionary = REALM_ETHICS.normalize(realm_ethics)["choices"]
 if choices.has("realm_village_aid") or choices.has("realm_village_betray"):
  return
 realm_ethics = REALM_ETHICS.apply_choice(realm_ethics, str(entry["id"]), int(entry["moral"]), entry["factions"])
 _show_message("The realm remembers: " + str(entry["text"]), 3.0)
 _refresh_realm_choices()

func _refresh_realm_choices() -> void:
 if not is_instance_valid(realm_status):
  return
 var state:Dictionary = REALM_ETHICS.normalize(realm_ethics)
 realm_status.text = "REALM CHOICES | " + REALM_ETHICS.alignment(int(state["morality"])) + "\nVillagers: " + REALM_ETHICS.standing(int(state["factions"].get("Villagers",0))) + " | Wardens: " + REALM_ETHICS.standing(int(state["factions"].get("Wardens",0))) + "\nOutlaws: " + REALM_ETHICS.standing(int(state["factions"].get("Outlaws",0)))
 if is_instance_valid(realm_progress):
  var evolving:Dictionary=REALM_ETHICS.weapon_evolution(state,realm_victories,realm_elemental_kills)
  realm_progress.text="WEAPON: "+str(evolving["alignment"])+" • Rank "+str(evolving["rank"])+" • Bonus "+str(evolving["bonus_percent"])+"% | Companion loyalty: "+str(REALM_ETHICS.companion_loyalty(state,"Good"))+" (Good), "+str(REALM_ETHICS.companion_loyalty(state,"Evil"))+" (Evil)"
 var decided:bool = state["choices"].has("realm_village_aid") or state["choices"].has("realm_village_betray")
 for button in realm_choice_buttons:
  if is_instance_valid(button):
   button.disabled = decided
 for i in range(realm_quest_buttons.size()):
  var quest_id:String="realm_wardens_oath" if i==0 else "realm_outlaw_pact"
  var faction:String="Wardens" if i==0 else "Outlaws"
  var ethic:String="Good" if i==0 else "Evil"
  realm_quest_buttons[i].disabled=state["choices"].has(quest_id) or not REALM_ETHICS.quest_available(state,faction,20,ethic)
 realm_panel.visible = hero_playing and not death_active and not menu_open

func _process(delta:float) -> void:
 super._process(delta)
 if is_instance_valid(realm_panel):
  realm_panel.visible = hero_playing and not death_active and not menu_open
