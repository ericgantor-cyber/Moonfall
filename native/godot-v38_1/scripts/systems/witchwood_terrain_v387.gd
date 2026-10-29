extends RefCounted
## An original lightweight forest-floor surface for Witchwood only.
## Decorative micro-relief, not replacement world terrain or physics.
const GRID:int=32
const HALF:float=29.0
const PATCH_RES:int=96

static func _clearance(x:float,z:float)->float:
 # Fade to the existing ground on both crossing roads, the combat approach,
 # and the old shrine so no existing readable terrain is covered.
 var road:float=clampf(minf((absf(x)-8.0)/3.2,(absf(z)-7.0)/3.2),0.0,1.0)
 var altar:float=clampf((Vector2(x-3.0,z+7.5).length()-8.0)/3.0,0.0,1.0)
 var gate:float=clampf((Vector2(x-15.0,z-16.0).length()-9.0)/3.0,0.0,1.0)
 var edge:float=clampf((HALF-maxf(absf(x),absf(z)))/2.5,0.0,1.0)
 return minf(minf(road,altar),minf(gate,edge))

static func _height(x:float,z:float)->float:
 # Centimeter-level surface shading only; never deform gameplay collision.
 var billow:float=sin(x*0.38)*cos(z*0.31)+0.45*sin(x*0.92+z*0.63)
 return 0.035+0.010*billow

static func _point(st:SurfaceTool,x:float,z:float)->void:
 var mask:float=_clearance(x,z)
 var soil:float=0.5+0.5*sin(x*0.19+z*0.27)
 var uv:Vector2=Vector2(x,z)*0.17
 st.set_uv(uv)
 st.set_color(Color(lerpf(0.73,0.91,soil),lerpf(0.80,0.98,soil),lerpf(0.73,0.92,soil),mask*0.58))
 st.add_vertex(Vector3(x,_height(x,z),z))

static func _texture()->ImageTexture:
 # Bounded, deterministic soil/moss grain with no external asset licenses.
 var img:=Image.create(PATCH_RES,PATCH_RES,false,Image.FORMAT_RGBA8)
 for y in range(PATCH_RES):
  for x in range(PATCH_RES):
   var a:float=sin(float(x)*0.31+sin(float(y)*0.15)*1.3)
   var b:float=cos(float(y)*0.43+float(x)*0.17)
   var c:float=sin(float(x+y)*0.76)
   var shade:float=clampf(0.71+0.12*a+0.08*b+0.05*c,0.50,0.95)
   img.set_pixel(x,y,Color(shade*0.73,shade*0.85,shade*0.74,1.0))
 img.generate_mipmaps()
 return ImageTexture.create_from_image(img)

static func _mesh()->ArrayMesh:
 var st:=SurfaceTool.new()
 st.begin(Mesh.PRIMITIVE_TRIANGLES)
 for zi in range(GRID):
  for xi in range(GRID):
   var x0:float=-HALF+float(xi)*HALF*2.0/float(GRID)
   var x1:float=-HALF+float(xi+1)*HALF*2.0/float(GRID)
   var z0:float=-HALF+float(zi)*HALF*2.0/float(GRID)
   var z1:float=-HALF+float(zi+1)*HALF*2.0/float(GRID)
   # Counterclockwise viewed from above: normals point toward the sky.
   _point(st,x0,z0)
   _point(st,x0,z1)
   _point(st,x1,z0)
   _point(st,x0,z1)
   _point(st,x1,z1)
   _point(st,x1,z0)
 st.generate_normals()
 st.generate_tangents()
 return st.commit()

static func build(region_detail_root:Node3D)->Dictionary:
 var material:=StandardMaterial3D.new()
 material.albedo_color=Color("a4b2a1")
 material.albedo_texture=_texture()
 material.vertex_color_use_as_albedo=true
 material.roughness=0.96
 material.transparency=BaseMaterial3D.TRANSPARENCY_ALPHA
 material.depth_draw_mode=BaseMaterial3D.DEPTH_DRAW_OPAQUE_ONLY
 material.cull_mode=BaseMaterial3D.CULL_DISABLED
 var surface:=MeshInstance3D.new()
 surface.name="WitchwoodLeafSoilMicroRelief"
 surface.mesh=_mesh()
 surface.material_override=material
 surface.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
 region_detail_root.add_child(surface)
 return {"surface":surface,"material":material,"grid":GRID}

static func apply_quality(detail:Dictionary,settings:Dictionary)->void:
 if detail.is_empty():return
 var surface:MeshInstance3D=detail["surface"] as MeshInstance3D
 var level:int=clampi(int(settings.get("environment_detail","7")),1,10)
 var texture_mode:String=str(settings.get("terrain_surface","PBR Detail"))
 var clutter:String=str(settings.get("ground_clutter","High"))
 surface.visible=level>=5 and texture_mode!="Off" and clutter!="Off"
 var lod:String=str(settings.get("scenery_lod","Balanced"))
 surface.visibility_range_end=24.0 if lod=="Near" else 72.0 if lod=="Far" else 48.0
