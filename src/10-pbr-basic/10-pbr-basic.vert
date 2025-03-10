#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (set = 0, binding = 0) uniform UBOShared
{
	vec4 camera_position;
	vec4 light_direction;
    vec4 light_color;
	mat4 light_mvp;
	mat4 projection;
	mat4 view;
} ubo_shared;

layout (set = 1, binding = 0) uniform UBO 
{
	mat4 model;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outWorldPosition;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main() 
{
	outUV = inUV;

	vec3 worldPos = vec3(ubo.model * vec4(inPos, 1.0));

	gl_Position = ubo_shared.projection * ubo_shared.view * ubo.model * vec4(inPos.xyz, 1.0);

    vec4 pos = ubo.model * vec4(inPos, 1.0);
	// outNormal = mat3(inverse(transpose(ubo.model))) * inNormal;
	outNormal = mat3(ubo.model) * inNormal;
	outWorldPosition = worldPos;
}
