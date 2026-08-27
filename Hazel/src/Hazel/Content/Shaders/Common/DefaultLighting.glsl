#include "LightSource.glsl"
#include "ShadowSource.glsl"

float GetRangeAttenuation(float distanceToLight, float range)
{
	if (range <= 0.0 || distanceToLight >= range)
		return 0.0;

	float x = distanceToLight / range;

	// 平滑衰减，并在 Range 边界归零。
	float attenuation = clamp(1.0 - x * x * x * x, 0.0, 1.0);
	return attenuation * attenuation;
}

// directional light
vec3 GetDirectionalShading(vec3 baseColor, vec3 normal, vec3 lightDir, vec3 viewDir, vec3 specularColor)
{
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	vec3 diffuse = max(dot(normal, lightDir), 0.0) * baseColor;
	vec3 specular = pow(max(dot(normal, halfwayDir), 0.0), 64.0) * specularColor;
	return diffuse + specular;
}

// point light
vec3 GetPointLightShading(vec3 lightPos, vec3 position, vec3 normal, vec3 viewDir,
	vec3 baseColor, vec3 specularColor, float range)
{
	vec3 toLight = lightPos - position;
	float distanceTolight = length(toLight);

	float rangeAttenuation = GetRangeAttenuation(distanceTolight, range);
	if (rangeAttenuation <= 0.0)
		return vec3(0.0);

	vec3 lightDir = normalize(lightPos - position);

	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	vec3 diffuse = max(dot(normal, lightDir), 0.0) * baseColor;
	vec3 specular = pow(max(dot(normal, halfwayDir), 0.0), 64.0) * specularColor;

	return (diffuse + specular) * rangeAttenuation;
}

// spot light
vec3 GetSpotLightShading(vec3 lightPos, vec3 lightForward, float range, float innerCos, float outerCos, vec3 position, vec3 normal, vec3 viewDir, vec3 baseColor, vec3 specularColor)
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
	vec3 diffuse = max(dot(normal, lightDir), 0.0) * baseColor;
	vec3 specular = pow(max(dot(normal, halfDir), 0.0), 64.0) * specularColor;

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
				visibility = CalculateSceneVisibility(
					position, normal, lightDir, LightType, vec3(0.0));
				if (visibility <= 0.0)
					continue;
			}

			outColor += GetDirectionalShading(baseColor, normal,
				lightDir, viewDir, specularColor) * lightColor * visibility;
		}
		else if (LightType == 1)	// point light
		{
			vec3 lightPos = u_Lights[i].PositionRange.xyz;
			float range = u_Lights[i].PositionRange.w;

			if (i == u_ShadowLightIndex)
			{
				vec3 lightDir = normalize(lightPos - position);
				visibility = CalculateSceneVisibility(
					position, normal, lightDir, LightType, lightPos);
				if (visibility <= 0.0)
					continue;
			}

			outColor += GetPointLightShading(lightPos, position, normal, viewDir,
				baseColor, specularColor, range) * lightColor * visibility;
		}
		else if (LightType == 2)	// Spot light
		{
			vec3 lightPos = u_Lights[i].PositionRange.xyz;
			vec3 lightDir = normalize(lightPos - position);
			float range = u_Lights[i].PositionRange.w;

			vec3 lightForward = normalize(u_Lights[i].DirectionType.xyz);

			if (i == u_ShadowLightIndex)
			{
				visibility = CalculateSceneVisibility(
					position, normal, lightDir, LightType, lightPos);
				if (visibility <= 0.0)
					continue;
			}

			float innerCos = u_Lights[i].SpotAngles.x;
			float outerCos = u_Lights[i].SpotAngles.y;

			outColor += GetSpotLightShading(
				lightPos, lightForward, range, innerCos, outerCos,
				position, normal, viewDir, baseColor, specularColor
			) * lightColor * visibility;

		}
	}
	return outColor;
}
