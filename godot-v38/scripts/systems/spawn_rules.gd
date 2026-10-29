extends RefCounted
## Deterministic encounter layouts. Does not create Nodes or touch save data.
static func region_encounters(zone:int, region:Dictionary) -> Array[Dictionary]:
 var result:Array[Dictionary]=[]
 var center:Vector2=region["at"]
 for j in range(7):
  var theta:=float(j)*TAU/7.0
  var at:=Vector3(center.x+cos(theta)*float(10+j%3*3),0.0,center.y+sin(theta)*float(10+j%3*3))
  var kind:=str(region["monster"])
  if j==3:kind="stag"
  if j==5:kind="fox" if zone%2==0 else "elk"
  var rarity:="normal"
  if j==6:rarity=["moon","dusk","elder"][zone%3]
  result.append({"at":at,"kind":kind,"zone":zone,"variant":rarity,"boss":false})
 if zone>0:
  result.append({"at":Vector3(center.x+21,0,center.y+21),"kind":str(region["monster"]),"zone":zone,"variant":"elder","boss":true})
 return result

static func health_scale(tier:int,variant:String,boss:bool) -> float:
 var scale:=1.0+float(tier-1)*0.3
 if variant!="normal":scale*=1.35
 if boss:scale*=3.2
 return scale
