
vec2 GetUV(vec4 worldPos, mat4 VP)
{
	vec4 clip = VP * worldPos;

	if (clip.w <= 0.0)
		return vec2(2.0);

	vec2 uv = (clip.xy / clip.w) * 0.5 + 0.5;

	return uv;
}

bool CheckUV(vec2 uv)
{
	return (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0);
}

vec2 GetVelocity(vec4 worldPos, mat4 curVP, mat4 preVP)
{
	vec2 curUV = GetUV(worldPos, curVP);
	vec2 preUV = GetUV(worldPos, preVP);

	//if (CheckUV(curUV) || CheckUV(preUV))
	//	return vec2(0.0);

	return vec2(curUV - preUV);
}

vec2 GetVelocity(vec4 curClip, vec4 preClip)
{
	if (curClip.w <= 0.0 || preClip.w <= 0)
		return vec2(2.0);

	vec2 curUV = curClip.xy / curClip.w * 0.5 + 0.5;
	vec2 preUV = preClip.xy / preClip.w * 0.5 + 0.5;

	//if (CheckUV(curUV) || CheckUV(preUV))
	//	return vec2(0.0);

	return vec2(curUV - preUV);
}