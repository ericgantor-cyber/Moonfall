extends RefCounted
## v36.1 world art palette. All geometry is decorative, visual-only.
const REGION_NAMES=["Bronze Sanctuary","Witchwood Heart","Moonmist Beacon","Cinder Quarry","Shattered Eclipse","Bloodmoon Citadel"]
const COLORS:=[
 Color("d4b56d"), Color("84e1c4"), Color("70c9ed"),
 Color("fa9b65"), Color("b59cff"), Color("ff658a")
]
const STONE:=[
 Color("555145"), Color("354a44"), Color("314958"),
 Color("665044"), Color("554860"), Color("482c43")
]
static func details_enabled(settings:Dictionary,quality:String)->bool:
 return int(settings.get("environment_detail","6"))>2 and quality!="Smooth"
static func hero_tint(id:String)->Color:
 return Color("af80ff") if id=="nyra" else Color("f0a56b")
