extends RefCounted
## Player-controlled, versioned graphics preferences. Never stored in hero checkpoints.
const PROFILE_PATH := "user://moonfall_graphics.cfg"
const GROUPS:Dictionary = {
 "Display":["window_mode","resolution","render_scale","upscaler","dynamic_resolution","target_fps","vsync","aa","fps_foreground","fps_background","ui_text"],
 "World":["view_distance","shadow_quality","shadow_distance","environment_detail","ground_clutter","fog","weather_visibility","terrain_surface","realism_grade","wet_surfaces","soft_shadows","cinematic_lighting","foliage_quality","scenery_lod"],
 "Effects":["ssao","ssil","reflections","glow","particle_density","treasure_highlights","combat_effects"],
 "Advanced":["texture_filter","anisotropy","brightness","contrast","projected_telegraphs"]
}
# Every enabled setting has a concrete Godot runtime effect. Options unavailable to
# this renderer are shown separately, not presented as working toggles.
const OPTIONS:Dictionary = {
 "window_mode":["Windowed","Borderless","Fullscreen"],
 "resolution":["1280x800","1600x900","1920x1080","2560x1440","3840x2160"],
 "render_scale":["50%","67%","75%","85%","100%","125%"],
 "upscaler":["Bilinear","FSR 1.0"],
 "dynamic_resolution":["Off","On"],
 "target_fps":["30","60","90","120"],
 "vsync":["Off","On","Adaptive"],
 "aa":["Off","FXAA","MSAA 2x","MSAA 4x","TAA"],
 "fps_foreground":["30","60","90","120","144","165","240","Unlimited"],
 "fps_background":["15","30","60"],
 "ui_text":["Small","Normal","Large","Extra Large"],
 "view_distance":["1","2","3","4","5","6","7","8","9","10"],
 "shadow_quality":["Off","Low","Medium","High","Ultra"],
 "shadow_distance":["Near","Medium","Far"],
 "environment_detail":["1","2","3","4","5","6","7","8","9","10"],
 "ground_clutter":["Off","Low","Medium","High","Ultra"],
 "fog":["Off","Low","Normal","High"],
 "weather_visibility":["Low","Normal","High"],
 "ssao":["Off","Low","High"],
 "ssil":["Off","On"],
 "reflections":["Off","On"],
 "glow":["Off","On"],
 "particle_density":["Low","Medium","High"],
 "treasure_highlights":["Minimal","Halo","Full"],
 "texture_filter":["Nearest","Linear","Trilinear","Anisotropic"],
 "anisotropy":["Off","2x","4x","8x","16x"],
 "brightness":["Low","Normal","High"],
 "contrast":["Low","Normal","High"],
 "terrain_surface":["Off","Color Detail","PBR Detail"],
 "realism_grade":["Classic","Natural","Cinematic"],
 "wet_surfaces":["Off","On"],
 "soft_shadows":["Off","On"],
 "cinematic_lighting":["Classic","Natural","Cinematic"],
 "foliage_quality":["Off","Low","Medium","High"],
 "scenery_lod":["Near","Balanced","Far"],
 "combat_effects":["Off","Medium","High"],
 "projected_telegraphs":["Always Visible"]
}
const DEFAULTS:Dictionary={
 "window_mode":"Windowed","resolution":"1280x800","render_scale":"100%","upscaler":"Bilinear","dynamic_resolution":"Off","target_fps":"60","vsync":"On",
 "aa":"MSAA 2x","fps_foreground":"120","fps_background":"30","ui_text":"Normal",
 "view_distance":"7","shadow_quality":"High","shadow_distance":"Medium","environment_detail":"7","ground_clutter":"High",
 "fog":"Normal","weather_visibility":"Normal","terrain_surface":"PBR Detail","realism_grade":"Natural","wet_surfaces":"On","soft_shadows":"On","ssao":"Low","ssil":"Off","reflections":"Off",
 "glow":"Off","particle_density":"High","treasure_highlights":"Full",
 "texture_filter":"Linear","anisotropy":"4x","brightness":"Normal","contrast":"Normal","cinematic_lighting":"Natural","foliage_quality":"High","scenery_lod":"Balanced","combat_effects":"Medium","projected_telegraphs":"Always Visible"}
