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
out vec4 v_CurrentClip;
out vec4 v_PreviousClip;

uniform mat4 u_Model;
//uniform mat4 u_NormalMatrix;
uniform int u_HasSkeleton;

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
	// mat3 normalMatrix = mat3(u_NormalMatrix);

	vec3 N = normalize(normalMatrix * localNormal);
	vec3 T = normalize(modelMatrix3 * localTangent);
	T = normalize(T - N * dot(N, T));

	v_TexCoord = a_TexCoord;
	v_WorldPos = u_Model * localPos;
	v_Normal = N;
	v_Tangent = vec4(T, a_Tangent.w);
	v_CurrentClip = u_Camera.StabledViewProj * v_WorldPos;
	v_PreviousClip = u_Camera.PreViewProj * v_WorldPos;

	gl_Position = u_Camera.ViewProj * v_WorldPos;
}

#type fragment
#version 460
layout(location = 0) out vec4 o_AlbedoAlpha;
layout(location = 1) out vec4 o_NormalRoughness;
layout(location = 2) out vec4 o_EmissiveMetallic;
//layout(location = 3) out vec4 o_ToonParaments;
//layout(location = 4) out vec4 o_RimParaments;	// u_RimColor, u_RimPower;
layout(location = 5) out vec2 o_Velocity;	// velocity (x, y)

in vec3 v_Normal;
in vec2 v_TexCoord;
in vec4 v_WorldPos;
in vec4 v_Tangent;
in vec4 v_CurrentClip;
in vec4 v_PreviousClip;

uniform float u_AlphaCutoff;
uniform int u_AlphaMode;

uniform sampler2D u_BaseColorTexture;
uniform sampler2D u_OpacityTexture;
uniform sampler2D u_SpecularTexture;
uniform sampler2D u_EmissiveTexture;
uniform sampler2D u_NormalTexture;
uniform sampler2D u_MetallicRoughnessTexture;

uniform vec4 u_BaseColorFactor;
uniform float u_Opacity;
uniform vec3 u_EmissiveFactor;
uniform float u_EmissiveStrength;
uniform float u_MetallicFactor;
uniform float u_RoughnessFactor;
uniform float u_SSRStrength;

uniform int u_HasBaseColorTexture;
uniform int u_HasOpacityTexture;
uniform int u_HasSpecularTexture;
uniform int u_HasEmissiveTexture;
uniform int u_HasNormalTexture;
uniform int u_HasMetallicRoughnessTexture;
uniform int u_Selected;

uniform vec3 u_AmbientColor;

#include "Common/ToonSource.glsl"
#include "Anti-aliasing/TAAHelper.glsl";

uniform vec3 u_RimColor;
uniform float u_RimIntensity;
uniform float u_RimPower;
uniform float u_RimLightMask;

void main()
{
	vec4 surface = u_BaseColorFactor;
	if (u_HasBaseColorTexture == 1)
		surface *= texture(u_BaseColorTexture, v_TexCoord);

	// AlphaCutout 必须在 GBuffer 阶段丢弃，与 Forward 行为一致。
	if (u_AlphaMode == 1 && surface.a < u_AlphaCutoff)
		discard;

	vec3 normal = normalize(v_Normal);
	// 临时处理 模型的地板、墙壁的法线反向问题
	if (!gl_FrontFacing)
		normal = -normal;
	if (u_HasNormalTexture == 1)
	{
		vec3 T = normalize(v_Tangent.xyz);
		T = normalize(T - normal * dot(normal, T));
		vec3 B = normalize(cross(normal, T)) * v_Tangent.w;

		vec3 tangentNormal = texture(u_NormalTexture, v_TexCoord).xyz * 2.0 - 1.0;
		normal = normalize(mat3(T, B, normal) * tangentNormal);
	}

	float metallic = u_MetallicFactor;
	float roughness = u_RoughnessFactor;
	if (u_HasMetallicRoughnessTexture == 1)
	{
		vec4 metallicRoughness = texture(u_MetallicRoughnessTexture, v_TexCoord);

		// 这里先确定引擎约定：glTF/PBR 使用 G=roughness，B=metallic。
		roughness *= metallicRoughness.g;
		metallic *= metallicRoughness.b;
	}

	vec3 emissive = u_EmissiveFactor * u_EmissiveStrength;
	if (u_HasEmissiveTexture == 1)
		emissive *= texture(u_EmissiveTexture, v_TexCoord).rgb;

	// 保持在线性空间；LightingPass 最后才做 tone mapping / gamma。
	o_AlbedoAlpha = vec4(surface.rgb, clamp(u_SSRStrength, 0.0, 1.0));


	o_NormalRoughness = vec4(normal, roughness);
	o_EmissiveMetallic = vec4(emissive, metallic);
	//o_ToonParaments = vec4(u_Threshold, u_Softness, u_ToonLitLevel, u_ToonShadowLevel);
	//o_RimParaments = vec4(u_RimColor * u_RimIntensity, u_RimPower);
	o_Velocity = GetVelocity(v_CurrentClip, v_PreviousClip);
}
