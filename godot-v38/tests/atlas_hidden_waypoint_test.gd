extends SceneTree
## Exercise actual atlas left-click handling with discovered/hidden chambers.
const NAV=preload("res://scripts/systems/navigation_map.gd")
var failures:int=0
var selected:Array[Vector3]=[]

func check(ok:bool,label:String)->void:
 if ok:print("PASS atlas waypoint: "+label)
 else:
  failures+=1
  push_error("FAIL atlas waypoint: "+label)

func _on_waypoint(point:Vector3)->void:
 selected.append(point)

func _click(map:Control,world_x:float)->void:
 var click:=InputEventMouseButton.new()
 click.button_index=MOUSE_BUTTON_LEFT
 click.pressed=true
 # Test map's actual 300x240 chart with 100m half-width.
 click.position=Vector2(150.0+world_x*1.08,120.0)
 map.call("_gui_input",click)

func _initialize()->void:
 call_deferred("_run")

func _run()->void:
 var atlas:Control=NAV.new()
 root.add_child(atlas)
 atlas.size=Vector2(300,240)
 atlas.set("metres_per_half_width",100.0)
 atlas.call("update_navigation",Vector3.ZERO,Vector3.ZERO,false)
 atlas.connect("waypoint_selected",Callable(self,"_on_waypoint"))
 var rooms:Array[Vector3]=[Vector3.ZERO,Vector3(20,0,0)]
 var radii:Array[float]=[6.0,6.0]
 atlas.call("set_dungeon_context",true,rooms,radii)
 atlas.call("set_chamber_markers",Array([],TYPE_VECTOR3,"",null))

 _click(atlas,0.0)
 check(selected.is_empty(),"unknown chamber center refuses left-click waypoint")
 _click(atlas,20.0)
 check(selected.is_empty(),"second unknown chamber center is also blocked")
 _click(atlas,28.0)
 check(selected.is_empty(),"outside click cannot snap into hidden chamber")

 var known:Array[Vector3]=[Vector3.ZERO]
 atlas.call("set_chamber_markers",known)
 _click(atlas,0.0)
 check(selected.size()==1 and selected[0].is_equal_approx(Vector3.ZERO),"discovered chamber accepts waypoint")
 _click(atlas,20.0)
 check(selected.size()==1,"discovery of one room does not reveal the other")
 _click(atlas,28.0)
 check(selected.size()==1,"snap near unknown room stays blocked")

 _click(atlas,40.0)
 check(selected.size()==2 and selected[1].is_equal_approx(Vector3(40,0,0)),"ordinary world location outside hidden rooms remains selectable")
 var all_known:Array[Vector3]=[Vector3.ZERO,Vector3(20,0,0)]
 atlas.call("set_chamber_markers",all_known)
 _click(atlas,20.0)
 check(selected.size()==3 and selected[2].is_equal_approx(Vector3(20,0,0)),"newly discovered second chamber becomes selectable")

 atlas.call("set_dungeon_context",false,rooms,radii)
 _click(atlas,20.0)
 check(selected.size()==4 and selected[3].is_equal_approx(Vector3(20,0,0)),"world mode preserves ordinary waypoint selection")

 atlas.queue_free()
 print("MOONFALL ATLAS WAYPOINT "+("PASS" if failures==0 else "FAIL"))
 quit(0 if failures==0 else 1)
