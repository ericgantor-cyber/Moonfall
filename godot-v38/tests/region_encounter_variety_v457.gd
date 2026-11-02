extends SceneTree
const S=preload("res://scripts/systems/spawn_rules.gd")
var failures:int=0
func ck(ok:bool,label:String)->void:
 if ok:print("PASS region: "+label)
 else:
  failures+=1
  push_error("FAIL region: "+label)
func _initialize()->void:call_deferred("run")
func run()->void:
 var region:Dictionary={"at":Vector2(100,100),"monster":"wolf"}
 var a:Array[Dictionary]=S.region_encounters(0,region)
 var b:Array[Dictionary]=S.region_encounters(3,region)
 ck(a.size()==7,"starting region has seven regular encounters")
 ck(b.size()==8,"advanced region retains seven regular and one boss")
 ck(a[3]["kind"]=="stag","stag encounter preserved")
 ck(a[5]["kind"]=="fox","even-region wildlife")
 ck(S.region_encounters(1,region)[5]["kind"]=="elk","odd-region wildlife")
 ck(a[6]["variant"]=="moon","first rare encounter")
 ck(S.region_encounters(1,region)[6]["variant"]=="dusk","second rare encounter")
 ck(S.region_encounters(2,region)[6]["variant"]=="elder","third rare encounter")
 ck(b[7]["boss"] and b[7]["kind"]=="wolf","regional boss identity and slot")
 ck(S.region_encounters(3,region)==b,"deterministic layout for replay")
 ck(a[0]["at"]!=b[0]["at"],"zones rotate encounter placements")
 ck(a[0]["at"].distance_to(Vector3(100,0,100))>=9.0,"spawns stay outside immediate center")
 ck(S.region_encounters(3,{"at":Vector2(INF,0),"monster":"wolf"}).is_empty(),"invalid region center rejected")
 ck(S.region_encounters(2,{"at":Vector2.ZERO}).is_empty(),"missing monster rejected")
 ck(S.health_scale(1,"normal",false)==1.0,"base HP unchanged")
 ck(S.health_scale(3,"elder",true)>S.health_scale(3,"normal",false),"boss health remains higher")
 ck(is_finite(S.health_scale(2147483647,"elder",true)),"huge zone cannot overflow HP scale")
 ck(load("res://scenes/main.tscn")!=null,"main scene resource loads")
 print("region failures: %d" % failures)
 quit(1 if failures else 0)
