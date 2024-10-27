#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require
#include "vd.glsl"


layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outWorldPos;

layout(buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};

//push constants block
layout( push_constant ) uniform constants
{	
	VertexBuffer vertexBuffer;
    mat4         obj;
} PushConstants;

void main() 
{	
	//load vertex data from device adress
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

	//output data
	gl_Position = object_space_to_ndc(PushConstants.obj, v.position);

	// outColor = v.color.xyz;
	outColor = vec3(v.uv_x, v.uv_y, 1.0f);
	outUV.x = v.uv_x;
	outUV.y = v.uv_y;
    outNormal = v.normal;
    outWorldPos = (PushConstants.obj * vec4(v.position, 1.0)).xyz;
}
