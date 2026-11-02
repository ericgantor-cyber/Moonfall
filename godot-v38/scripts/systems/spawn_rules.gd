extends RefCounted
## Deterministic regional encounters. Existing spawn count, save keys, and boss schedule stay intact.
## Zones now vary ring orientation, pack spacing, roaming roles and elite placement.
static func region_encounters(zone:int, region:Dictionary) -> Array[Dictionary]:
 var result:Array[Dictionary]=[]
 if not region.has("at") or not region.has("monster"):return result
 if not (region["at"] is Vector2):return result
 var center:Vector2=region["at"]
 if not is_finite(center.x) or not is_finite(center.y):return result
 var kind:String=str(region["monster"])
 if kind.is_empty():return result
 var tier:int=maxi(0,zone)
 var rotation:float=float(posmod(tier*37,360))*PI/180.0
 for j in range(7):
  var theta:float=rotation+float(j)*TAU/7.0
  var radius:float=10.0+float(j%3)*3.0+float(posmod(tier+j,3))*0.75
  var at:Vector3=Vector3(center.x+cos(theta)*radius,0.0,center.y+sin(theta)*radius)
  var encounter_kind:String=kind
  if j==3:encounter_kind="stag"
  if j==5:encounter_kind="fox" if tier%2==0 else "elk"
  var rarity:String="normal"
  if j==6:rarity=["moon","dusk","elder"][tier%3]
  result.append({"at":at,"kind":encounter_kind,"zone":zone,"variant":rarity,"boss":false})
 if zone>0:
  # Keep the original 21-unit offset, but rotate it with the region's zone.
  var boss_angle:float=rotation+PI/4.0
  var boss_radius:float=sqrt(2.0)*21.0
  result.append({"at":Vector3(center.x+cos(boss_angle)*boss_radius,0.0,center.y+sin(boss_angle)*boss_radius),"kind":kind,"zone":zone,"variant":"elder","boss":true})
 return result

static func health_scale(tier:int,variant:String,boss:bool) -> float:
 var clamped_tier:int=clampi(tier,1,1000)
 var scale:float=1.0+float(clamped_tier-1)*0.3
 if variant!="normal":scale*=1.35
 if boss:scale*=3.2
 return minf(1000.0,scale)
