extends "res://scripts/native_v30_8.gd"
## v30.9 WoW-inspired, capability-aware graphics controls.
## Separate user:// preferences, unchanged hero save schema and original art.
const GRAPHICS_RULES:=preload("res://scripts/systems/graphics_rules.gd")
var gfx_settings:Dictionary={}
var gfx_ready:bool=false
var gfx_restore_in_progress:bool=false
var gfx_section:String="Presets"
var gfx_tick:float=0.0
var gfx_background:bool=false
var gfx_scatter:Array[MultiMeshInstance3D]=[]

func _release_label()->String:return "v30.9"

func _ready()->void:
 super._ready()
 gfx_settings=GRAPHICS_RULES.load_preferences()
 gfx_ready=true
 _gfx_collect_scatter(world)
 # Restore saved window size/mode on launch; headless checks remain display-free.
 _gfx_apply()

func _gfx_collect_scatter(node:Node)->void:
 # Done once; there is no full-world tree search in the frame loop.
 if node is MultiMeshInstance3D:gfx_scatter.append(node)
 for child in node.get_children():_gfx_collect_scatter(child)

func _apply_graphics_quality(mode:String)->void:
 # Legacy Smooth/Full buttons still work; a checkpoint restore must not replace
 # the user's machine-specific graphics preferences.
 super._apply_graphics_quality(mode)
 if not gfx_ready or gfx_restore_in_progress:return
 gfx_settings=GRAPHICS_RULES.preset(2 if mode=="Smooth" else 8)
 _gfx_apply(false)
 GRAPHICS_RULES.save_preferences(gfx_settings)

func _restore_data(data:Dictionary)->void:
 gfx_restore_in_progress=true
 super._restore_data(data)
 gfx_restore_in_progress=false
 if gfx_ready:_gfx_apply(false)

func _refresh_world_lighting()->void:
 super._refresh_world_lighting()
 if gfx_ready:_gfx_environment()

func _gfx_environment()->void:
 if living_environment==null:return
 living_environment.fog_enabled=str(gfx_settings.get("fog","Normal"))!="Off"
 var fog_multiplier:float={"Off":0.0,"Low":0.50,"Normal":1.0,"High":1.8}.get(str(gfx_settings.get("fog","Normal")),1.0)
 if weather_name!="Clear":fog_multiplier*= {"Low":0.5,"Normal":1.0,"High":1.5}.get(str(gfx_settings.get("weather_visibility","Normal")),1.0)
 living_environment.fog_density*=fog_multiplier
 # Changing day/night continues to apply fog and brightness in the base script;
 # this function is called only after that base refresh or explicit option changes.
 living_environment.ssao_enabled=str(gfx_settings.get("ssao","Off"))!="Off"
 living_environment.ssao_intensity=0.45 if str(gfx_settings.get("ssao","Off"))=="Low" else 1.0
 living_environment.ssil_enabled=str(gfx_settings.get("ssil","Off"))=="On"
 living_environment.ssr_enabled=str(gfx_settings.get("reflections","Off"))=="On"
 living_environment.glow_enabled=str(gfx_settings.get("glow","Off"))=="On"
 living_environment.adjustment_enabled=true
 living_environment.adjustment_brightness={"Low":0.8,"Normal":1.0,"High":1.2}.get(str(gfx_settings.get("brightness","Normal")),1.0)
 living_environment.adjustment_contrast={"Low":0.85,"Normal":1.0,"High":1.18}.get(str(gfx_settings.get("contrast","Normal")),1.0)

