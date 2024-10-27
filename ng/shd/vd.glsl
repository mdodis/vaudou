const float PI = 3.14159265359;

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
    vec3 sun_direction;
    vec3 sun_color;
} vd_scene_data;

vec4 object_space_to_ndc(mat4 obj, vec3 position) {
    return vd_scene_data.proj * vd_scene_data.view * obj * vec4(position, 1.0);
}

vec3 eye_position() {
    mat3 rotation = mat3(vd_scene_data.view);
    vec3 translation = vec3(vd_scene_data.view[3]);
    return -rotation * translation;
}

vec3 eye_direction() {
    return -vec3(vd_scene_data.view[2][0], vd_scene_data.view[2][1], vd_scene_data.view[2][2]);
}

// @todo: Implementation of the specular D term in GLSL optimized for fp16
float d_ggx(float NoH, float a) {
    float a2 = a * a;
    // float f = (NoH * a2 - NoH) * NoH + 1.0;
    float f = (NoH * NoH * (a2 - 1.0) + 1.0);
    return a2 / (PI * f * f);
}

vec3 f_schlick(float u, vec3 f0) {
    return f0 + (1.0 - f0) * pow(clamp(1.0 - u, 0.0, 1.0), 5.0);
}

float g_schlickggx(float NoV, float a)
{
    float r = (a + 1.0);
    float k = (r * r) / 8.0;
    float nom = NoV;
    float denom = NoV * (1.0 - k) + k;

    return nom / denom;
}

float v_smithggx_correlated(float NoV, float NoL, float a) {
    float ggx2 = g_schlickggx(NoV, a);
    float ggx1 = g_schlickggx(NoL, a);
    return ggx1 * ggx2;
}

float fd_lambert() {
    return 1.0 / PI;
}
