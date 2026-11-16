extends Control
## Native, resolution-independent celestial clock art. No imported character artwork.
var hour:float=12.0:
 set(value):
  if not is_finite(value):return
  var next_hour:float=fposmod(value,24.0)
  if is_equal_approx(hour,next_hour):return
  hour=next_hour
  queue_redraw()

func _draw()->void:
 var center:Vector2=size*0.5
 var radius:float=minf(size.x,size.y)*0.42
 if radius<=0.0:return
 draw_circle(center,radius,Color("10162e"))
 draw_arc(center,radius,0.0,TAU,96,Color("caa56a"),3.0,true)
 draw_arc(center,radius*0.82,0.0,TAU,96,Color("5b6391"),2.0,true)
 for n in range(24):
  var angle:float=TAU*float(n)/24.0-PI*0.5
  var inner:float=radius*(0.81 if n%6==0 else 0.87)
  var outer:float=radius*0.96
  var direction:=Vector2(cos(angle),sin(angle))
  draw_line(center+direction*inner,center+direction*outer,Color("ebcb8d"),2.0 if n%6==0 else 1.0,true)
 var orbit_angle:float=TAU*hour/24.0-PI*0.5
 var point:Vector2=center+Vector2(cos(orbit_angle),sin(orbit_angle))*radius*0.70
 var day:bool=hour>=6.0 and hour<18.0
 draw_circle(point,12.0,Color("ffdb83") if day else Color("c0d1f9"))
 if not day:draw_circle(point+Vector2(5.0,-4.0),10.0,Color("10162e"))
 draw_line(center,point,Color("dcbf8f"),2.0,true)
 draw_circle(center,5.0,Color("f8deab"))
