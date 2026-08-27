#include "LightSource.glsl"
#include "ShadowSource.glsl"

#include "ToonSource.glsl"

float CalculateToonVisibility(
	vec3 worldPos,
	vec3 normal,
	vec3 lightDir,
	int lightType,
	vec3 lightPosition)
{
	float sceneVisibility = CalculateSceneVisibility(
		worldPos, normal, lightDir, lightType, lightPosition);

#if defined(TOON_FACE_SHADER) || defined(TOON_SCENE_SHADOW_ONLY)
	return sceneVisibility;
#else
	float characterVisibility = CalculateCharacterVisibility(
		worldPos, normal, lightDir, lightType);
	return min(sceneVisibility, characterVisibility);
#endif
}

vec3 SampleMMDToonRamp(float lighting01)
{
	float halfTexel = 0.5 / float(textureSize(u_ToonRampTexture, 0).y);

	// lighting01=1 -> 图片顶部亮色
	// lighting01=0 -> 图片底部阴影色
	float rampV = mix(
		1.0 - halfTexel,
		halfTexel,
		clamp(lighting01, 0.0, 1.0));

	return textureLod(
		u_ToonRampTexture,
		vec2(0.5, rampV),
		0.0).rgb;
}

float GetRangeAttenuation(float distanceToLight, float range)
{
	if (range <= 0.0 || distanceToLight >= range)
		return 0.0;

	float x = distanceToLight / range;

	// 平滑衰减，并在 Range 边界归零。
	float attenuation = clamp(1.0 - x * x * x * x, 0.0, 1.0);
	return attenuation * attenuation;
}

vec3 ToonDiffuse(vec3 normal, vec3 lightDir, float visibility)
{
	float NdotL = dot(normal, lightDir);
	float halfLambert = (NdotL * 0.5 + 0.5);

	if (u_HasToonRampTexture == 1)
	{
		vec3 localLighting = SampleMMDToonRamp(halfLambert);
		vec3 castShadowLighting = SampleMMDToonRamp(0.0);

		return mix(castShadowLighting, localLighting, visibility);
	}

	float band = smoothstep(
		u_Threshold - u_Softness,
		u_Threshold + u_Softness,
		halfLambert);

	vec3 darkLighting =
		u_ToonShadowTint * u_ToonShadowLevel;

	vec3 localLighting = mix(
		darkLighting,
		vec3(u_ToonLitLevel),
		band);

	return mix(
		darkLighting,
		localLighting,
		clamp(visibility, 0.0, 1.0));
}

// directional light
vec3 GetDirectionalShading(vec3 baseColor, vec3 normal, vec3 lightDir, 
	vec3 viewDir, vec3 specularColor, float visibility, bool IsMain)
{
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	vec3 diffuse = baseColor * max(dot(normal, lightDir), 0.0) * visibility;

	if (IsMain)
	{
#ifdef TOON_FACE_SHADER
		diffuse = baseColor * GetFaceDiffuseLighting(lightDir, v_TexCoord);
#else
		vec3 diffuseFactor = ToonDiffuse(normal, lightDir, visibility);
		diffuse = baseColor * diffuseFactor;
#endif
	}

	vec3 specular = pow(max(dot(normal, halfwayDir), 0.0), 64.0) * 
		specularColor * visibility;
	return (diffuse + specular);
}

// point light
vec3 GetPointLightShading(vec3 lightPos, vec3 position, vec3 normal, vec3 viewDir,
	vec3 baseColor, vec3 specularColor, float range, float visibility, bool IsMain)
{
	vec3 toLight = lightPos - position;
	float distanceTolight = length(toLight);

	float rangeAttenuation = GetRangeAttenuation(distanceTolight, range);
	if (rangeAttenuation <= 0.0)
		return vec3(0.0);

	vec3 lightDir = normalize(lightPos - position);

	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	vec3 diffuse = baseColor * max(dot(normal, lightDir), 0.0) * visibility;
	if (IsMain)
	{
		vec3 lightLevel = vec3(u_ToonLitLevel);
		vec3 darkLevel = u_ToonShadowTint * u_ToonShadowLevel;

		vec3 diffuseFactor = ToonDiffuse(normal, lightDir, visibility);
		diffuse = baseColor * diffuseFactor;
	}
	vec3 specular = pow(max(dot(normal, halfwayDir), 0.0), 64.0) *
		specularColor * visibility;

	return (diffuse + specular) * rangeAttenuation;
}

