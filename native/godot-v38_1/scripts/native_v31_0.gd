extends "res://scripts/native_v30_9.gd"
## v31.0 WoW-style advanced graphics controls grounded in Godot renderer features.
## No changes to hero save data or inherited artwork.
var gfx31_dynamic_scale:float=1.0
var gfx31_poll_clock:float=0.0

func _release_label()->String:return "v31.0"

func _gfx_apply(apply_display:bool=true)->void:
 super._gfx_apply(apply_display)
 if not gfx_ready:return
 # FSR1 upscales the 3D scene only; text and HUD remain native-resolution.
 var vp:Viewport=get_viewport()
 vp.scaling_3d_mode=Viewport.SCALING_3D_MODE_FSR if str(gfx_settings["upscaler"])=="FSR 1.0" and float(str(gfx_settings["render_scale"]).trim_suffix("%"))<=100.0 else Viewport.SCALING_3D_MODE_BILINEAR
 gfx31_dynamic_scale=vp.scaling_3d_scale
 var level:String=str(gfx_settings["anisotropy"])
 var anisotropy:int={"Off":0,"2x":1,"4x":2,"8x":3,"16x":4}.get(level,2)
 RenderingServer.viewport_set_anisotropic_filtering_level(vp.get_viewport_rid(),anisotropy)
 var shadow_light:=get_node_or_null("TwinMoonlight") as DirectionalLight3D
 if is_instance_valid(shadow_light):
  var distance_factor:float={"Near":0.6,"Medium":1.0,"Far":1.5}.get(str(gfx_settings["shadow_distance"]),1.0)
  shadow_light.directional_shadow_max_distance*=distance_factor
 _gfx31_treasure_lighting()

func _refresh_treasure_fx()->void:
 super._refresh_treasure_fx()
 if gfx_ready:_gfx31_treasure_lighting()

func _gfx31_treasure_lighting()->void:
 # Decorative treasure cues can scale down, but chest names, pickup prompts,
 # collision, and boss attack telegraphs always remain visible.
 var mode:String=str(gfx_settings.get("treasure_highlights","Full"))
 var particles:String=str(gfx_settings.get("particle_density","High"))
 var detail:int=int(gfx_settings.get("environment_detail","7"))
 for entry in v308_treasure_fx:
  var closed:bool=not int(entry["zone"]) in opened_treasure_chests
  (entry["halo"] as MeshInstance3D).visible=closed and mode!="Minimal" and particles!="Low" and detail>2
  (entry["crystal"] as MeshInstance3D).visible=closed and mode=="Full" and particles=="High" and detail>2

func _v24_update_lod()->void:
 # Recalculate visibility in both directions. Previously items could stay
 # hidden forever because the loop skipped previously invisible nodes.
 if not gfx_ready or not is_instance_valid(player):return
 var detail:float=float(int(gfx_settings.get("environment_detail","7")))
 var distance_squared:float=pow(8.0+detail*2.6,2.0)
 var visible_count:int=0
 for item in v24_diorama_cache:
  if not is_instance_valid(item):continue
  var parent_region:=item.get_parent() as Node3D
  var enabled:bool=graphics_quality!="Smooth" and is_instance_valid(parent_region) and parent_region.is_visible_in_tree()
  item.visible=enabled and item.global_position.distance_squared_to(player.global_position)<=distance_squared
  if item.visible:visible_count+=1
 v24_dioramas_visible=visible_count

func _process(delta:float)->void:
 super._process(delta)
 if not gfx_ready or str(gfx_settings.get("dynamic_resolution","Off"))!="On":return
 if DisplayServer.get_name()=="headless" or not hero_playing or gfx_background:return
 gfx31_poll_clock+=delta
 if gfx31_poll_clock<3.0:return
 gfx31_poll_clock=0.0
 var fps:float=Engine.get_frames_per_second()
 if fps<=1.0:return
 var target:float=float(gfx_settings.get("target_fps","60"))
 var desired:float=float(str(gfx_settings["render_scale"]).trim_suffix("%"))/100.0
 if fps<target*0.89:gfx31_dynamic_scale=maxf(0.50,gfx31_dynamic_scale-0.05)
 elif fps>target*1.12:gfx31_dynamic_scale=minf(desired,gfx31_dynamic_scale+0.05)
 get_viewport().scaling_3d_scale=gfx31_dynamic_scale
