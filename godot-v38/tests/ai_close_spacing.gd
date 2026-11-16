extends SceneTree
# Run: godot --headless --path godot-v38 --script res://tests/ai_close_spacing.gd
const Rules = preload("res://scripts/systems/ai_rules.gd")

func _initialize() -> void:
 var distant:Vector3=Rules.approach(Vector3(1,0,0),5.0,2,false,1,6.0)
 var contact:Vector3=Rules.approach(Vector3(1,0,0),5.0,2,false,1,1.0)
 var other_side:Vector3=Rules.approach(Vector3(1,0,0),5.0,3,false,1,1.0)
 assert(contact.z>distant.z, "Contact range should favor orbiting")
 assert(contact.x>0.0, "Enemy must keep approaching its target")
 assert(other_side.z<0.0, "Odd spawn IDs should orbit opposite direction")
 assert(absf(contact.length()-5.0)<0.001, "Orbit must preserve movement speed")
 assert(Rules.approach(Vector3.ZERO,5.0,2,false,1,1.0)==Vector3.ZERO)
 assert(Rules.approach(Vector3(1,0,0),5.0,2,true,2,1.0).z==0.0, "Boss behavior must remain unchanged")
 print("AI close-range spacing regression passed")
 quit(0)
