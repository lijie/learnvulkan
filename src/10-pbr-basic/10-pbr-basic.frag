#version 450

const float PI = 3.14159265359;

layout (set = 0, binding = 0) uniform UBOShared
{
	vec4 camera_position;
	vec4 light_direction;
    vec4 light_color;
	mat4 light_mvp;
	mat4 projection;
	mat4 view;
} ubo_shared;

layout (set = 0, binding = 1) uniform sampler2D shadowMap;

layout (set = 1, binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inWorldPosition;
layout (location = 3) in vec4 inShadowCoord;

layout (location = 0) out vec4 outFragColor;

layout (set = 1, binding = 2) uniform UBOFrag
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

float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( shadowMap, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = 0.1;
		}
	}
	return shadow;
}

float filterPCF(vec4 sc)
{
	ivec2 texDim = textureSize(shadowMap, 0);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y));
			count++;
		}
	
	}
	return shadowFactor / count;
}

// Specular BRDF composition --------------------------------------------
vec3 BRDF(vec3 L, vec3 V, vec3 N, float metallic, float roughness)
{
	// Precalculate vectors and dot products	
	vec3 H = normalize (V + L);
	float dotNV = abs(dot(N, V)) + 1e-5;  // clamp(dot(N, V), 0.01, 1.0);
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

	// L direction is fragment to light
	vec3 L = normalize(-ubo_shared.light_direction.xyz);
	Lo += BRDF(L, V, N, ubo_frag.metallic, ubo_frag.roughness);

	vec3 color = ubo_frag.color.xyz / PI;
	
    float shadow = filterPCF(inShadowCoord / inShadowCoord.w);
    
    color += Lo;
    color *= shadow;

	// Gamma correct
	color = pow(color, vec3(0.4545));

	outFragColor = vec4(color, 1.0);
}
