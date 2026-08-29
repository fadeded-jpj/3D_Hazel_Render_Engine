#type vertex
#version 460 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main()
{
	v_TexCoord = a_TexCoord;
	gl_Position = vec4(a_Pos, 1.0);
}

#type fragment
#version 460 core

layout(location = 0) out vec4 o_Color;

in vec2 v_TexCoord;

uniform vec3 u_AmbientColor;

uniform sampler2D u_GBufferAlbedoAlpha;
uniform sampler2D u_GBufferNormalRoughness;
uniform sampler2D u_GBufferEmissiveMetallic;
uniform sampler2D u_GBufferDepth;
uniform sampler2D u_SSAOTexture;
uniform sampler2D u_ShadowMask;

uniform int u_SSAOEnabled;
uniform int u_DebugView;

#include "Common/LightSource.glsl"
#include "Common/ShadowSource.glsl"
#include "Common/SkyEnvironment.glsl"

const float PI = 3.1415926;

vec3 ReconstructWorldPosition(vec2 uv, float depth)
{
	vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	vec4 world = u_Camera.InverseViewProj * clip;
	return world.xyz / world.w;
}

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

vec3 EvaluateBlinnPhong(
	vec3 baseColor,
	vec3 normal,
	vec3 lightDir,
	vec3 viewDir,
	float metallic,
	float roughness)
{
	float NoL = max(dot(normal, lightDir), 0.0);
	if (NoL <= 0.0)
		return vec3(0.0);

	vec3 halfVector = SafeNormalize(lightDir + viewDir);
	float NoH = max(dot(normal, halfVector), 0.0);
	float shininess = mix(256.0, 4.0, roughness * roughness);

	vec3 diffuseColor = baseColor * (1.0 - metallic);
	vec3 specularColor = mix(vec3(0.04), baseColor, metallic);
	vec3 diffuse = diffuseColor * NoL;
	vec3 specular = specularColor * pow(NoH, shininess) * NoL;
	return diffuse + specular;
}

vec3 FresnelSchlick(float cosineTheta, vec3 F0)
{
	float x = 1.0 - clamp(cosineTheta, 0.0, 1.0);
	float x2 = x * x;
	float x5 = x2 * x2 * x;
	return F0 + (1.0 - F0) * x5;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float alpha = roughness * roughness;
	float a2 = alpha * alpha;

	// float NdotH = max(dot(N, H), 0.0);
	float NdotH = clamp(dot(N, H), 0.0, 1.0);
	float NdotH2 = NdotH * NdotH;

	float denom = NdotH2 * (a2 - 1.0) + 1.0;

	return a2 / (PI * denom * denom);
}

float GeoSchlickGGX(float NdotX, float roughness)
{
	float r = roughness + 1.0;
	float k = r * r / 8.0;

	return NdotX / (NdotX * (1.0 - k) + k);
}

float GeoSchlick(vec3 N, vec3 V, vec3 L, float roughness)
{
	//float NdotV = max(dot(N, V), 0.0);
	//float NdotL = max(dot(N, L), 0.0);
	float NdotV = clamp(dot(N, V), 0.0, 1.0);
	float NdotL = clamp(dot(N, L), 0.0, 1.0);


	float G1NV = GeoSchlickGGX(NdotV, roughness);
	float G1NL = GeoSchlickGGX(NdotL, roughness);

	return G1NV * G1NL;
}

vec3 EvaluatePBR(
	vec3 baseColor, vec3 normal, vec3 lightDir,
	vec3 viewDir, float metallic, float roughness
)
{
	vec3 F0 = mix(vec3(0.04), baseColor, metallic);
	vec3 halfDir = SafeNormalize(lightDir + viewDir);
	float VdotH = max(dot(viewDir, halfDir), 0.0);
	float NdotV = max(dot(normal, viewDir), 0.0);
	float NdotL = max(dot(normal, lightDir), 0.0);

	// specular
	float G = GeoSchlick(normal, viewDir, lightDir, roughness);
	float D = DistributionGGX(normal, halfDir, roughness);
	vec3 F = FresnelSchlick(VdotH, F0);

	vec3 specular = D * G * F / max(4.0 * NdotL * NdotV, 0.0001);

	// diffuse
	vec3 kd = (vec3(1.0) - F) * (1.0 - metallic);
	vec3 diffuse = kd * baseColor / PI;

	return (diffuse + specular) * NdotL;
}

