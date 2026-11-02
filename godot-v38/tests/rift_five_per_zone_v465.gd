extends SceneTree
const GAME=preload("res://scripts/native_v38_10.gd")
var failed:int=0
func check(value:bool,message:String)->void:
 if not value:
  failed+=1
  push_error(message)
func _initialize()->void:call_deferred("run")
func run()->void:
 var game=GAME.new()
 root.add_child(game)
 var sites:Array=game.get("rifts")
 var regions:Array=game.get("REGIONS")
 check(sites.size()==regions.size()*5,"five dungeon sites per zone")
 for zone in range(regions.size()):
  var total:int=0
  for site in sites:
   if int(site["zone"])==zone:total+=1
  check(total==5,"zone %d contains five rifts"%zone)
 check(int(sites[0]["zone"])==1 and int(sites[1]["zone"])==4 and int(sites[2]["zone"])==5,"legacy save indexes preserved")
 check(load("res://scenes/main.tscn")!=null,"active scene loads")
 game.queue_free()
 quit(1 if failed else 0)
