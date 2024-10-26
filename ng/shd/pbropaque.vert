#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require
#include "vd.glsl"


layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;

layout(buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};

//push constants block
layout( push_constant ) uniform constants
{	
	VertexBuffer vertexBuffer;
} PushConstants;

void main() 
{	
	//load vertex data from device adress
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

	//output data
	gl_Position = object_space_to_ndc(v.position);
	// outColor = v.color.xyz;
	outColor = vec3(v.uv_x, v.uv_y, 1.0f);
	outUV.x = v.uv_x;
	outUV.y = v.uv_y;
}
