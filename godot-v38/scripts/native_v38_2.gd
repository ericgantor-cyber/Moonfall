extends "res://scripts/native_v38_1.gd"
## Coordinate-accurate atlas overlay; existing connected world geometry unchanged.
const NAVIGATION_MAP=preload("res://scripts/systems/navigation_map.gd")
var atlas_panel:PanelContainer
var atlas_chart:Control
var atlas_readout:Label
var atlas_waypoint:Vector3=Vector3.ZERO
var atlas_has_waypoint:bool=false
var atlas_elapsed:float=0.0

func _ready()->void:
 super._ready()
 _create_atlas()

func _start_game(selected_hero:String)->void:
 super._start_game(selected_hero)
 atlas_has_waypoint=false
 _refresh_atlas()

func _checkpoint_extra_data()->Dictionary:
 var data:Dictionary=super._checkpoint_extra_data()
 data["atlas_has_waypoint"]=atlas_has_waypoint
 data["atlas_waypoint"]=[atlas_waypoint.x,atlas_waypoint.y,atlas_waypoint.z]
 return data

func _restore_data(data:Dictionary)->void:
 super._restore_data(data)
 var extras:Variant=data.get("optional_resources",{})
 atlas_has_waypoint=false
 if extras is Dictionary:
  var point:Variant=extras.get("atlas_waypoint",[])
  if point is Array and point.size()==3:
   var x:float=float(point[0])
   var y:float=float(point[1])
   var z:float=float(point[2])
   if is_finite(x) and is_finite(y) and is_finite(z):
    atlas_waypoint=Vector3(x,y,z)
    atlas_has_waypoint=bool(extras.get("atlas_has_waypoint",false))
 _refresh_atlas()

func _create_atlas()->void:
 var layer:CanvasLayer=get_node_or_null("NativeAtlasUI") as CanvasLayer
 if not is_instance_valid(layer):return
 atlas_panel=PanelContainer.new()
 atlas_panel.name="NavigationAtlas"
 atlas_panel.set_anchors_preset(Control.PRESET_TOP_LEFT)
 atlas_panel.position=Vector2(18,175)
 atlas_panel.custom_minimum_size=Vector2(310,340)
 layer.add_child(atlas_panel)
 var layout:VBoxContainer=VBoxContainer.new()
 atlas_panel.add_child(layout)
 var heading:Label=Label.new()
 heading.text="MOONFALL ATLAS | N ↑"
 layout.add_child(heading)
 atlas_chart=NAVIGATION_MAP.new()
 atlas_chart.custom_minimum_size=Vector2(300,240)
 layout.add_child(atlas_chart)
 atlas_readout=Label.new()
 atlas_readout.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART
 layout.add_child(atlas_readout)
 var buttons:HBoxContainer=HBoxContainer.new()
 layout.add_child(buttons)
 var mark:Button=Button.new()
 mark.text="MARK HERE"
 mark.pressed.connect(_atlas_mark_here)
 buttons.add_child(mark)
 var clear:Button=Button.new()
 clear.text="CLEAR"
 clear.pressed.connect(_atlas_clear)
 buttons.add_child(clear)
 var close:Button=Button.new()
 close.text="CLOSE"
 close.pressed.connect(func():atlas_panel.visible=false)
 buttons.add_child(close)
 atlas_panel.visible=false
 _refresh_atlas()

func _atlas_mark_here()->void:
 if not hero_playing or not is_instance_valid(player):return
 atlas_waypoint=player.global_position
 atlas_has_waypoint=true
 _refresh_atlas()

func _atlas_clear()->void:
 atlas_has_waypoint=false
 _refresh_atlas()

func _refresh_atlas()->void:
 if not is_instance_valid(atlas_chart) or not is_instance_valid(atlas_readout):return
 var pos:Vector3=player.global_position if is_instance_valid(player) else Vector3.ZERO
 atlas_chart.update_navigation(pos,atlas_waypoint,atlas_has_waypoint)
 atlas_readout.text="X %.0f | Z %.0f" % [pos.x,pos.z]
 if atlas_has_waypoint:
  var horizontal_distance:float=Vector2(pos.x,pos.z).distance_to(Vector2(atlas_waypoint.x,atlas_waypoint.z))
  atlas_readout.text+=" | Marker: %.0f m" % horizontal_distance

func _unhandled_key_input(event:InputEvent)->void:
 if event is InputEventKey and event.pressed and not event.echo and event.keycode==KEY_F8:
  if is_instance_valid(atlas_panel) and hero_playing and not menu_open and not death_active:
   atlas_panel.visible=not atlas_panel.visible
   _refresh_atlas()
  get_viewport().set_input_as_handled()

func _process(delta:float)->void:
 super._process(delta)
 if not is_instance_valid(atlas_panel):return
 if not hero_playing or death_active or menu_open:
  atlas_panel.visible=false
 if atlas_panel.visible:
  atlas_elapsed+=delta
  if atlas_elapsed>=0.15:
   atlas_elapsed=0.0
   _refresh_atlas()
