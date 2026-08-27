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

uniform int u_Enabled;
uniform sampler2D u_SSAOColor;
uniform sampler2D u_Depth;
uniform sampler2D u_Normal;
uniform vec2 u_BlurDirection;
float LinearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0;
	return (2.0 * u_Camera.CameraClip.x * u_Camera.CameraClip.y) /
		(u_Camera.CameraClip.y + u_Camera.CameraClip.x -
			z * (u_Camera.CameraClip.y - u_Camera.CameraClip.x));
}

const float GaussianWeights[7] = float[](
	0.006,
	0.061,
	0.242,
	0.382,
	0.242,
	0.061,
	0.006
	);

float GetGaussianWeight(int x, int y)
{
	return GaussianWeights[x + 3] * GaussianWeights[y + 3];
}

float GetViewSpaceDepth(vec2 uv, float depth)
{
	return LinearizeDepth(depth);
}

float GetDepthWeight(float d1, float d2)
{
	// return 1.0;
	float diff = abs(d1 - d2);
	
	return exp(-diff * 10.0);
}

float GetNormalWeight(vec3 n1, vec3 n2)
{
	// return 1.0;
	float cosine = max(dot(n1, n2), 0.0);

	return pow(cosine, 8);
}

void main()
{
	if (u_Enabled == 0)
	{
		o_Color = 1.0;
		return;
	}

	ivec2 textureSizeValue = textureSize(u_SSAOColor, 0);
	vec2 texelSize = 1.0 / textureSizeValue;

	float currentDepth = texture(u_Depth, v_TexCoord).r;
	if (currentDepth >= 1.0)
	{
		o_Color = 1.0;
		return;
	}
	
	float centerViewZ = GetViewSpaceDepth(v_TexCoord, currentDepth);

	vec3 centerNormal = normalize(texture(u_Normal, v_TexCoord).xyz);

	float aoSum = 0.0;
	float weightSum = 0.0f;

	//for (int y = -3; y <= 3; y++)
	//{


		for (int x = -3; x <= 3; x++)
		{
			// float gaussianWeight = GetGaussianWeight(x, y);
			float gaussianWeight = GaussianWeights[x + 3];

			vec2 sampleUV = clamp(v_TexCoord + texelSize * x * u_BlurDirection,
				vec2(0.0), vec2(1.0));
			//vec2 sampleUV = clamp(v_TexCoord + texelSize * ivec2(x, y),
			//	vec2(0.0), vec2(1.0));

			float sampleDepth = texture(u_Depth, sampleUV).r;
			float sampleViewZ = GetViewSpaceDepth(sampleUV, sampleDepth);

			vec3 sampleNormal = normalize(texture(u_Normal, sampleUV).xyz);

			float depthWeight = GetDepthWeight(centerViewZ, sampleViewZ);
			float normalWeight = GetNormalWeight(centerNormal, sampleNormal);

			float Weight = gaussianWeight * normalWeight * depthWeight;

			aoSum += texture(u_SSAOColor, sampleUV).r * Weight;
			weightSum += Weight;
		}
	//}

	o_Color = aoSum / weightSum;
}