// spot light
vec3 GetSpotLightShading(vec3 lightPos, vec3 lightForward, float range, 
	float innerCos, float outerCos, vec3 position, vec3 normal, vec3 viewDir, 
	vec3 baseColor, vec3 specularColor, float visibility, bool IsMain)
{
	vec3 toLight = lightPos - position;
	float distanceTolight = length(toLight);

	float rangeAttenuation = GetRangeAttenuation(distanceTolight, range);
	if (rangeAttenuation <= 0.0)
		return vec3(0.0);

	vec3 lightDir = toLight / max(distanceTolight, 0.0001);
	float cosine = dot(normalize(lightForward), -lightDir);

	// 判断是否在聚光锥中
	float spotAttenuation = smoothstep(outerCos, innerCos, cosine);
	if (spotAttenuation <= 0)
		return vec3(0.0);

	vec3 halfDir = normalize(lightDir + viewDir);

	vec3 diffuse = baseColor * max(dot(normal, lightDir), 0.0) * visibility;
	if (IsMain)
	{
		vec3 lightLevel = vec3(u_ToonLitLevel);
		vec3 darkLevel = u_ToonShadowTint * u_ToonShadowLevel;

		vec3 diffuseFactor = ToonDiffuse(normal, lightDir, visibility);
		diffuse = baseColor * diffuseFactor;
	}

	vec3 specular = pow(max(dot(normal, halfDir), 0.0), 64.0) *
		specularColor * visibility;

	return (diffuse + specular) * rangeAttenuation * spotAttenuation;
}


vec3 CacLighting(vec3 baseColor, vec3 specularColor, vec3 normal, vec3 viewDir, vec3 position)
{
	vec3 outColor = vec3(0.0);

	for (int i = 0; i < u_LightCount; i++)
	{
		int LightType = int(u_Lights[i].DirectionType.w);
		vec3 lightColor = u_Lights[i].ColorIntensity.w * u_Lights[i].ColorIntensity.xyz;
		float visibility = 1.0;

		if (LightType == 0)			// Directional light
		{
			vec3 lightDir = normalize(-u_Lights[i].DirectionType.xyz);

			if (i == u_ShadowLightIndex)
			{
				visibility = CalculateToonVisibility(
					position, normal, lightDir, LightType, vec3(0.0));
			}

			outColor += GetDirectionalShading(baseColor, normal,
				lightDir, viewDir, specularColor, visibility, i == u_ToonMainLightIndex) * lightColor;
		}
		else if (LightType == 1)	// point light
		{
			vec3 lightPos = u_Lights[i].PositionRange.xyz;
			float range = u_Lights[i].PositionRange.w;

			if (i == u_ShadowLightIndex)
			{
				vec3 lightDir = normalize(lightPos - position);
				visibility = CalculateToonVisibility(
					position, normal, lightDir, LightType, lightPos);
			}

			outColor += GetPointLightShading(lightPos, position, normal, viewDir,
				baseColor, specularColor, range, visibility, i == u_ToonMainLightIndex) * lightColor;
		}
		else if (LightType == 2)	// Spot light
		{
			vec3 lightPos = u_Lights[i].PositionRange.xyz;
			vec3 lightDir = normalize(lightPos - position);
			float range = u_Lights[i].PositionRange.w;

			vec3 lightForward = normalize(u_Lights[i].DirectionType.xyz);

			if (i == u_ShadowLightIndex)
			{
				visibility = CalculateToonVisibility(
					position, normal, lightDir, LightType, lightPos);
			}

			float innerCos = u_Lights[i].SpotAngles.x;
			float outerCos = u_Lights[i].SpotAngles.y;

			outColor += GetSpotLightShading(
				lightPos, lightForward, range, innerCos, outerCos,
				position, normal, viewDir, baseColor, specularColor, visibility, 
				i == u_ToonMainLightIndex
			) * lightColor;

		}
	}
	return outColor;
}
