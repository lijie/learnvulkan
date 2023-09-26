#version 450

layout (binding = 2) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

layout (binding = 3) uniform UBO 
{
	vec3 color;
} ubo;

void main() 
{
	vec4 color = texture(samplerColor, inUV);
	outFragColor = vec4(ubo.color, 1.0);	
}