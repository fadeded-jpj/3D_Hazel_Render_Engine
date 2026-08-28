// ---------------- Shadow resources ----------------
#include "CameraUniform.glsl"

uniform int u_ShadowEnabled;
uniform int u_ShadowLightIndex;
uniform mat4 u_ShadowViewProj;
uniform float u_ShadowBias;
uniform float u_ShadowFarPlane;

uniform int u_CSMEnabled;
uniform mat4 u_CSMViewProj[4];
uniform vec4 u_CSMSplits;
uniform float u_OverlapRatio;

uniform int u_CharacterShadowEnabled;
uniform mat4 u_CharacterShadowViewProj;
uniform float u_CharacterShadowBias;

layout(binding = 1) uniform sampler2D u_ShadowMap;
layout(binding = 2) uniform samplerCube u_PointShadowMap;
layout(binding = 3) uniform sampler2DArray u_CSMShadowMap;
layout(binding = 4) uniform sampler2D u_CharacterShadowMap;

const int SHADOW_CASCADE_COUNT = 4;
const float SHADOW_PCF_RADIUS = 2.0;

const vec2 PCF_KERNEL_9[9] = vec2[](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.707, 0.707),
	vec2(0.0, 1.0),
	vec2(-0.707, 0.707),
	vec2(-1.0, 0.0),
	vec2(-0.707, -0.707),
	vec2(0.0, -1.0),
	vec2(0.707, -0.707)
);

// ---------------- Common shadow evaluation ----------------
bool ProjectShadowCoordinate(mat4 viewProjection, vec3 worldPos, out vec3 shadowCoord)
{
	vec4 clip = viewProjection * vec4(worldPos, 1.0);
	if (abs(clip.w) < 0.000001)
		return false;

	shadowCoord = clip.xyz / clip.w;
	shadowCoord = shadowCoord * 0.5 + 0.5;

	return shadowCoord.x >= 0.0 && shadowCoord.x <= 1.0 &&
		shadowCoord.y >= 0.0 && shadowCoord.y <= 1.0 &&
		shadowCoord.z >= 0.0 && shadowCoord.z <= 1.0;
}

float CalculateDepthBias(float depthBias, vec3 normal, vec3 lightDir)
{
	float NdotL = max(dot(normal, lightDir), 0.0);

	float slopeBias = depthBias * (1.0 - NdotL);
	float minBias = depthBias * 0.01;

	return max(slopeBias, minBias);
	//return 0.0001 + depthBias * (1.0 - NdotL);
}

float EvaluateShadow2D(
	sampler2D shadowMap,
	mat4 viewProjection,
	float depthBias,
	vec3 worldPos,
	vec3 normal,
	vec3 lightDir)
{
	vec3 shadowCoord;
	if (!ProjectShadowCoordinate(viewProjection, worldPos, shadowCoord))
		return 1.0;

	float bias = CalculateDepthBias(depthBias, normal, lightDir);
	vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
	float visibility = 0.0;

	for (int i = 0; i < 9; ++i)
	{
		float closestDepth = texture(
			shadowMap,
			shadowCoord.xy + PCF_KERNEL_9[i] * texelSize * SHADOW_PCF_RADIUS).r;
		visibility += shadowCoord.z - bias <= closestDepth ? 1.0 : 0.0;
	}

	return visibility / 9.0;
}

float EvaluateShadow2DArrayLayer(
	sampler2DArray shadowMap,
	int layer,
	mat4 viewProjection,
	float depthBias,
	vec3 worldPos,
	vec3 normal,
	vec3 lightDir)
{
	vec3 shadowCoord;
	if (!ProjectShadowCoordinate(viewProjection, worldPos, shadowCoord))
		return 1.0;

	float bias = CalculateDepthBias(depthBias, normal, lightDir);
	vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0).xy);
	float visibility = 0.0;

	for (int i = 0; i < 9; ++i)
	{
		float closestDepth = texture(
			shadowMap,
			vec3(
				shadowCoord.xy + PCF_KERNEL_9[i] * texelSize * SHADOW_PCF_RADIUS,
				layer)).r;
		visibility += shadowCoord.z - bias <= closestDepth ? 1.0 : 0.0;
	}

	return visibility / 9.0;
}

float EvaluatePointShadow(
	samplerCube shadowMap,
	vec3 lightPosition,
	float farPlane,
	float depthBias,
	vec3 worldPos,
	vec3 normal)
{
	vec3 lightToFragment = worldPos - lightPosition;
	float distanceToLight = length(lightToFragment);
	if (farPlane <= 0.000001 || distanceToLight <= 0.000001)
		return 1.0;

	float currentDepth = distanceToLight / farPlane;

	if (currentDepth >= 1.0)
		return 1.0;

	vec3 sampleDirection = lightToFragment / distanceToLight;
	vec3 lightDir = -sampleDirection;
	float bias = max(
		depthBias * (1.0 - max(dot(normal, lightDir), 0.0)),
		0.0001);

	ivec2 shadowSize = textureSize(shadowMap, 0);
	float texelSize = 2.0 / float(shadowSize.x);
	vec3 helper = abs(sampleDirection.y) < 0.999
		? vec3(0.0, 1.0, 0.0)
		: vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(helper, sampleDirection));
	vec3 bitangent = normalize(cross(tangent, sampleDirection));

	float visibility = 0.0;
	for (int i = 0; i < 9; ++i)
	{
		vec3 direction = normalize(
			sampleDirection +
			tangent * PCF_KERNEL_9[i].x * texelSize * SHADOW_PCF_RADIUS +
			bitangent * PCF_KERNEL_9[i].y * texelSize * SHADOW_PCF_RADIUS);

		float closestDepth = texture(shadowMap, direction).r;
		visibility += currentDepth - bias <= closestDepth ? 1.0 : 0.0;
	}

	return visibility / 9.0;
}

