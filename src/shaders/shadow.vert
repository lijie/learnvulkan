#version 450

layout (location = 0) in vec3 inPos;

layout (set = 0, binding = 0) uniform UBOShared {
    vec4 camera_position;
    vec4 light_direction;
    vec4 light_color;
    mat4 light_mvp;
    mat4 projection;
    mat4 view;
} uboShared;

layout (set = 1, binding = 0) uniform UBOModel {
	mat4 model;
} uboModel;

void main()
{
	gl_Position = uboShared.light_mvp * uboModel.model * vec4(inPos, 1.0);
}