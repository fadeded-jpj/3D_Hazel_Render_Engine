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
#version 460
layout(location = 0) out vec4 o_Color;

in vec2 v_TexCoord;

#include "Common/CameraUniform.glsl"

uniform vec3 u_AmbientColor;

uniform sampler2D u_GBufferNormalRoughness;
uniform sampler2D u_GBufferDepth;
uniform float u_DepthThreshold;
uniform float u_NormalThreshold;
uniform vec3 u_OutlineColor;
uniform float u_OutlineAlpha;

uniform int u_DebugView;


vec3 ReconstructWorldPosition(vec2 uv, float depth)
{
	vec2 ndcXY = uv * 2.0 - 1.0;
	vec4 clip = vec4(ndcXY, depth * 2 - 1.0, 1.0);	// xyz 都要是[-1, 1]的范围内

	vec4 world = u_Camera.InverseViewProj * clip;
	return world.xyz / world.w;
}

float LinearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0;
	return (2.0 * u_Camera.CameraClip.x * u_Camera.CameraClip.y) /
		(u_Camera.CameraClip.y + u_Camera.CameraClip.x -
			z * (u_Camera.CameraClip.y - u_Camera.CameraClip.x));
}

void main()
{
	ivec2 pixel = ivec2(gl_FragCoord.xy);

	float currentDepth = LinearizeDepth(
		texelFetch(u_GBufferDepth, pixel, 0).r
	);

	vec3 currentNormal = normalize(
		texelFetch(u_GBufferNormalRoughness, pixel, 0).xyz * 2.0 - 1.0);

	const ivec2 offsets[4] = ivec2[](
		ivec2(-1, 0), ivec2(1, 0),
		ivec2(0, -1), ivec2(0, 1));

	float maxDepthDiff = 0.0f;
	float maxNormalDiff = 0.0f;

	for (int i = 0; i < 4; i++)
	{
		ivec2 samplePixel = pixel + offsets[i];

		float sampleDepth = LinearizeDepth(
			texelFetch(u_GBufferDepth, samplePixel, 0).r
		);

		vec3 sampleNormal = normalize(
			texelFetch(u_GBufferNormalRoughness, samplePixel, 0).xyz * 2.0 - 1.0);

		maxDepthDiff = max(maxDepthDiff, abs(currentDepth - sampleDepth));
		maxNormalDiff = max(maxNormalDiff,
			1.0 - dot(currentNormal, sampleNormal));
	}

	float normalEdge = step(u_NormalThreshold, maxNormalDiff);
	float depthEdge = step(
		max(u_DepthThreshold, currentDepth * 0.01),
		maxDepthDiff);

	//float edge = max(depthEdge, normalEdge);
	float edge = normalEdge;

	o_Color = vec4(u_OutlineColor, edge * u_OutlineAlpha);
}