vec3 CalculateDirectLighting(
	vec3 baseColor,
	vec3 normal,
	vec3 viewDir,
	vec3 worldPos,
	float metallic,
	float roughness)
{
	vec3 result = vec3(0.0);

	for (int i = 0; i < u_LightCount; ++i)
	{
		int lightType = int(u_Lights[i].DirectionType.w);
		vec3 lightColor = u_Lights[i].ColorIntensity.rgb * u_Lights[i].ColorIntensity.a;
		vec3 lightDir = vec3(0.0);


		float attenuation = 1.0;

		if (lightType == 0)
		{
			lightDir = SafeNormalize(-u_Lights[i].DirectionType.xyz);
		}
		else
		{
			vec3 toLight = u_Lights[i].PositionRange.xyz - worldPos;
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

		float NdotL = dot(normal, lightDir);
		if (NdotL <= 0.0)
			continue;

		float visibility = 1.0;
		//float visibility = texture(u_ShadowMask, v_TexCoord).r;
		if (i == u_ShadowLightIndex)
		{
			//float shadowClass =
			//	texture(u_ShadowMask, v_TexCoord).r;
			//float characterClass = texture(u_ShadowMask, v_TexCoord).g;
			float sceneVisibility = CalculateSceneVisibility(
					worldPos,
					normal,
					lightDir,
					lightType,
					u_Lights[i].PositionRange.xyz);
			float characterVisibility = CalculateCharacterVisibility(
					worldPos, normal, lightDir, lightType);
			visibility = min(sceneVisibility, characterVisibility);
		}

		result += EvaluatePBR(
			baseColor,
			normal,
			lightDir,
			viewDir,
			metallic,
			roughness) * lightColor * attenuation * visibility;
	}

	return result;
}

void main()
{
	float depth = texture(u_GBufferDepth, v_TexCoord).r;
	if (depth >= 1.0)
	{
		vec3 skyColor = SampleSkyBox(v_TexCoord) * 1.1f;
		o_Color = vec4(skyColor, 1.0);
		return;
	}

	vec4 albedoAlpha = texture(u_GBufferAlbedoAlpha, v_TexCoord);
	vec4 normalRoughness = texture(u_GBufferNormalRoughness, v_TexCoord);
	vec4 emissiveMetallic = texture(u_GBufferEmissiveMetallic, v_TexCoord);

	vec3 baseColor = albedoAlpha.rgb;
	vec3 normal = SafeNormalize(normalRoughness.rgb);
	float roughness = clamp(normalRoughness.a, 0.04, 1.0);
	float metallic = clamp(emissiveMetallic.a, 0.0, 1.0);
	vec3 worldPos = ReconstructWorldPosition(v_TexCoord, depth);
	vec3 viewDir = SafeNormalize(u_Camera.CameraPositionAndTime.xyz - worldPos);

	float ambientOcclusion = u_SSAOEnabled != 0
		? texture(u_SSAOTexture, v_TexCoord).r
		: 1.0;

	vec3 color = u_AmbientColor * baseColor * ambientOcclusion;
	color += CalculateDirectLighting(
		baseColor, normal, viewDir, worldPos, metallic, roughness);
	color += emissiveMetallic.rgb;

	if (u_DebugView == 1)
		color = baseColor;
	else if (u_DebugView == 2)
		color = normal * 0.5 + 0.5;
	else if (u_DebugView == 3)
		color = emissiveMetallic.rgb;
	else if (u_DebugView == 4)
		color = vec3(albedoAlpha.a);
	else if (u_DebugView == 5)
	{
		float viewDepth = LinearizeViewDepth(
			depth, u_Camera.CameraClip.x, u_Camera.CameraClip.y);
		int cascade = SelectCascade(viewDepth, u_CSMSplits);
		const vec3 cascadeColors[4] = vec3[4](
			vec3(1.0, 0.1, 0.1),
			vec3(0.1, 1.0, 0.1),
			vec3(0.1, 0.3, 1.0),
			vec3(1.0, 1.0, 0.1));
		color = cascade >= 0 ? cascadeColors[cascade] : vec3(0.05);
	}

	o_Color = vec4(color, 1.0);
}
