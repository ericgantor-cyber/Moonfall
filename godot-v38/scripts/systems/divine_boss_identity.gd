extends RefCounted
## Canonical identities for future divine boss encounters.
## The original art paths are intentionally unchanged.
const BOSSES:Dictionary={
 "z":{"name":"Z","role":"god_boss","art":"res://assets/heroes/Z_Original.webp"},
 "nyra":{"name":"Nyra","role":"god_boss","art":"res://assets/heroes/Nyra_Original.png"}
}
static func boss(id:String)->Dictionary:
 return BOSSES.get(id,{}).duplicate()
static func is_divine_boss(id:String)->bool:
 return BOSSES.has(id)
