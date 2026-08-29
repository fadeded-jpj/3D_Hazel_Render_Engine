#type vertex
#version 460 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Tangent;
layout(location = 3) in vec2 a_TexCoord;
layout(location = 4) in ivec4 a_BoneIDs;
layout(location = 5) in vec4 a_BoneWeights;

layout(std430, binding = 0) readonly buffer BoneBuffer
{
	mat4 u_BoneMatrices[];
};

#include "Common/CameraUniform.glsl"

out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_WorldPos;
out vec4 v_Tangent;

uniform mat4 u_Model;
uniform int u_HasSkeleton;

void main()
{
	vec4 localPos = vec4(a_Pos, 1.0);
	vec3 localNormal = a_Normal;
	vec3 localTangent = a_Tangent.xyz;

	if (u_HasSkeleton == 1)
	{
		mat4 skinMatrix = mat4(0.0);
		float totalWeight = 0.0;
		for (int i = 0; i < 4; ++i)
		{
			int boneID = a_BoneIDs[i];
			float weight = a_BoneWeights[i];
			if (boneID >= 0 && boneID < 2048 && weight > 0.0)
			{
				skinMatrix += u_BoneMatrices[boneID] * weight;
				totalWeight += weight;
			}
		}

		if (totalWeight > 0.0)
		{
			localPos = skinMatrix * localPos;
			localNormal = mat3(skinMatrix) * localNormal;
			localTangent = mat3(skinMatrix) * localTangent;
		}
	}

	mat3 modelMatrix = mat3(u_Model);
	mat3 normalMatrix = transpose(inverse(modelMatrix));
	vec3 normal = normalize(normalMatrix * localNormal);
	vec3 tangent = normalize(modelMatrix * localTangent);
	tangent = normalize(tangent - normal * dot(normal, tangent));

	vec4 worldPos = u_Model * localPos;
	v_Normal = normal;
	v_Tangent = vec4(tangent, a_Tangent.w);
	v_TexCoord = a_TexCoord;
	v_WorldPos = worldPos.xyz;
	gl_Position = u_Camera.ViewProj * worldPos;
}

#type fragment
#version 460 core

layout(location = 0) out vec4 o_Color;

in vec3 v_Normal;
in vec2 v_TexCoord;
in vec3 v_WorldPos;
in vec4 v_Tangent;

uniform vec3 u_AmbientColor;

uniform float u_AlphaCutoff;
uniform int u_AlphaMode;

uniform sampler2D u_BaseColorTexture;
uniform sampler2D u_NormalTexture;
uniform sampler2D u_EmissiveTexture;
uniform sampler2D u_MetallicRoughnessTexture;

uniform vec4 u_BaseColorFactor;
uniform vec3 u_EmissiveFactor;
uniform float u_EmissiveStrength;
uniform float u_MetallicFactor;
uniform float u_RoughnessFactor;

uniform int u_HasBaseColorTexture;
uniform int u_HasNormalTexture;
uniform int u_HasEmissiveTexture;
uniform int u_HasMetallicRoughnessTexture;
uniform int u_Selected;
uniform int u_DebugView;

#include "Common/LightSource.glsl"
#include "Common/ShadowSource.glsl"

const float PI = 3.1415926;

vec3 SafeNormalize(vec3 value)
{
	float lengthSquared = dot(value, value);
	return lengthSquared > 0.00000001
		? value * inversesqrt(lengthSquared)
		: vec3(0.0);
}

float GetRangeAttenuation(float distanceToLight, float range)
{
	if (range <= 0.0 || distanceToLight >= range)
		return 0.0;

	float normalizedDistance = distanceToLight / range;
	float falloff = clamp(1.0 - pow(normalizedDistance, 4.0), 0.0, 1.0);
	return falloff * falloff;
}

vec3 FresnelSchlick(float cosineTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosineTheta, 5.0);
}

float DistributionGGX(vec3 normal, vec3 halfDir, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSquared = alpha * alpha;
	float NdotH = max(dot(normal, halfDir), 0.0);
	float denominator = NdotH * NdotH * (alphaSquared - 1.0) + 1.0;
	return alphaSquared / max(PI * denominator * denominator, 0.000001);
}

float GeometrySchlickGGX(float NdotX, float roughness)
{
	float r = roughness + 1.0;
	float k = r * r / 8.0;
	return NdotX / max(NdotX * (1.0 - k) + k, 0.000001);
}

float GeometrySmith(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness)
{
	float NdotV = max(dot(normal, viewDir), 0.0);
	float NdotL = max(dot(normal, lightDir), 0.0);
	return GeometrySchlickGGX(NdotV, roughness) *
		GeometrySchlickGGX(NdotL, roughness);
}

