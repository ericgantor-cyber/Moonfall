extends Control
## Compact round in-world map; no artwork assets and no persistent map state.
var game: Node3D
var redraw_clock := 0.0
const CENTER := Vector2(126,121)
const RADIUS := 104.0
const MAP_SCALE := 0.90

func _process(delta: float) -> void:
 redraw_clock += delta
 if redraw_clock < 0.14:return
 redraw_clock = fmod(redraw_clock,0.14)
 if is_instance_valid(game) and game.get("hero_playing"):queue_redraw()

func _plot(world_position: Vector2) -> Vector2:
 return CENTER + world_position * MAP_SCALE

func _inside(point: Vector2) -> bool:
 return point.distance_to(CENTER) < RADIUS-7.0

func _draw() -> void:
 if not is_instance_valid(game):return
 draw_circle(CENTER, RADIUS+5, Color(0.60,0.42,0.86,0.72))
 draw_circle(CENTER, RADIUS, Color(0.035,0.065,0.105,0.94))
 draw_arc(CENTER,RADIUS-5,0.0,TAU,96,Color(0.78,0.66,1.0,0.82),1.6,true)
 var regions: Array = game.get("REGIONS")
 for idx in range(regions.size()):
  var region:Dictionary=regions[idx]
  var at:Vector2=region["at"]
  var mark:Vector2=_plot(at)
  if not _inside(mark):continue
  var region_color:Color=Color(str(region["light"]))
  region_color.a=0.17
  draw_circle(mark,21.0,region_color)
  draw_arc(mark,20.0,0,TAU,32,Color(0.52,0.60,0.72,0.38),1.0)
  var level_label:String=str(idx+1)
  draw_string(get_theme_default_font(),mark+Vector2(-4.0,4.0),level_label,HORIZONTAL_ALIGNMENT_LEFT,-1,12,Color(0.92,0.92,0.96))
 var unlocked:Array=game.get("shrine_discovery")
 for index in unlocked:
  var loc:Vector2=regions[int(index)]["at"]
  var plotted:Vector2=_plot(loc+Vector2(4.0,2.0))
  if _inside(plotted):draw_circle(plotted,4.0,Color(0.36,0.93,1.0))
 var rifts:Array=game.get("rifts")
 for entry in rifts:
  var site:Vector3=entry["anchor"]
  var marked:Vector2=_plot(Vector2(site.x,site.z))
  if _inside(marked):draw_circle(marked,4.5,Color(0.5,0.95,0.74) if bool(entry["cleared"]) else Color(0.85,0.32,0.92))
 var events:Array=game.get("world_events")
 for entry in events:
  if bool(entry.get("completed",false)):continue
  var anchor:Vector3=entry["anchor"]
  var dot:Vector2=_plot(Vector2(anchor.x,anchor.z))
  if _inside(dot):draw_circle(dot,3.0,Color(1.0,0.80,0.38))
 var actor:CharacterBody3D=game.get("player")
 if is_instance_valid(actor):
  var marker:Vector2=_plot(Vector2(actor.position.x,actor.position.z))
  if _inside(marker):
   draw_circle(marker,6.0,Color(0.08,0.15,0.22))
   draw_circle(marker,4.5,Color(1.0,0.99,1.0))
 draw_string(get_theme_default_font(),Vector2(20,243),"MOONFALL  •  WORLD MAP",HORIZONTAL_ALIGNMENT_LEFT,-1,13,Color(0.90,0.84,1.0))
