#ifndef VD_DEFAULT_SHADERS_H
#define VD_DEFAULT_SHADERS_H

const char *VD_PBROPAQUE_VERT = 
"#version 450\n"
"\n"
"layout (location = 0) out vec3 outColor;\n"
"\n"
"void main() \n"
"{\n"
	"//const array of positions for the triangle\n"
	"const vec3 positions[3] = vec3[3](\n"
		"vec3(1.f,1.f, 0.0f),\n"
		"vec3(-1.f,1.f, 0.0f),\n"
		"vec3(0.f,-1.f, 0.0f)\n"
	");\n"
"\n"
	"//const array of colors for the triangle\n"
	"const vec3 colors[3] = vec3[3](\n"
		"vec3(1.0f, 0.0f, 0.0f), //red\n"
		"vec3(0.0f, 1.0f, 0.0f), //green\n"
		"vec3(00.f, 0.0f, 1.0f)  //blue\n"
	");\n"
"\n"
	"//output the position of each vertex\n"
	"gl_Position = vec4(positions[gl_VertexIndex], 1.0f);\n"
	"outColor = colors[gl_VertexIndex];\n"
"}\n"
"\n";

const char *VD_PBROPAQUE_FRAG =
"#version 450\n"
"\n"
"//shader input\n"
"layout (location = 0) in vec3 inColor;\n"
"\n"
"//output write\n"
"layout (location = 0) out vec4 outFragColor;\n"
"\n"
"void main() \n"
"{\n"
	"//return red\n"
	"outFragColor = vec4(inColor,1.0f);\n"
"}\n"
"\n";


#endif // !VD_DEFAULT_SHADERS_H