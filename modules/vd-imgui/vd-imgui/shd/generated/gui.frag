"#version 450 core\n"
"layout(location = 0) out vec4 fColor;\n"
"layout(set=0, binding=0) uniform sampler2D sTexture;\n"
"layout(location = 0) in struct { vec4 Color; vec2 UV; } In;\n"
"void main()\n"
"{\n"
"    fColor = In.Color * texture(sTexture, In.UV.st);\n"
"}\n"
"";