func _gfx_apply(apply_display:bool=true)->void:
 if not gfx_ready:return
 gfx_settings=GRAPHICS_RULES.normalize(gfx_settings)
 var vp:Viewport=get_viewport()
 vp.scaling_3d_scale=float(str(gfx_settings["render_scale"]).trim_suffix("%"))/100.0
 var aa:String=str(gfx_settings["aa"])
 vp.msaa_3d=Viewport.MSAA_2X if aa=="MSAA 2x" else Viewport.MSAA_4X if aa=="MSAA 4x" else Viewport.MSAA_DISABLED
 vp.screen_space_aa=Viewport.SCREEN_SPACE_AA_FXAA if aa=="FXAA" else Viewport.SCREEN_SPACE_AA_DISABLED
 vp.use_taa=aa=="TAA"
 var detail:int=int(gfx_settings["environment_detail"])
 # Match existing visual LOD paths, without disabling elite attack warnings.
 super._apply_graphics_quality("Smooth" if detail<=2 else "Full")
 var light:DirectionalLight3D=get_node_or_null("TwinMoonlight")
 var shadow:String=str(gfx_settings["shadow_quality"])
 if is_instance_valid(light):
  light.shadow_enabled=shadow!="Off"
  light.directional_shadow_max_distance={"Off":20.0,"Low":28.0,"Medium":42.0,"High":60.0,"Ultra":85.0}.get(shadow,42.0)
 if is_instance_valid(camera):camera.far=75.0+float(int(gfx_settings["view_distance"]))*20.0
 _refresh_world_lighting()
 # Scatter batches are purely decorative.  Keep essential objects and telegraphs visible.
 var clutter:String=str(gfx_settings["ground_clutter"])
 for i in range(gfx_scatter.size()):
  var instance:MultiMeshInstance3D=gfx_scatter[i]
  if is_instance_valid(instance):
   var show:bool=clutter=="Ultra" or clutter=="High" or (clutter=="Medium" and i%2==0) or (clutter=="Low" and i%3==0)
   instance.visible=show and detail>2
 # Ground textures are the assets actually using filtering in this project.
 var filter:String=str(gfx_settings["texture_filter"])
 var filter_mode:BaseMaterial3D.TextureFilter=BaseMaterial3D.TEXTURE_FILTER_NEAREST if filter=="Nearest" else BaseMaterial3D.TEXTURE_FILTER_LINEAR if filter=="Linear" else BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS if filter=="Trilinear" else BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC
 for zone_root in region_nodes:
  var ground:Node3D=zone_root.get_node_or_null("RegionGround")
  if ground==null:continue
  for child in ground.get_children():
   if child is MeshInstance3D and child.material_override is BaseMaterial3D:
    (child.material_override as BaseMaterial3D).texture_filter=filter_mode
 var particles:String=str(gfx_settings["particle_density"])
 for entry in v308_treasure_fx:
  var closed:bool=not int(entry["zone"]) in opened_treasure_chests
  (entry["halo"] as MeshInstance3D).visible=closed and particles!="Low" and detail>2
  (entry["crystal"] as MeshInstance3D).visible=closed and particles=="High" and detail>2
 var sizes:Dictionary={"Small":14,"Normal":17,"Large":20,"Extra Large":23}
 if is_instance_valid(menu_body):menu_body.add_theme_font_size_override("normal_font_size",int(sizes[str(gfx_settings["ui_text"])]))
 var cap:String=str(gfx_settings["fps_foreground"])
 Engine.max_fps=0 if cap=="Unlimited" else int(cap)
 if apply_display and DisplayServer.get_name()!="headless":
  var sync:String=str(gfx_settings["vsync"])
  DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED if sync=="Off" else DisplayServer.VSYNC_ADAPTIVE if sync=="Adaptive" else DisplayServer.VSYNC_ENABLED)
  var mode:String=str(gfx_settings["window_mode"])
  if mode=="Windowed":
   DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)
   var parts:PackedStringArray=str(gfx_settings["resolution"]).split("x")
   # Do not create an off-screen 4K window on a smaller monitor.
   var requested:Vector2i=Vector2i(int(parts[0]),int(parts[1]))
   var usable:Rect2i=DisplayServer.screen_get_usable_rect(DisplayServer.window_get_current_screen())
   var fitted:Vector2i=Vector2i(mini(requested.x,usable.size.x),mini(requested.y,usable.size.y))
   if fitted.x>0 and fitted.y>0:
    DisplayServer.window_set_size(fitted)
    DisplayServer.window_set_position(usable.position+Vector2i(int((usable.size.x-fitted.x)/2.0),int((usable.size.y-fitted.y)/2.0)))
  elif mode=="Borderless":DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)
  else:DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_EXCLUSIVE_FULLSCREEN)

func _v24_update_lod()->void:
 super._v24_update_lod()
 if not gfx_ready or not is_instance_valid(player):return
 var detail:float=float(int(gfx_settings.get("environment_detail","7")))
 var distance_squared:float=pow(8.0+detail*2.6,2.0)
 var visible_count:int=0
 for item in v24_diorama_cache:
  if not is_instance_valid(item) or not item.visible:continue
  item.visible=item.global_position.distance_squared_to(player.global_position)<=distance_squared
  if item.visible:visible_count+=1
 v24_dioramas_visible=visible_count

