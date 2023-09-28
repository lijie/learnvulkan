#version 450

const float PI = 3.14159265359;

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

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 F_Schlick(float cosTheta, float metallic)
{
	vec3 F0 = mix(vec3(0.04), ubo_frag.color.xyz, metallic); // * material.specular
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); 
	return F;    
}

// Specular BRDF composition --------------------------------------------
vec3 BRDF(vec3 L, vec3 V, vec3 N, float metallic, float roughness)
{
	// Precalculate vectors and dot products	
	vec3 H = normalize (V + L);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);

	// Light color fixed
	vec3 lightColor = vec3(1.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0)
	{
		float rroughness = max(0.05, roughness);
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, roughness); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		vec3 F = F_Schlick(dotNV, metallic);

		vec3 spec = D * F * G / (4.0 * dotNL * dotNV);

		color += spec * dotNL * lightColor;
	}

	return color;
}

void main() 
{
	vec3 N = normalize(inNormal);
	vec3 V = normalize(ubo_shared.camera_position.xyz - inWorldPosition);

	vec3 Lo = vec3(0.0);
	vec3 L = normalize(ubo_shared.light_direction.xyz);
	Lo += BRDF(L, V, N, ubo_frag.metallic, ubo_frag.roughness);

	vec3 color = ubo_frag.color.xyz * 0.02;
	color += Lo;

	// Gamma correct
	color = pow(color, vec3(0.4545));

	// vec4 color = texture(samplerColor, inUV);
	outFragColor = vec4(color, 1.0);	
}