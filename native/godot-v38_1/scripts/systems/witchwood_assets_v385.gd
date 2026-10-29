extends RefCounted
## Original Witchwood geometry and compact PBR detail maps.
## Meshes are generated once, reused by the four existing MultiMesh batches.
## No gameplay random source, colliders, external assets or runtime shader loops.
const SIDES:int=8
const TEXTURE_SIZE:int=64

static func _vertex(st:SurfaceTool,position:Vector3,normal:Vector3,uv:Vector2)->void:
 st.set_normal(normal)
 st.set_uv(uv)
 st.add_vertex(position)

static func _tube_point(start:Vector3,finish:Vector3,radius:float,t:float,theta:float,bend:float)->Vector3:
 var axis:Vector3=(finish-start).normalized()
 var right:Vector3=axis.cross(Vector3.FORWARD)
 if right.length_squared()<0.05:right=axis.cross(Vector3.RIGHT)
 right=right.normalized()
 var forward:Vector3=axis.cross(right).normalized()
 var irregular:float=1.0+0.07*sin(theta*3.0+t*9.0)
 var core:Vector3=start.lerp(finish,t)+right*(bend*sin(t*PI))
 return core+(right*cos(theta)+forward*sin(theta))*radius*irregular

static func _tube(st:SurfaceTool,start:Vector3,finish:Vector3,base_radius:float,tip_radius:float,bend:float,segments:int=5)->void:
 var axis:Vector3=(finish-start).normalized()
 var right:Vector3=axis.cross(Vector3.FORWARD)
 if right.length_squared()<0.05:right=axis.cross(Vector3.RIGHT)
 right=right.normalized()
 var forward:Vector3=axis.cross(right).normalized()
 for ring in range(segments):
  var t0:float=float(ring)/float(segments)
  var t1:float=float(ring+1)/float(segments)
  var r0:float=lerpf(base_radius,tip_radius,t0)
  var r1:float=lerpf(base_radius,tip_radius,t1)
  for side in range(SIDES):
   var a0:float=TAU*float(side)/float(SIDES)
   var a1:float=TAU*float(side+1)/float(SIDES)
   var p00:Vector3=_tube_point(start,finish,r0,t0,a0,bend)
   var p01:Vector3=_tube_point(start,finish,r0,t0,a1,bend)
   var p10:Vector3=_tube_point(start,finish,r1,t1,a0,bend)
   var p11:Vector3=_tube_point(start,finish,r1,t1,a1,bend)
   var n0:Vector3=(right*cos(a0)+forward*sin(a0)).normalized()
   var n1:Vector3=(right*cos(a1)+forward*sin(a1)).normalized()
   var u0:float=float(side)/float(SIDES)
   var u1:float=float(side+1)/float(SIDES)
   _vertex(st,p00,n0,Vector2(u0,t0))
   _vertex(st,p10,n0,Vector2(u0,t1))
   _vertex(st,p01,n1,Vector2(u1,t0))
   _vertex(st,p01,n1,Vector2(u1,t0))
   _vertex(st,p10,n0,Vector2(u0,t1))
   _vertex(st,p11,n1,Vector2(u1,t1))

static func make_moonwood_trunk()->ArrayMesh:
 var st:=SurfaceTool.new()
 st.begin(Mesh.PRIMITIVE_TRIANGLES)
 # One shared branchy silhouette instead of a featureless cylinder.
 _tube(st,Vector3(0.0,-1.9,0.0),Vector3(0.10,1.92,0.05),0.40,0.18,0.17)
 _tube(st,Vector3(0.03,0.12,0.0),Vector3(-1.15,1.68,-0.30),0.19,0.035,-0.18,4)
 _tube(st,Vector3(0.09,0.61,0.05),Vector3(1.24,1.58,0.55),0.18,0.03,0.13,4)
 _tube(st,Vector3(0.08,1.20,0.05),Vector3(-0.68,2.18,0.81),0.12,0.02,0.11,3)
 st.generate_tangents()
 return st.commit()