const UNSUPPORTED:Array[String]=[
 "DLSS / NVIDIA Reflex / Intel XeSS (requires vendor integration)",
 "FSR 2.2 temporal upscaling and vendor-specific sharpen controls (not configured)",
 "Ray-traced shadows and variable-rate shading (not implemented)",
 "DirectX/Vulkan graphics API switching at runtime (requires restart/build flags)",
 "GPU/device selection, triple buffering and compute effects (renderer-managed)",
 "Liquid simulation quality (Moonfall has no scalable water simulation)",
 "Independent high-resolution texture packs (not authored yet)",
 "Raid-specific quality presets (Moonfall has no raid mode)",
 "Adjustable camera FOV (perspective camera is set to 66 degrees; mouse wheel adjusts distance)",
 "NVIDIA Reflex/low-latency driver modes and silhouette outlines (not implemented)",
 "Spell-density culling (spell telegraphs and hit cues are intentionally never hidden)",
 "Color calibration / HDR output (requires display-specific setup)",
 "WoW raid graphics overrides (no separate raid mode in this connected world)",
 "Texture resolution switch (requires multiple source texture sets)",
 "Liquid detail, depth-of-field and compute effects (no corresponding scalable scene system)"]
static func normalize(values:Dictionary)->Dictionary:
 var output:Dictionary=DEFAULTS.duplicate(true)
 for key in DEFAULTS:
  if key=="projected_telegraphs":continue # Never allow attack warnings to disappear.
  var choice:String=str(values.get(key,DEFAULTS[key]))
  if choice in OPTIONS[key]:output[key]=choice
 return output
static func preset(level:int)->Dictionary:
 var value:Dictionary=DEFAULTS.duplicate(true)
 var n:int=clampi(level,1,10)
 value["cinematic_lighting"]="Classic" if n<=2 else "Natural" if n<=7 else "Cinematic"
 value["foliage_quality"]="Off" if n<=2 else "Low" if n<=4 else "Medium" if n<=7 else "High"
 value["scenery_lod"]="Near" if n<=3 else "Balanced" if n<=7 else "Far"
 value["combat_effects"]="Off" if n<=2 else "Medium" if n<=6 else "High"
 value["view_distance"]=str(n)
 value["terrain_surface"]="Off" if n<=2 else "Color Detail" if n<=5 else "PBR Detail"
 value["realism_grade"]="Classic" if n<=2 else "Natural" if n<=7 else "Cinematic"
 value["wet_surfaces"]="On" if n>=5 else "Off"
 value["soft_shadows"]="On" if n>=5 else "Off"
 value["upscaler"]="FSR 1.0" if n<=5 else "Bilinear"
 value["shadow_distance"]="Near" if n<=3 else "Medium" if n<=7 else "Far"
 value["treasure_highlights"]="Minimal" if n<=3 else "Halo" if n<=6 else "Full"
 value["texture_filter"]="Nearest" if n<=2 else "Linear" if n<=4 else "Trilinear" if n<=6 else "Anisotropic"
 value["anisotropy"]="Off" if n<=4 else "4x" if n<=7 else "8x" if n<=9 else "16x"
 value["environment_detail"]=str(n)
 value["shadow_quality"]="Off" if n<=2 else "Low" if n<=4 else "Medium" if n<=6 else "High" if n<=8 else "Ultra"
 value["ground_clutter"]="Off" if n<=2 else "Low" if n<=4 else "Medium" if n<=6 else "High" if n<=8 else "Ultra"
 value["render_scale"]="67%" if n<=2 else "75%" if n<=4 else "85%" if n<=6 else "100%" if n<=9 else "125%"
 value["aa"]="Off" if n<=2 else "FXAA" if n<=4 else "MSAA 2x" if n<=7 else "MSAA 4x"
 value["ssao"]="Off" if n<=3 else "Low" if n<=7 else "High"
 value["ssil"]="On" if n>=9 else "Off"
 value["glow"]="On" if n>=8 else "Off"
 value["fog"]="Off" if n<=2 else "Low" if n<=4 else "Normal"
 value["particle_density"]="Low" if n<=3 else "Medium" if n<=6 else "High"
 value["fps_foreground"]="60" if n<=3 else "120"
 return normalize(value)
static func cycle(settings:Dictionary,key:String)->Dictionary:
 var output:Dictionary=normalize(settings)
 if not OPTIONS.has(key) or key=="projected_telegraphs":return output
 var choices:Array=OPTIONS[key]
 var next:int=(choices.find(str(output[key]))+1)%choices.size()
 output[key]=choices[next]
 return output
static func load_preferences()->Dictionary:
 var cfg:=ConfigFile.new()
 if cfg.load(PROFILE_PATH)!=OK:return DEFAULTS.duplicate(true)
 var values:Dictionary={}
 for key in DEFAULTS:values[key]=cfg.get_value("graphics",key,DEFAULTS[key])
 return normalize(values)
static func save_preferences(settings:Dictionary)->int:
 var cfg:=ConfigFile.new()
 var clean:Dictionary=normalize(settings)
 for key in clean:cfg.set_value("graphics",key,clean[key])
 return cfg.save(PROFILE_PATH)