vec3 EvaluatePBR(
	vec3 baseColor,
	vec3 normal,
	vec3 lightDir,
	vec3 viewDir,
	float metallic,
	float roughness)
{
	float NdotL = max(dot(normal, lightDir), 0.0);
	float NdotV = max(dot(normal, viewDir), 0.0);
	if (NdotL <= 0.0 || NdotV <= 0.0)
		return vec3(0.0);

	vec3 halfDir = SafeNormalize(lightDir + viewDir);
	vec3 F0 = mix(vec3(0.04), baseColor, metallic);
	vec3 fresnel = FresnelSchlick(max(dot(viewDir, halfDir), 0.0), F0);
	float distribution = DistributionGGX(normal, halfDir, roughness);
	float geometry = GeometrySmith(normal, viewDir, lightDir, roughness);

	vec3 specular = distribution * geometry * fresnel /
		max(4.0 * NdotL * NdotV, 0.001);
	vec3 diffuseWeight = (vec3(1.0) - fresnel) * (1.0 - metallic);
	vec3 diffuse = diffuseWeight * baseColor / PI;

	return (diffuse + specular) * NdotL;
}

vec3 CalculateDirectLighting(
	vec3 baseColor,
	vec3 normal,
	vec3 viewDir,
	float metallic,
	float roughness)
{
	vec3 result = vec3(0.0);
	for (int i = 0; i < u_LightCount; ++i)
	{
		int lightType = int(u_Lights[i].DirectionType.w);
		vec3 lightColor = u_Lights[i].ColorIntensity.rgb * u_Lights[i].ColorIntensity.a;
		vec3 lightDir;
		float attenuation = 1.0;

		if (lightType == 0)
		{
			lightDir = SafeNormalize(-u_Lights[i].DirectionType.xyz);
		}
		else
		{
			vec3 toLight = u_Lights[i].PositionRange.xyz - v_WorldPos;
			float distanceToLight = length(toLight);
			lightDir = distanceToLight > 0.0001 ? toLight / distanceToLight : vec3(0.0);
			attenuation = GetRangeAttenuation(distanceToLight, u_Lights[i].PositionRange.w);

			if (lightType == 2)
			{
				float coneCos = dot(SafeNormalize(u_Lights[i].DirectionType.xyz), -lightDir);
				attenuation *= smoothstep(
					u_Lights[i].SpotAngles.y,
					u_Lights[i].SpotAngles.x,
					coneCos);
			}
		}

		if (attenuation <= 0.0)
			continue;

		float visibility = 1.0;
		if (i == u_ShadowLightIndex)
		{
			visibility = CalculateSceneVisibility(
				v_WorldPos,
				normal,
				lightDir,
				lightType,
				u_Lights[i].PositionRange.xyz);
			visibility = min(visibility, CalculateCharacterVisibility(
				v_WorldPos, normal, lightDir, lightType));
		}

		result += EvaluatePBR(
			baseColor, normal, lightDir, viewDir, metallic, roughness) *
			lightColor * attenuation * visibility;
	}

	return result;
}

void main()
{
	vec4 surface = u_BaseColorFactor;
	if (u_HasBaseColorTexture == 1)
		surface *= texture(u_BaseColorTexture, v_TexCoord);

	if (u_AlphaMode == 1 && surface.a < u_AlphaCutoff)
		discard;

	vec3 normal = normalize(v_Normal);
	if (!gl_FrontFacing)
		normal = -normal;

	if (u_HasNormalTexture == 1)
	{
		vec3 tangent = normalize(v_Tangent.xyz);
		tangent = normalize(tangent - normal * dot(normal, tangent));
		vec3 bitangent = normalize(cross(normal, tangent)) * v_Tangent.w;
		vec3 tangentNormal = texture(u_NormalTexture, v_TexCoord).xyz * 2.0 - 1.0;
		normal = normalize(mat3(tangent, bitangent, normal) * tangentNormal);
	}

	float metallic = u_MetallicFactor;
	float roughness = u_RoughnessFactor;
	if (u_HasMetallicRoughnessTexture == 1)
	{
		vec4 metallicRoughness = texture(u_MetallicRoughnessTexture, v_TexCoord);
		roughness *= metallicRoughness.g;
		metallic *= metallicRoughness.b;
	}
	metallic = clamp(metallic, 0.0, 1.0);
	roughness = clamp(roughness, 0.04, 1.0);

	vec3 viewDir = SafeNormalize(u_Camera.CameraPositionAndTime.xyz - v_WorldPos);
	vec3 color = u_AmbientColor * surface.rgb * (1.0 - metallic);
	color += CalculateDirectLighting(
		surface.rgb, normal, viewDir, metallic, roughness);

	vec3 emissive = u_EmissiveFactor * u_EmissiveStrength;
	if (u_HasEmissiveTexture == 1)
		emissive *= texture(u_EmissiveTexture, v_TexCoord).rgb;
	color += emissive;

	if (u_Selected == 1)
		color += vec3(0.2);

	if (u_DebugView == 1)
		color = surface.rgb;
	else if (u_DebugView == 2)
		color = normal * 0.5 + 0.5;
	else if (u_DebugView == 3)
		color = emissive;
	else if (u_DebugView == 4)
		color = vec3(0.0);

	o_Color = vec4(color, surface.a);
}
