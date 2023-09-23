#version 450

layout (binding = 2) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec4 color = texture(samplerColor, inUV);
	outFragColor = vec4(0.9, 0.9, 0.9, 1.0);	
}