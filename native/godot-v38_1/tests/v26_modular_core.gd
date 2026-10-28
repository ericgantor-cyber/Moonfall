extends SceneTree
const STORE := preload("res://scripts/systems/checkpoint_store.gd")
const GEAR := preload("res://scripts/systems/gear_codec.gd")
const RULES := preload("res://scripts/systems/hero_rules.gd")
var failures := 0

func check(condition:bool,label:String) -> void:
 if condition:print("PASS v26: "+label)
 else:
  failures+=1
  push_error("FAIL v26: "+label)

func _initialize() -> void:call_deferred("_run")

func _run() -> void:
 var scene:PackedScene=load("res://scenes/main.tscn")
 check(scene!=null,"main scene loads")
 if scene==null:quit(1);return
 var game:Node3D=scene.instantiate()
 root.add_child(game)
 await process_frame
 check(str(game.call("_release_label")).begins_with("v3"),"v27 successor label")
 check(is_equal_approx(RULES.damage_multiplier(1,0,false),1.0),"new hero baseline damage")
 check(is_equal_approx(RULES.damage_multiplier(11,5,true),1.45),"combo and level damage scaling")
 check(is_equal_approx(RULES.power_modifier(0,300),0.70),"underequipped power floor")
 check(is_equal_approx(RULES.power_modifier(900,1),1.22),"high equipment power cap")
 check(RULES.recommended_power(99,game.get("REGIONS"))==RULES.recommended_power(5,game.get("REGIONS")),"region index clamping")
 check(game.call("_recommended_power",5)==RULES.recommended_power(5,game.get("REGIONS")),"legacy region power wrapper")
 var legendary:Dictionary={"name":"Astral Fang","rarity":"Legendary","power":999,"affix":"Fury","affix_value":0.70}
 var cleaned:Dictionary=GEAR.sanitize(legendary,["Fury","Aegis","Vampiric","Haste"])
 check(cleaned["power"]==250 and is_equal_approx(cleaned["affix_value"],0.35),"legendary affix and power limits")
 check(game.call("_safe_item",legendary)==cleaned,"legacy gear sanitizer wrapper")
 var unknown:Dictionary=GEAR.sanitize({"rarity":"Imaginary","power":-10,"affix":"Fury"},["Fury"])
 check(unknown["rarity"]=="Common" and unknown["power"]==0 and not unknown.has("affix"),"invalid gear rarity sanitized")
 # Must be isolated from any live checkpoint directory.
 var profile:String=OS.get_environment("MOONFALL_TEST_PROFILE")
 check(not profile.is_empty() and ProjectSettings.globalize_path("user://").begins_with(profile),"isolated test profile")
 if profile.is_empty() or not ProjectSettings.globalize_path("user://").begins_with(profile):
  game.queue_free()
  quit(1)
  return
 var primary:="user://v26_modular_primary.json"
 var backup:="user://v26_modular_backup.json"
 var pending:="user://v26_modular_pending.json"
 var save:Dictionary={"schema":2,"hero":"z","at":[2.5,-1.5],"hp":96.0,"gold":5,"kills":7,"level":1}
 check(STORE.commit_checkpoint(save,pending,primary,backup,7,"z")=="OK","staged primary save commits")
 check(str(STORE.decode(primary).get("hero",""))=="z" and int(STORE.decode(primary).get("kills",-1))==7 and is_equal_approx(float(STORE.decode(primary)["at"][0]),2.5),"schema-2 primary round trip")
 check(game.call("_decode_save",primary)==STORE.decode(primary),"legacy checkpoint decode wrapper")
 var save2:Dictionary=save.duplicate(true)
 save2["gold"]=99
 check(STORE.commit_checkpoint(save2,pending,primary,backup,7,"z")=="OK","updated primary commits")
 check(int(STORE.decode(primary)["gold"])==99 and int(STORE.decode(backup)["gold"])==5,"previous checkpoint backed up")
 check(STORE.commit_checkpoint(save2,pending,primary,backup,999,"z")=="INVALID STAGING","staging identity mismatch rejected")
 check(int(STORE.decode(primary)["gold"])==99,"failed staging preserves primary checkpoint")
 var f:=FileAccess.open("user://v26_corrupt.json",FileAccess.WRITE)
 if f!=null:
  f.store_string('{"schema":2,"hero":"z","at":[0,0],"hp":"invalid"}')
  f.close()
 check(STORE.decode("user://v26_corrupt.json").is_empty(),"invalid numeric checkpoint rejected")
 for path in [primary,backup,pending,"user://v26_corrupt.json"]:
  if FileAccess.file_exists(path):DirAccess.remove_absolute(ProjectSettings.globalize_path(path))
 game.queue_free()
 if failures==0:print("MOONFALL v26 MODULAR CORE PASS")
 quit(0 if failures==0 else 1)
