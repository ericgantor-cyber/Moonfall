extends SceneTree
const SPAWNS := preload("res://scripts/systems/spawn_rules.gd")
const AI := preload("res://scripts/systems/ai_rules.gd")
const COMBAT := preload("res://scripts/systems/combat_rules.gd")
var failed:=0
func check(ok:bool,message:String)->void:
 if ok:print("PASS v27: "+message)
 else:
  failed+=1
  push_error("FAIL v27: "+message)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"main scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(str(game.call("_release_label")).begins_with("v3"),"v27 label")
 var regions:Array=game.get("REGIONS")
 var expected_count:=0
 var guardian_count:=0
 for zone in range(regions.size()):
  var encounters:Array[Dictionary]=SPAWNS.region_encounters(zone,regions[zone])
  check(encounters.size()==(7 if zone==0 else 8),"region %d encounter count"%zone)
  expected_count+=encounters.size()
  for enc in encounters:
   check(int(enc["zone"])==zone,"encounter zone %d"%zone)
   if bool(enc["boss"]):guardian_count+=1
 check(expected_count==47 and guardian_count==5,"47 native population slots with five guardians")
 check(game.get("beasts").size()>=expected_count,"existing ecology extra actors retained")
 check(is_equal_approx(SPAWNS.health_scale(3,"elder",true),6.912),"guardian HP scale unchanged")
 check(is_equal_approx(AI.perception(false,"Clear",false),10.0),"base perception unchanged")
 check(is_equal_approx(AI.perception(true,"Mist",true),14.0*0.78*1.16),"weather night alert perception")
 var heading:Vector3=AI.approach(Vector3(5,0,0),2.5,1,false,1,5.0)
 check(heading.z<0.0 and is_equal_approx(heading.length(),2.5),"flank steering and speed preserved")
 check(is_equal_approx(AI.approach(Vector3(5,0,0),2.5,1,true,3,5.0).length(),3.25),"guardian phase acceleration")
 check(AI.approach(Vector3.ZERO,2.0,0,false,1,0.0)==Vector3.ZERO,"zero-length pursuit avoids NaNs")
 check(AI.retreat_due(27.0,100.0,false,-1,20.0,19.0),"wounded normal beast retreat")
 check(not AI.retreat_due(27.0,100.0,true,-1,20.0,19.0),"guardian retreat excluded")
 check(not AI.retreat_due(27.0,100.0,false,0,20.0,19.0),"rift beasts do not retreat")
 check(is_equal_approx(COMBAT.outgoing_damage(50.0,false,false,0,0.0,1.0),50.0),"base player damage unchanged")
 check(is_equal_approx(COMBAT.outgoing_damage(50.0,true,true,2,0.10,1.0),105.6),"critical execute fury order preserved")
 check(COMBAT.elite_missed(Vector3.ZERO,Vector3(2,0,0),0),"outside marked elite attack range")
 check(COMBAT.elite_missed(Vector3.ZERO,Vector3.ZERO,0.2),"dodge avoids elite strike")
 check(is_equal_approx(COMBAT.guardian_attack_cooldown(3),2.1),"guardian phase attack rate")
 # Reproducible CPU microbenchmark, not a claim about real-world FPS.
 var start:=Time.get_ticks_usec()
 var sum:float=0.0
 for i in range(15000):
  sum+=COMBAT.outgoing_damage(28.0,i%7==0,i%3==0,2,0.1,1.0)
  sum+=AI.perception(i%2==0,"Mist",i%4==0)
  sum+=SPAWNS.health_scale((i%4)+1,"elder",i%10==0)
 var elapsed_ms:float=float(Time.get_ticks_usec()-start)/1000.0
 check(is_finite(sum) and elapsed_ms>=0.0,"45k pure rules calculations complete")
 print("V27 CORE MICROBENCHMARK: %0.2f ms for 45,000 rule calls (headless, not FPS)"%elapsed_ms)
 game.queue_free()
 if failed==0:print("MOONFALL v27 SYSTEM MODULES PASS")
 quit(0 if failed==0 else 1)
