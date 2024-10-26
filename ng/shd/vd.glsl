
struct Vertex {
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
}; 

layout (set = 0, binding = 0) uniform VD_R_SceneData {
    mat4 view;
    mat4 proj;
    mat4 obj;
    vec3 sun_direction;
    vec3 sun_color;
} vd_scene_data;

vec4 object_space_to_ndc(vec3 position) {
    return vd_scene_data.proj * vd_scene_data.obj * vec4(position, 1.0);
}
