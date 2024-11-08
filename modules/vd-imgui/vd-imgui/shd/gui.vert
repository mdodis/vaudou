#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require
#include "vd.structs.glsl"

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout (push_constant) uniform constants {
    VertexBuffer vertex_buffer;
    vec2         scale;
    vec2         translate;
} pc;

void main()
{
    Vertex v = pc.vertex_buffer.vertices[gl_VertexIndex];
    Out.Color = v.color;
    Out.UV = vec2(v.uv_x, v.uv_y);
    gl_Position = vec4(v.position.xy * pc.scale + pc.translate, 0, 1);
}
