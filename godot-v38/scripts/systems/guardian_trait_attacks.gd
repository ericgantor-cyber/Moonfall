extends RefCounted
## Four physical warning footprints and counterplay checks for native bosses.
## Pure math, no new combat scheduler or extra enemy actors.
const WARNING_SECONDS:float=1.35
const ENGAGE_RANGE_SQ:float=11.0*11.0
const WARNING_COLORS:Array[Color]=[
 Color("b9a2ff",0.52),Color("ff8855",0.52),
 Color("69d598",0.52),Color("f184b1",0.52)
]
const ATTACK_NAMES:Array[String]=[
 "Moonbound Ring","Ember Charge","Venom Pool","Eclipse Cross"
]
const COUNTER_HINTS:Array[String]=[
 "Move inside the ring or outside its rim",
 "Sidestep the marked charge lane",
 "Leave the green pool before it detonates",
 "Move diagonally off the crossed lanes"
]

static func valid_trait(trait_id:int)->bool:
 return trait_id>=0 and trait_id<4

static func cooldown(trait_id:int,phase:int)->float:
 return maxf(4.7,8.0-0.65*float(clampi(phase,1,3))-0.22*float(clampi(trait_id,0,3)))

static func damage(trait_id:int,phase:int)->float:
 return 11.0+3.0*float(clampi(phase,1,3))+2.0*float(clampi(trait_id,0,3))

static func hit(trait_id:int,boss:Vector3,target:Vector3,hero:Vector3,facing:Vector2)->bool:
 if not valid_trait(trait_id):return false
 var hero_2d:Vector2=Vector2(hero.x,hero.z)
 var boss_2d:Vector2=Vector2(boss.x,boss.z)
 var diff:Vector2=hero_2d-boss_2d
 match trait_id:
  0:
   # Moonbound: donut shape, safe in centre or beyond outer rim.
   var radius:float=diff.length()
   return radius>=2.1 and radius<=5.2
  1:
   # Ember: a narrow 7m forward lane. Sidestepping breaks the hit.
   var forward:Vector2=facing.normalized()
   if forward.length_squared()<0.5:return false
   var along:float=diff.dot(forward)
   var across:float=absf(diff.cross(forward))
   return along>=0.0 and along<=7.0 and across<=1.2
  2:
   # Venom: location is locked at wind-up, never tracks a fleeing player.
   return hero_2d.distance_squared_to(Vector2(target.x,target.z))<=2.8*2.8
  3:
   # Eclipse: crossing axes with four diagonal safe corridors.
   return diff.length_squared()<=7.0*7.0 and (absf(diff.x)<=0.9 or absf(diff.y)<=0.9)
 return false
