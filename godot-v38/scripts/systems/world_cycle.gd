extends RefCounted
## Moonfall v30.3: deterministic living-world rules extracted from native_v11.gd.
## This module does not read/write saves or mutate any hero, enemy, or scene state.

static func phase(clock:float,day_length:float)->String:
 var fraction:float=fposmod(clock,day_length)/day_length
 if fraction>=0.35 and fraction<0.65:return "Day"
 if fraction>=0.20 and fraction<0.35:return "Dawn"
 if fraction>=0.65 and fraction<0.80:return "Dusk"
 return "Night"

static func advance(clock:float,day:int,weather:String,remaining:float,changes:int,visual_tick:float,delta:float,rng:RandomNumberGenerator,day_length:float,weather_interval:float,weather_types:Array)->Dictionary:
 # Mirror previous clock behavior: one day increment at each elapsed boundary,
 # one weather shift per frame even if the frame spans multiple intervals.
 clock+=delta
 if clock>=day_length:
  clock=fposmod(clock,day_length)
  day=mini(99999,day+1)
 remaining-=delta
 var changed:bool=false
 if remaining<=0.0:
  remaining=weather_interval
  var choices:Array=weather_types.duplicate()
  choices.erase(weather)
  if not choices.is_empty():
   weather=str(choices[rng.randi_range(0,choices.size()-1)])
   changes+=1
   changed=true
 visual_tick+=delta
 var lighting:bool=visual_tick>=0.30
 if lighting:visual_tick=0.0
 return {"clock":clock,"day":day,"weather":weather,"remaining":remaining,"changes":changes,"visual_tick":visual_tick,"weather_changed":changed,"refresh_lighting":lighting}

static func daylight(clock:float,day_length:float,weather:String)->float:
 var amount:float=clampf(sin(clock/day_length*TAU-PI*0.5)*0.5+0.5,0.0,1.0)
 if weather=="Eclipse":amount*=0.38
 elif weather=="Rain":amount*=0.72
 return amount

static func fog_density(weather:String)->float:
 return 0.017 if weather=="Mist" else 0.012 if weather=="Rain" else 0.010 if weather=="Eclipse" else 0.008

static func apply_lighting(environment:Environment,main_light:DirectionalLight3D,clock:float,day_length:float,weather:String)->void:
 if environment==null:return
 var amount:float=daylight(clock,day_length,weather)
 environment.background_color=Color("070b19").lerp(Color("344552"),amount)
 environment.ambient_light_color=Color("7182ba").lerp(Color("c2b9ab"),amount)
 environment.ambient_light_energy=0.35+amount*0.48
 environment.fog_light_color=Color("1d2340").lerp(Color("53636c"),amount)
 environment.fog_density=fog_density(weather)
 if is_instance_valid(main_light):
  main_light.light_energy=0.55+amount*0.85
  main_light.light_color=Color("a5a3ec").lerp(Color("f0d8b3"),amount)
