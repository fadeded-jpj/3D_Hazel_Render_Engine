
#type vertex
#version 460 core
layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Tangent;
layout(location = 3) in vec2 a_TexCoord;
layout(location = 4) in ivec4 a_BoneIDs;
layout(location = 5) in vec4 a_BoneWeights;

layout(std430, binding = 0) readonly buffer BoneBuffer
{
	mat4 u_BoneMatrices[];
};

#include "Common/CameraUniform.glsl"


out vec3 v_Normal;
out vec2 v_TexCoord;
out vec4 v_WorldPos;
out vec4 v_Tangent;

uniform mat4 u_Model;
uniform int u_HasSkeleton;

uniform float u_EdgeSize;

void main()
{
	vec4 localPos = vec4(a_Pos, 1.0);
	vec3 localNormal = a_Normal;
	vec3 localTangent = a_Tangent.xyz;

	if (u_HasSkeleton == 1)
	{
		mat4 skinMat = mat4(0.0);
		float totalWeight = 0.0;
		for (int i = 0; i < 4; i++)
		{
			int boneID = a_BoneIDs[i];
			float weight = a_BoneWeights[i];

			if (boneID >= 0 && weight > 0.0 && boneID < 2048)
			{
				skinMat += u_BoneMatrices[boneID] * weight;
				totalWeight += weight;
			}
		}

		if (totalWeight > 0.0)
		{
			localPos = skinMat * vec4(a_Pos, 1.0);
			localNormal = mat3(skinMat) * localNormal;
			localTangent = mat3(skinMat) * localTangent;
		}
	}
	mat3 modelMatrix3 = mat3(u_Model);
	mat3 normalMatrix = transpose(inverse(modelMatrix3));

	vec3 N = normalize(normalMatrix * localNormal);
	vec3 T = normalize(modelMatrix3 * localTangent);
	T = normalize(T - N * dot(N, T));

	v_TexCoord = a_TexCoord;
	v_Tangent = vec4(T, a_Tangent.w);
	
	v_WorldPos = u_Model * localPos;
	v_Normal = N;

	vec4 clipPos = u_Camera.ViewProj * v_WorldPos;

	// 原计划投射的顶点在屏幕空间的位置
	vec4 normalClipPos = u_Camera.ViewProj * vec4(v_WorldPos.xyz + v_Normal, 1.0);

	// normal在 NDC空间中的， xOy 屏幕的投影
	vec2 screenNormal = normalClipPos.xy / normalClipPos.w - clipPos.xy / clipPos.w;



	float lengthSq = dot(screenNormal, screenNormal);
	if (lengthSq > 1e-6)
	{
		vec2 direction = normalize(
			screenNormal * 0.5 * u_Camera.ViewportSizeAndJittered.xy);
		vec2 ndcOffset = direction * 2.0 * u_EdgeSize *
			(2.0 / u_Camera.ViewportSizeAndJittered.xy);

		// 透视除法是后面 管线自己做的， 这里的提前乘回去
		clipPos.xy += ndcOffset * clipPos.w;
	}
	

	gl_Position = clipPos;
}

#type fragment
#version 460
layout(location = 0) out vec4 o_Color;

in vec3 v_Normal;
in vec2 v_TexCoord;
in vec4 v_WorldPos;
in vec4 v_Tangent;

uniform vec4 u_EdgeColor;

void main()
{
	o_Color = u_EdgeColor;
}
