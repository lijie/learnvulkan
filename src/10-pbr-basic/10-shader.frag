#version 450

layout (binding = 0) uniform UBOShared
{
	vec4 camera_position;
	vec4 light_direction;
    vec4 light_color;
} ubo_shared;

layout (binding = 2) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inWorldPosition;

layout (location = 0) out vec4 outFragColor;

layout (binding = 3) uniform UBOFrag
{
	vec4 color;
	float roughness;
	float metallic;
	float padding[2];
} ubo_frag;

void main() 
{
	vec3 N = normalize(inNormal);
	vec3 V = normalize(ubo_shared.camera_position.xyz - inWorldPosition);

	vec4 color = texture(samplerColor, inUV);
	outFragColor = vec4(ubo_frag.color.xyz, 1.0);	
}