static func _leaf_lobe(st:SurfaceTool,center:Vector3,radius:Vector3,twist:float)->void:
 var rings:int=5
 var sides:int=9
 for ring in range(rings):
  var t0:float=float(ring)/float(rings)
  var t1:float=float(ring+1)/float(rings)
  var y0:float=-radius.y+2.0*radius.y*t0
  var y1:float=-radius.y+2.0*radius.y*t1
  var r0:float=sqrt(maxf(0.002,1.0-pow(2.0*t0-1.0,2.0)))
  var r1:float=sqrt(maxf(0.002,1.0-pow(2.0*t1-1.0,2.0)))
  for side in range(sides):
   var a0:float=TAU*float(side)/float(sides)+twist
   var a1:float=TAU*float(side+1)/float(sides)+twist
   var p00:Vector3=center+Vector3(cos(a0)*radius.x*r0,y0,sin(a0)*radius.z*r0)
   var p01:Vector3=center+Vector3(cos(a1)*radius.x*r0,y0,sin(a1)*radius.z*r0)
   var p10:Vector3=center+Vector3(cos(a0)*radius.x*r1,y1,sin(a0)*radius.z*r1)
   var p11:Vector3=center+Vector3(cos(a1)*radius.x*r1,y1,sin(a1)*radius.z*r1)
   var n0:Vector3=Vector3(cos(a0)/radius.x,(2.0*t0-1.0)/radius.y,sin(a0)/radius.z).normalized()
   var n1:Vector3=Vector3(cos(a1)/radius.x,(2.0*t0-1.0)/radius.y,sin(a1)/radius.z).normalized()
   var n2:Vector3=Vector3(cos(a0)/radius.x,(2.0*t1-1.0)/radius.y,sin(a0)/radius.z).normalized()
   var n3:Vector3=Vector3(cos(a1)/radius.x,(2.0*t1-1.0)/radius.y,sin(a1)/radius.z).normalized()
   var u0:float=float(side)/float(sides)
   var u1:float=float(side+1)/float(sides)
   _vertex(st,p00,n0,Vector2(u0,t0))
   _vertex(st,p10,n2,Vector2(u0,t1))
   _vertex(st,p01,n1,Vector2(u1,t0))
   _vertex(st,p01,n1,Vector2(u1,t0))
   _vertex(st,p10,n2,Vector2(u0,t1))
   _vertex(st,p11,n3,Vector2(u1,t1))

static func make_moonwood_canopy()->ArrayMesh:
 var st:=SurfaceTool.new()
 st.begin(Mesh.PRIMITIVE_TRIANGLES)
 # Organic overlapping crowns break up the artificial cone silhouette.
 _leaf_lobe(st,Vector3(0.0,0.0,0.0),Vector3(1.15,1.80,1.10),0.0)
 _leaf_lobe(st,Vector3(-0.70,-0.40,-0.25),Vector3(0.80,1.10,0.85),0.47)
 _leaf_lobe(st,Vector3(0.77,-0.22,0.22),Vector3(0.85,1.20,0.78),0.18)
 _leaf_lobe(st,Vector3(0.12,0.48,-0.70),Vector3(0.78,1.05,0.70),0.28)
 st.generate_tangents()
 return st.commit()

static func make_mossy_rock()->ArrayMesh:
 var st:=SurfaceTool.new()
 st.begin(Mesh.PRIMITIVE_TRIANGLES)
 _leaf_lobe(st,Vector3.ZERO,Vector3(0.70,0.47,0.58),0.27)
 st.generate_tangents()
 return st.commit()

static func make_detail_texture(kind:String,normal_map:bool=false)->ImageTexture:
 var img:=Image.create(TEXTURE_SIZE,TEXTURE_SIZE,false,Image.FORMAT_RGBA8)
 for y in range(TEXTURE_SIZE):
  for x in range(TEXTURE_SIZE):
   var fx:float=float(x)/float(TEXTURE_SIZE)
   var fy:float=float(y)/float(TEXTURE_SIZE)
   var grooves:float=0.5+0.5*sin(fx*TAU*13.0+sin(fy*TAU*2.0)*1.2)
   var grain:float=0.5+0.5*sin(fy*TAU*23.0+fx*TAU*7.0)
   var fleck:float=0.5+0.5*sin((fx*31.0+fy*49.0)*TAU)
   var n:float=clampf(0.22+0.50*grooves+0.19*grain+0.09*fleck,0.0,1.0)
   if normal_map:
    var dx:float=(0.5+0.5*cos(fx*TAU*13.0+sin(fy*TAU*2.0)*1.2))*0.34
    img.set_pixel(x,y,Color(0.5+dx,0.5,0.91,1.0))
   else:
    var shade:float=lerpf(0.64,1.0,n)
    var tint:Color=Color.WHITE
    if kind=="bark":tint=Color(0.82,0.77,0.79)
    elif kind=="foliage":tint=Color(0.70,0.93,0.79)
    elif kind=="stone":tint=Color(0.77,0.85,0.81)
    img.set_pixel(x,y,Color(tint.r*shade,tint.g*shade,tint.b*shade,1.0))
 img.generate_mipmaps()
 return ImageTexture.create_from_image(img)

static func make_surface_material(kind:String,tint:Color)->StandardMaterial3D:
 var material:=StandardMaterial3D.new()
 material.albedo_color=tint
 material.albedo_texture=make_detail_texture(kind)
 material.metallic=0.0
 material.roughness=0.91 if kind=="bark" else 0.88 if kind=="stone" else 0.86
 material.normal_enabled=true
 material.normal_texture=make_detail_texture(kind,true)
 material.normal_scale=0.42
 return material