func _process(delta:float)->void:
 super._process(delta)
 if not gfx_ready:return
 gfx_tick+=delta
 if gfx_tick<1.0:return
 gfx_tick=0.0
 var background:bool=not DisplayServer.window_is_focused() if DisplayServer.get_name()!="headless" else false
 if background!=gfx_background:
  gfx_background=background
  var cap:String=str(gfx_settings["fps_background"] if background else gfx_settings["fps_foreground"])
  Engine.max_fps=0 if cap=="Unlimited" else int(cap)

func _menu_tab(which:String)->void:
 if which!="Graphics":
  super._menu_tab(which)
  if which=="System":_atlas_action("ADVANCED GRAPHICS • WoW-style options","gfx_open","Presets 1-10, display, detail, effects and advanced settings")
  return
 current_tab="Graphics"
 if not is_instance_valid(menu_body):return
 if is_instance_valid(inventory_search_input):inventory_search_input.visible=false
 for child in menu_buttons.get_children():child.queue_free()
 beast_guide.texture=null
 _gfx_show_section()

func _gfx_show_section()->void:
 var message:String="[b]MOONFALL GRAPHICS[/b]   •  Inspired by WoW's settings layout\n"
 message+="Real Godot effects are applied immediately. Machine-specific preferences are stored separately from hero saves.\n"
 message+="Combat telegraphs are ALWAYS visible, even at minimum settings.\n\n"
 if gfx_section=="Display":message+="4K UHD: select 3840x2160 in Windowed mode on a 4K monitor. Fullscreen uses your display resolution. Reduce Render Scale or select FSR 1.0 if needed for FPS. UI scales with the window; use UI Text for readability.\n\n"
 for group in ["Presets","Display","World","Effects","Advanced","Unavailable"]:
  _atlas_action(("▶ " if gfx_section==group else "   ")+group,"gfx_group:"+group)
 if gfx_section=="Presets":
  message+="[b]QUALITY PRESETS 1–10[/b]\n\n1 prioritizes FPS. 10 prioritizes visual detail.\nYou can customize every setting afterward.\n"
  for level in range(1,11):_atlas_action("QUALITY PRESET "+str(level),"gfx_preset:"+str(level))
 elif gfx_section=="Unavailable":
  message+="[b]NOT IMPLEMENTED IN THE CURRENT GODOT RENDERER[/b]\n\n"
  for entry in GRAPHICS_RULES.UNSUPPORTED:message+="• "+str(entry)+"\n\n"
 else:
  message+="[b]"+gfx_section.to_upper()+"[/b]  • Click an option to cycle values\n\n"
  for key in GRAPHICS_RULES.GROUPS[gfx_section]:
   var label:String=str(key).replace("_"," ").capitalize()
   var val:String=str(gfx_settings.get(key,""))
   message+="[b]"+label+"[/b]:  [color=#b1eacb]"+val+"[/color]\n\n"
   if key!="projected_telegraphs":_atlas_action(label+"  ◂ "+val+" ▸","gfx_cycle:"+key)
 message+="\n[b]Note:[/b] Moonfall uses a perspective third-person 3D camera. Right-drag to orbit, wheel to zoom; advanced effects vary by hardware.\n"
 menu_body.text=message
 _atlas_action("RESET GRAPHICS TO BALANCED","gfx_reset")
 _atlas_action("BACK TO SYSTEM","gfx_back")

func _atlas_execute(action:String)->void:
 if action.begins_with("gfx_"):
  if action=="gfx_open":gfx_section="Presets";_menu_tab("Graphics");return
  if action=="gfx_back":_menu_tab("System");return
  if action.begins_with("gfx_group:"):gfx_section=action.get_slice(":",1);_menu_tab("Graphics");return
  if action.begins_with("gfx_preset:"):gfx_settings=GRAPHICS_RULES.preset(int(action.get_slice(":",1)))
  elif action.begins_with("gfx_cycle:"):gfx_settings=GRAPHICS_RULES.cycle(gfx_settings,action.get_slice(":",1))
  elif action=="gfx_reset":gfx_settings=GRAPHICS_RULES.DEFAULTS.duplicate(true)
  _gfx_apply()
  GRAPHICS_RULES.save_preferences(gfx_settings)
  _menu_tab("Graphics")
  return
 super._atlas_execute(action)
