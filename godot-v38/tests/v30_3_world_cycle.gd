extends SceneTree
const CYCLE=preload("res://scripts/systems/world_cycle.gd")
var errors:int=0

func check(ok:bool,description:String)->void:
 if ok:print("PASS v30.3: "+description)
 else:
  errors+=1
  push_error("FAIL v30.3: "+description)

func _initialize()->void:call_deferred("_run")

func _run()->void:
 for at in [0.0,95.0,120.0,167.9,168.0,240.0,311.9,312.0,383.9,384.0,479.9]:
  var fraction:float=fposmod(at,480.0)/480.0
  var expected:String="Day" if fraction>=0.35 and fraction<0.65 else "Dawn" if fraction>=0.20 and fraction<0.35 else "Dusk" if fraction>=0.65 and fraction<0.80 else "Night"
  check(CYCLE.phase(at,480.0)==expected,"day phase at "+str(at))
 check(is_equal_approx(CYCLE.daylight(240.0,480.0,"Rain"),0.72),"rain darkens daytime")
 check(is_equal_approx(CYCLE.daylight(240.0,480.0,"Eclipse"),0.38),"eclipse darkens daytime")
 check(is_equal_approx(CYCLE.fog_density("Mist"),0.017),"mist fog rule")
 var rng:=RandomNumberGenerator.new()
 rng.seed=301
 var next:Dictionary=CYCLE.advance(479.0,2,"Clear",0.1,0,0.25,1.2,rng,480.0,145.0,["Clear","Mist","Rain","Eclipse"])
 check(next["day"]==3 and is_equal_approx(float(next["clock"]),0.2),"day wraps, count advances")
 check(bool(next["weather_changed"]) and next["weather"]!="Clear" and next["changes"]==1,"weather changes once to a different kind")
 check(bool(next["refresh_lighting"]) and is_zero_approx(float(next["visual_tick"])),"lighting update cadence")
 var normal:Dictionary=CYCLE.advance(240.0,1,"Rain",40.0,3,0.0,0.1,rng,480.0,145.0,["Clear","Mist","Rain","Eclipse"])
 check(normal["day"]==1 and normal["weather"]=="Rain" and not normal["weather_changed"],"normal frame keeps weather")
 var env:=Environment.new()
 var light:=DirectionalLight3D.new()
 CYCLE.apply_lighting(env,light,240.0,480.0,"Rain")
 check(is_equal_approx(env.fog_density,0.012) and is_equal_approx(env.ambient_light_energy,0.35+0.72*0.48),"visual lighting values")
 light.queue_free()
 var dial:Control=load("res://scripts/systems/celestial_wait_clock.gd").new()
 dial.hour=26.0
 check(is_equal_approx(dial.hour,2.0),"celestial dial wraps beyond 24 hours")
 dial.hour=-1.0
 check(is_equal_approx(dial.hour,23.0),"celestial dial wraps negative hours")
 dial.hour=INF
 check(is_equal_approx(dial.hour,23.0),"celestial dial rejects nonfinite time")
 dial.queue_free()
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"game scene loads")
 if scene!=null:
  var game:Node3D=scene.instantiate()
  root.add_child(game)
  await process_frame
  game.set("world_clock",7199.0)
  game.set("world_day",2)
  game.set("weather_name","Clear")
  game.set("weather_remaining",0.1)
  game.call("_advance_living_world",1.2)
  check(game.get("world_day")==3 and game.get("weather_name")!="Clear","existing game wrappers use new module")
  check(game.call("_day_phase")==CYCLE.phase(float(game.get("world_clock")),7200.0),"old day-phase calls still work")
  check(is_equal_approx(float(game.get("DAY_LENGTH_SECONDS")),7200.0),"full cycle lasts two real hours")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":480.0})),3600.0),"legacy noon remains noon")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":5400.0,"world_cycle_seconds":7200.0})),5400.0),"new save keeps current time")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":INF,"world_cycle_seconds":0.0})),3600.0),"invalid save time falls back to noon")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":0.0})),0.0),"legacy midnight remains midnight")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":960.0})),0.0),"legacy full day wraps to midnight")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":-240.0})),5400.0),"legacy negative time wraps to evening")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":1800.0,"world_cycle_seconds":7200.0})),1800.0),"new save dawn remains dawn")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":240.0})),1800.0),"legacy dawn stays at six")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":720.0})),5400.0),"legacy dusk stays at eighteen")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":1920.0})),0.0),"legacy multi-day clock wraps to midnight")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":10800.0,"world_cycle_seconds":7200.0})),3600.0),"new multi-day save wraps to noon")  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":0.0})),0.0),"legacy midnight remains midnight")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":960.0})),0.0),"legacy full day wraps to midnight")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":-240.0})),5400.0),"legacy negative time wraps to evening")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":0.0})),0.0),"legacy midnight remains midnight")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":960.0})),0.0),"legacy full day wraps to midnight")
  check(is_equal_approx(float(game.call("_normalize_saved_world_clock",{"world_clock":0.0})),0.0),"legacy midnight remains midnight")
  check(game.has_method("_wait_clock_text"),"Wait system exposes in-game clock formatting")
  check(game.call("_wait_clock_text",3600.0)=="12:00","two-hour cycle midpoint reads noon")
  check(game.call("_wait_clock_text",1800.0)=="06:00","Long day quarter reads dawn")
  check(game.call("_wait_clock_text",5400.0)=="18:00","Long day three-quarter reads evening")
  check(game.call("_wait_clock_text",7200.0)=="00:00","Wait clock wraps cleanly at midnight")
  check(game.call("_wait_clock_text",10800.0)=="12:00","Clock format wraps across multiple days")
  check(game.call("_wait_clock_text",-300.0)=="23:00","Clock wraps negative time safely")
  check(game.has_method("_skip_wait_animation"),"Wait animation is skippable")
  check(game.has_method("_wait_change_hours"),"Wait adjustment helper exists")
  check(game.has_method("_finish_wait"),"Wait confirmation helper exists")
  check(game.get("wait_hours")>=1 and game.get("wait_hours")<=24,"Wait starts within valid hour bounds")
  game.queue_free()
 if errors==0:print("MOONFALL v30.3 WORLD CYCLE PASS")
 quit(0 if errors==0 else 1)
