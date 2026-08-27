uniform sampler2D u_SphereMap;
uniform int u_SphereMode;

vec2 CaculateSpheraUV(vec3 worldNormal)
{
	vec3 viewNormal = normalize(mat3(u_Camera.View) * worldNormal);
	vec2 uv = viewNormal.xy * 0.5 + 0.5;

	uv.y = 1.0 - uv.y;

	return clamp(uv, vec2(0.0f), vec2(1.0));
}

vec3 SampleSphereLighting(vec3 worldNormal)
{
	if (u_SphereMode != 2)
		return vec3(0.0);

	vec2 uv = CaculateSpheraUV(worldNormal);

	return texture(u_SphereMap, uv).rgb;
}
