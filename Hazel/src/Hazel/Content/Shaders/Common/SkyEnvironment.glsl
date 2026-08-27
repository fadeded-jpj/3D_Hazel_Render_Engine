#ifndef CAMERA
#include "CameraUniform.glsl"
#endif

uniform samplerCube u_SkyTexture;
uniform int u_SkyTextureEnabled;

vec3 SampleSkyBox(vec2 uv)
{
	if (u_SkyTextureEnabled == 0)
		return vec3(0.0);

	vec2 ndc = uv * 2.0 - 1.0;

	vec4 worldPos = u_Camera.InverseViewProj * vec4(ndc, 1.0, 1.0);

	worldPos /= worldPos.w;

	vec3 direction = normalize(worldPos.xyz - u_Camera.CameraPositionAndTime.xyz);

	return texture(u_SkyTexture, direction).rgb;
}