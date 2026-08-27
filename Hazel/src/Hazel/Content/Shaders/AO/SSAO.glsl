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
layout(location = 0) out float o_Color;

in vec2 v_TexCoord;

#include "../Common/CameraUniform.glsl"

//m_PerPassParameters.SetFloat("u_DepthBias", 0.02f);
//m_PerPassParameters.SetFloat("u_Radius", 0.5f);
//m_PerPassParameters.SetFloat("u_NearClip", view.NearClip);
//m_PerPassParameters.SetFloat("u_FarClip", view.FarClip);
//m_PerPassParameters.SetFloat2("u_ViewportSize", view.ViewportSize);
//m_PerPassParameters.SetTexture("u_Noraml", input->GetNormalRoughness());
//m_PerPassParameters.SetTexture("u_Depth", input->GetDepth());

uniform float u_DepthBias;
uniform float u_Radius;
uniform float u_Intensity;
uniform sampler2D u_Normal;	// RGB 有效
uniform sampler2D u_Depth;
uniform int u_Enabled;

// ------------ 采样坐标 --------------------
const int SAMPLE_COUNT = 16;
const vec3 SSAOKernel[SAMPLE_COUNT] = vec3[](
	vec3(0.12, 0.08, 0.04),
	vec3(-0.09, 0.15, 0.07),
	vec3(0.03, -0.18, 0.11),
	vec3(0.21, 0.05, 0.14),

	vec3(-0.17, -0.12, 0.19),
	vec3(0.08, 0.27, 0.22),
	vec3(-0.26, 0.06, 0.25),
	vec3(0.18, -0.25, 0.29),

	vec3(0.33, 0.12, 0.35),
	vec3(-0.31, 0.22, 0.39),
	vec3(0.15, -0.38, 0.43),
	vec3(-0.40, -0.13, 0.47),

	vec3(0.42, 0.28, 0.52),
	vec3(-0.25, 0.46, 0.58),
	vec3(0.36, -0.44, 0.65),
	vec3(-0.48, -0.32, 0.72)
	);

// ----------------------------------------

float Hash(vec2 p)
{
	return fract(
		sin(dot(p, vec2(12.9898, 78.233)))
		* 43758.5453
	);
}


vec3 RandomVector(vec2 pixel)
{
	float x = Hash(pixel);
	float y = Hash(pixel + vec2(17.0, 59.0));
	float z = Hash(pixel + vec2(83.0, 23.0));

	return normalize(vec3(
		x * 2.0 - 1.0,
		y * 2.0 - 1.0,
		z * 2.0 - 1.0
	));
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
	if (u_Enabled == 0)
	{
		o_Color = 1.0;
		return;
	}

	float depth = texture(u_Depth, v_TexCoord).r;
	// 天空盒就不考虑了
	if (depth >= 1.0)
	{
		o_Color = 1.0;
		return;
	}


	vec4 ndcPos = vec4(v_TexCoord.xy * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	vec4 clip = u_Camera.InverseProjection * ndcPos;
	vec3 fragPos = clip.xyz / clip.w;

	vec3 randomVec = RandomVector(gl_FragCoord.xy);
	vec3 worldNormal = normalize(texture(u_Normal, v_TexCoord).xyz);
	vec3 N = normalize(mat3(u_Camera.View) * worldNormal);	// view space
	vec3 T = normalize(randomVec - N * dot(N, randomVec));
	vec3 B = normalize(cross(T, N));
	mat3 TBN = mat3(T, B, N);

	float occlusion = 0.0f;

	for (int i = 0; i < SAMPLE_COUNT; i++)
	{
		vec3 offset = TBN * SSAOKernel[i];
		vec3 samplePos = fragPos + u_Radius * offset;
		vec4 sampleClip = u_Camera.Projection * vec4(samplePos, 1.0);

		float sampleDepth = samplePos.z;
		vec2 sampleUV = sampleClip.xy / sampleClip.w * 0.5 + 0.5;

		// 越界的默认不遮挡
		if (sampleClip.w <= 0.0 ||
			sampleUV.x < 0.0 || sampleUV.x > 1.0 ||
			sampleUV.y < 0.0 || sampleUV.y > 1.0)
		{
			continue;
		}

		float depthTarget = texture(u_Depth, sampleUV).r;
		depthTarget = -LinearizeDepth(depthTarget);
		
		// 距离权重， 过远的就剔除
		float rangeWeight = smoothstep(0.0, 1.0,
			u_Radius / max(abs(fragPos.z - depthTarget), 0.0001));
		float occluded = samplePos.z + u_DepthBias <= depthTarget ? 1.0 : 0.0;

		occlusion += rangeWeight * occluded;
	}

	float res = 1.0 - u_Intensity * occlusion / float(SAMPLE_COUNT);

	o_Color = res;
}
