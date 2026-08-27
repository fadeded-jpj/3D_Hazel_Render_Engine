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


out vec2 v_TexCoord;
out vec3 v_WorldPos;

uniform mat4 u_ViewProj;
uniform mat4 u_Model;
uniform int u_HasSkeleton;


void main()
{
	vec4 localPos = vec4(a_Pos, 1.0);

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
		}
	}

	v_TexCoord = a_TexCoord;
	vec4 worldPosition = u_Model * localPos;
	v_WorldPos = worldPosition.xyz;
	gl_Position = u_ViewProj * worldPosition;
}

#type fragment
#version 460

in vec2 v_TexCoord;
in vec3 v_WorldPos;

uniform float u_AlphaCutoff;
uniform int u_AlphaMode;

uniform sampler2D u_BaseColorTexture;
uniform vec4 u_BaseColorFactor;
uniform int u_HasBaseColorTexture;


uniform vec3 u_LightPosition;
uniform float u_FarPlane;


void main()
{
	float alpha = u_BaseColorFactor.a;

	if (u_HasBaseColorTexture == 1)
	{
		alpha *= texture(u_BaseColorTexture, v_TexCoord).a;
	}
	if (u_AlphaMode == 1 && alpha < u_AlphaCutoff)
		discard;

	float radialDepth = length(v_WorldPos - u_LightPosition) / u_FarPlane;

	gl_FragDepth = clamp(radialDepth, 0.0, 1.0);
}