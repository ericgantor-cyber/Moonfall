shader_type spatial;
render_mode cull_disabled;

// GPU-only ambient movement. Combat creates a localized impulse and fades out.
// No per-fern CPU update, collision change, or additional light source.
uniform vec3 impact_center = vec3(0.0, 0.0, 0.0);
uniform float impact_strength : hint_range(0.0, 1.0) = 0.0;
varying vec4 instance_tint;

void vertex() {
 instance_tint = COLOR;
 vec3 world = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
 float dist = distance(world.xz, impact_center.xz);
 float nearby = (1.0 - smoothstep(0.0, 5.0, dist)) * impact_strength;
 float tip = clamp(VERTEX.y / 0.65, 0.0, 1.0);
 float wind = sin(TIME * 2.0 + world.x * 0.72 + world.z * 0.46);
 VERTEX.x += tip * (0.045 * wind + nearby * 0.14);
 VERTEX.z += tip * (0.034 * cos(TIME * 1.8 + world.z * 0.62) + nearby * 0.11);
}

void fragment() {
 ALBEDO = vec3(0.32, 0.52, 0.40) * instance_tint.rgb;
 ROUGHNESS = 0.92;
}
