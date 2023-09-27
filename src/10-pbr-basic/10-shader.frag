#version 450

layout (binding = 2) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

layout (binding = 3) uniform UBOFrag
{
	vec3 color;
} ubo_frag;

void main() 
{
	vec4 color = texture(samplerColor, inUV);
	outFragColor = vec4(ubo_frag.color, 1.0);	
}