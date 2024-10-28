#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require
#include "vd.glsl"

//shader input
layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

//output write
layout (location = 0) out vec4 outFragColor;

layout (set = 1, binding = 0) uniform sampler2D dsTexture;

void main() 
{
    vec3 diffuseColor = texture(dsTexture, inUV).xyz;
    float roughness = 0.4 * 0.4;
    float metallic = 0.8;
    vec3 Lo = vec3(0.0);

    {
        vec3 v = normalize(eye_position() - inWorldPos);
        vec3 n = inNormal;
        vec3 l = normalize(-vd_scene_data.sun_direction);
        vec3 h = normalize(l + v);

        float NoV = abs(dot(n, v)) + 1e-5;
        float NoL = clamp(dot(n, l), 0.0, 1.0);
        float NoH = clamp(dot(n, h), 0.0, 1.0);
        float LoH = clamp(dot(l, h), 0.0, 1.0);
        float HoV = clamp(dot(h, v), 0.0, 1.0);

        float f0 = 0.04;
        float D = d_ggx(NoH, roughness);
        float V = v_smithggx_correlated(NoV, NoL, roughness);
        vec3  F = f_schlick(HoV, vec3(f0));

        vec3 Fr = (D * V) * F;
        float denom = 4.0 * NoV * NoL + 0.0001;
        Fr = Fr / denom;

        vec3 Fd = diffuseColor * fd_lambert();

        vec3 kS = F;

        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 radiance = vec3(1,1,1);
        Lo += ((kD * Fd) + Fr) * radiance * NoL;
    }

    vec3 ambient = vec3(0.09) * diffuseColor;
    Lo += ambient;

    vec3 color = Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    // outFragColor = texture(dsTexture, inUV);
    outFragColor = vec4(Lo, 1.0);
}
