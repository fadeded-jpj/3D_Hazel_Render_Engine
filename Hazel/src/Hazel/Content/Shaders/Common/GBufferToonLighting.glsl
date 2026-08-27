#include "LightSource.glsl"
#include "ShadowSource.glsl"

float CalculateToonSceneVisibility(
	vec3 worldPos,
	vec3 normal,
	vec3 lightDir,
	int lightType,
	vec3 lightPosition)
{
	float sceneVisibility = CalculateSceneVisibility(
		worldPos, normal, lightDir, lightType, lightPosition);
	float characterVisibility = CalculateCharacterVisibility(
		worldPos, normal, lightDir, lightType);
	return min(sceneVisibility, characterVisibility);
}
#include "ToonSource.glsl"

uniform sampler2D u_GBToonParamters;

// 注意： 这串代码刚需 vec2 v_TexCoord,
// 请放在 vec2 v_TexCoord; 后面

struct ToonParameters
{
	bool  Enabled;
	float Threshold;
	float Softness;
	float LitLevel;
	float ShadowLevel;
};

ToonParameters ReadToonParameters(vec2 uv)
{
	vec4 data = texture(u_GBToonParamters, uv);

	ToonParameters parameters;
	parameters.Threshold = data.r;
	parameters.Softness = data.g;
	parameters.LitLevel = data.b;
	parameters.ShadowLevel = data.a;
	return parameters;
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

float ToonDiffuse(vec3 normal, vec3 lightDir, ToonParameters toon)
{
	float NdotL = max(dot(normal, lightDir), 0.0);

	return smoothstep(toon.Threshold - toon.Softness, toon.Threshold + toon.Softness, NdotL);
}

// directional light
vec3 GetDirectionalShading(vec3 baseColor, vec3 normal, vec3 lightDir,
	vec3 viewDir, vec3 specularColor, float visibility, ToonParameters toon)
{
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	vec3 diffuse = baseColor * max(dot(normal, lightDir), 0.0) * visibility;
	if (toon.Enabled)
	{
		vec3 lightLevel = vec3(toon.LitLevel);
		vec3 darkLevel = u_ToonShadowTint * toon.ShadowLevel;

		float band = ToonDiffuse(normal, lightDir, toon) * visibility;
		diffuse = baseColor * mix(darkLevel, lightLevel, band);
	}

	vec3 specular = pow(max(dot(normal, halfwayDir), 0.0), 64.0) *
		specularColor * visibility;
	return (diffuse + specular);
}

// point light
vec3 GetPointLightShading(vec3 lightPos, vec3 position, vec3 normal, vec3 viewDir,
	vec3 baseColor, vec3 specularColor, float range, float visibility, ToonParameters toon)
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
	if (toon.Enabled)
	{
		vec3 lightLevel = vec3(toon.LitLevel);
		vec3 darkLevel = u_ToonShadowTint * toon.ShadowLevel;

		float band = ToonDiffuse(normal, lightDir, toon) * visibility;
		diffuse = baseColor * mix(darkLevel, lightLevel, band);
	}

	vec3 specular = pow(max(dot(normal, halfwayDir), 0.0), 64.0) *
		specularColor * visibility;

	return (diffuse + specular) * rangeAttenuation;
}

// spot light
vec3 GetSpotLightShading(vec3 lightPos, vec3 lightForward, float range,
	float innerCos, float outerCos, vec3 position, vec3 normal, vec3 viewDir,
	vec3 baseColor, vec3 specularColor, float visibility, ToonParameters toon)
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
	if (toon.Enabled)
	{
		vec3 lightLevel = vec3(toon.LitLevel);
		vec3 darkLevel = u_ToonShadowTint * toon.ShadowLevel;

		float band = ToonDiffuse(normal, lightDir, toon) * visibility;
		diffuse = baseColor * mix(darkLevel, lightLevel, band);
	}

	vec3 specular = pow(max(dot(normal, halfDir), 0.0), 64.0) *
		specularColor * visibility;

	return (diffuse + specular) * rangeAttenuation * spotAttenuation;
}


vec3 CacLighting(vec3 baseColor, vec3 specularColor, vec3 normal, vec3 viewDir, vec3 position)
{
	vec3 outColor = vec3(0.0);
	ToonParameters toon = ReadToonParameters(v_TexCoord);

	for (int i = 0; i < u_LightCount; i++)
	{
		toon.Enabled = i == u_ToonMainLightIndex;
		int LightType = int(u_Lights[i].DirectionType.w);
		vec3 lightColor = u_Lights[i].ColorIntensity.w * u_Lights[i].ColorIntensity.xyz;
		float visibility = 1.0;

		if (LightType == 0)			// Directional light
		{
			vec3 lightDir = normalize(-u_Lights[i].DirectionType.xyz);

			if (i == u_ShadowLightIndex)
			{
				visibility = CalculateToonSceneVisibility(
					position, normal, lightDir, LightType, vec3(0.0));
			}

			outColor += GetDirectionalShading(baseColor, normal,
				lightDir, viewDir, specularColor, visibility, toon) * lightColor;
		}
		else if (LightType == 1)	// point light
		{
			vec3 lightPos = u_Lights[i].PositionRange.xyz;
			float range = u_Lights[i].PositionRange.w;

			if (i == u_ShadowLightIndex)
			{
				vec3 lightDir = normalize(lightPos - position);
				visibility = CalculateToonSceneVisibility(
					position, normal, lightDir, LightType, lightPos);
			}

			outColor += GetPointLightShading(lightPos, position, normal, viewDir,
				baseColor, specularColor, range, visibility, toon) * lightColor;
		}
		else if (LightType == 2)	// Spot light
		{
			vec3 lightPos = u_Lights[i].PositionRange.xyz;
			vec3 lightDir = normalize(lightPos - position);
			float range = u_Lights[i].PositionRange.w;

			vec3 lightForward = normalize(u_Lights[i].DirectionType.xyz);

			if (i == u_ShadowLightIndex)
			{
				visibility = CalculateToonSceneVisibility(
					position, normal, lightDir, LightType, lightPos);
			}

			float innerCos = u_Lights[i].SpotAngles.x;
			float outerCos = u_Lights[i].SpotAngles.y;

			outColor += GetSpotLightShading(
				lightPos, lightForward, range, innerCos, outerCos,
				position, normal, viewDir, baseColor, specularColor, visibility,
				toon
			) * lightColor;

		}
	}
	return outColor;
}
