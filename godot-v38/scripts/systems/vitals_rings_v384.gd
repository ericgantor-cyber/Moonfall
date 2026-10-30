extends Control
## Lightweight, texture-free vitals rings; updated only by the existing 4Hz HUD tick.
var hp_ratio:float=1.0
var xp_ratio:float=0.0

func _ready()->void:
 custom_minimum_size=Vector2(142.0,96.0)
 mouse_filter=Control.MOUSE_FILTER_IGNORE

func set_vitals(health:float,experience:float)->void:
 var next_hp:float=clampf(health,0.0,1.0)
 var next_xp:float=clampf(experience,0.0,1.0)
 if is_equal_approx(next_hp,hp_ratio) and is_equal_approx(next_xp,xp_ratio):return
 hp_ratio=next_hp
 xp_ratio=next_xp
 queue_redraw()

func _draw()->void:
 var center_hp:=Vector2(37.0,48.0)
 var center_xp:=Vector2(105.0,48.0)
 _ring(center_hp,26.0,hp_ratio,Color("#ff555f"))
 _ring(center_xp,26.0,xp_ratio,Color("#b786ff"))

func _ring(center:Vector2,radius:float,ratio:float,tint:Color)->void:
 draw_circle(center,radius-6.0,Color(0.05,0.06,0.12,0.95))
 draw_arc(center,radius,0.0,TAU,56,Color(0.20,0.19,0.27,0.9),7.0,true)
 if ratio>0.0:
  draw_arc(center,radius,-PI/2.0,-PI/2.0+TAU*ratio,56,tint,7.0,true)