// ---------------- Pure CSM helpers ----------------
float GetViewDepth(vec3 worldPos, mat4 cameraView)
{
	return -(cameraView * vec4(worldPos, 1.0)).z;
}

float LinearizeViewDepth(float depth, float nearPlane, float farPlane)
{
	float zNDC = depth * 2.0 - 1.0;
	return (2.0 * nearPlane * farPlane) /
		(farPlane + nearPlane - zNDC * (farPlane - nearPlane));
}

int SelectCascade(float viewDepth, vec4 cascadeSplits)
{
	if (viewDepth <= cascadeSplits.x)
		return 0;
	if (viewDepth <= cascadeSplits.y)
		return 1;
	if (viewDepth <= cascadeSplits.z)
		return 2;
	if (viewDepth <= cascadeSplits.w)
		return 3;
	return -1;
}

float GetCascadeNear(int cascade, float cameraNear, vec4 cascadeSplits)
{
	return cascade == 0 ? cameraNear : cascadeSplits[cascade - 1];
}

float GetCascadeFar(int cascade, vec4 cascadeSplits)
{
	return cascadeSplits[cascade];
}

bool CheckCascadeOverlap(
	float viewDepth,
	int cascade,
	float cameraNear,
	vec4 cascadeSplits,
	float overlapRatio)
{
	if (cascade >= SHADOW_CASCADE_COUNT - 1)
		return false;

	float cascadeNear = GetCascadeNear(cascade, cameraNear, cascadeSplits);
	float cascadeFar = GetCascadeFar(cascade, cascadeSplits);
	return viewDepth >= cascadeFar -
		(cascadeFar - cascadeNear) * overlapRatio;
}

float GetCascadeOverlapBlend(
	float viewDepth,
	int cascade,
	float cameraNear,
	vec4 cascadeSplits,
	float overlapRatio)
{
	float cascadeNear = GetCascadeNear(cascade, cameraNear, cascadeSplits);
	float cascadeFar = GetCascadeFar(cascade, cascadeSplits);
	float blendNear = cascadeFar -
		(cascadeFar - cascadeNear) * overlapRatio;
	return smoothstep(blendNear, cascadeFar, viewDepth);
}

float CalculateSceneVisibility(
	vec3 worldPos,
	vec3 normal,
	vec3 lightDir,
	int lightType,
	vec3 lightPosition)
{
	float sceneVisibility = 1.0;

	if (u_ShadowEnabled != 0)
	{
		if (lightType == 1)
		{
			sceneVisibility = EvaluatePointShadow(
				u_PointShadowMap,
				lightPosition,
				u_ShadowFarPlane,
				u_ShadowBias,
				worldPos,
				normal);
		}
		else if (u_CSMEnabled != 0)
		{
			float viewDepth = GetViewDepth(worldPos, u_Camera.View);
			int cascade = SelectCascade(viewDepth, u_CSMSplits);

			if (cascade >= 0 && cascade < SHADOW_CASCADE_COUNT)
			{
				sceneVisibility = EvaluateShadow2DArrayLayer(
					u_CSMShadowMap,
					cascade,
					u_CSMViewProj[cascade],
					u_ShadowBias,
					worldPos,
					normal,
					lightDir);

				if (CheckCascadeOverlap(
					viewDepth,
					cascade,
					u_Camera.CameraClip.x,
					u_CSMSplits,
					u_OverlapRatio))
				{
					int nextCascade = cascade + 1;
					float nextVisibility = EvaluateShadow2DArrayLayer(
						u_CSMShadowMap,
						nextCascade,
						u_CSMViewProj[nextCascade],
						u_ShadowBias,
						worldPos,
						normal,
						lightDir);

					float blend = GetCascadeOverlapBlend(
						viewDepth,
						cascade,
						u_Camera.CameraClip.x,
						u_CSMSplits,
						u_OverlapRatio);
					sceneVisibility = mix(sceneVisibility, nextVisibility, blend);
				}
			}
		}
		else
		{
			sceneVisibility = EvaluateShadow2D(
				u_ShadowMap,
				u_ShadowViewProj,
				u_ShadowBias,
				worldPos,
				normal,
				lightDir);
		}
	}

	return sceneVisibility;
}

float CalculateCharacterVisibility(
	vec3 worldPos,
	vec3 normal,
	vec3 lightDir,
	int lightType)
{
	// The character-only map currently supports directional and spot lights.
	if (u_CharacterShadowEnabled == 0 || lightType == 1)
		return 1.0;

	return EvaluateShadow2D(
		u_CharacterShadowMap,
		u_CharacterShadowViewProj,
		u_CharacterShadowBias,
		worldPos,
		normal,
		lightDir);
}
