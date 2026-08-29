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

layout(std430, binding = 1) readonly buffer PreviousBoneBuffer
{
	mat4 u_PreviousBoneMatrices[];
};

#include "../Common/CameraUniform.glsl"

out vec3 v_Normal;
out vec2 v_TexCoord;
out vec4 v_WorldPos;
out vec4 v_Tangent;
out vec4 v_CurrentClip;
out vec4 v_PreviousClip;

uniform mat4 u_Model;
uniform int u_HasSkeleton;

void main()
{
	vec4 localPos = vec4(a_Pos, 1.0);
	vec3 localNormal = a_Normal;
	vec3 localTangent = a_Tangent.xyz;

	vec4 preLocalPos = localPos;

	if (u_HasSkeleton == 1)
	{
		mat4 skinMat = mat4(0.0);
		mat4 prevSkinMat = mat4(0.0);
		float totalWeight = 0.0;
		for (int i = 0; i < 4; i++)
		{
			int boneID = a_BoneIDs[i];
			float weight = a_BoneWeights[i];

			if (boneID >= 0 && weight > 0.0 && boneID < 2048)
			{
				skinMat += u_BoneMatrices[boneID] * weight;
				prevSkinMat += u_PreviousBoneMatrices[boneID] * weight;
				totalWeight += weight;
			}
		}

		if (totalWeight > 0.0)
		{
			localPos = skinMat * vec4(a_Pos, 1.0);
			preLocalPos = prevSkinMat * vec4(a_Pos, 1.0);
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
	v_WorldPos = u_Model * localPos;
	v_Normal = N;
	v_Tangent = vec4(T, a_Tangent.w);
	v_CurrentClip = u_Camera.StabledViewProj * v_WorldPos;
	v_PreviousClip = u_Camera.PreViewProj * u_Model * preLocalPos;
	gl_Position = u_Camera.ViewProj * v_WorldPos;
}

#type fragment
#version 460
layout(location = 0) out vec4 o_Color;

layout(location = 2) out vec2 o_Velocity;

in vec3 v_Normal;
in vec2 v_TexCoord;
in vec4 v_WorldPos;
in vec4 v_Tangent;
in vec4 v_CurrentClip;
in vec4 v_PreviousClip;
#include "../Anti-aliasing/TAAHelper.glsl";

uniform float u_AlphaCutoff;
uniform int u_AlphaMode;

uniform sampler2D u_BaseColorTexture;
uniform sampler2D u_OpacityTexture;
uniform sampler2D u_SpecularTexture;
uniform sampler2D u_EmissiveTexture;
uniform sampler2D u_NormalTexture;

uniform vec4 u_BaseColorFactor;
uniform float u_Opacity;
uniform vec3 u_EmissiveFactor;
uniform float u_EmissiveStrength;

uniform int u_HasBaseColorTexture;
uniform int u_HasOpacityTexture;
uniform int u_HasSpecularTexture;
uniform int u_HasEmissiveTexture;
uniform int u_HasNormalTexture;
uniform int u_Selected;

uniform vec3 u_AmbientColor;
uniform int u_DebugView;

uniform vec3 u_RimColor;
uniform float u_RimIntensity;
uniform float u_RimPower;
uniform float u_RimLightMask;

uniform sampler2D u_MetalMask;

#include "../Common/ToonLighting.glsl"
#include "../Common/SphereMap.glsl"
#include "../Common/ToonCharacterOutput.glsl"

vec3 CalculateMetalSpecular(
	vec3 normal,
	vec3 viewDir,
	vec3 lighting, float mask)
{
	vec3 up = vec3(0, 1, 0);
	vec3 normal_H = normalize(normal - up * dot(up, normal));
	vec3 v_H = normalize(viewDir - up * dot(up, viewDir));

	float NdotV = max(dot(normal_H, v_H), 0.0);
	float rawSpecular = pow(NdotV, 32);

	float band = smoothstep(0.9, 0.95, rawSpecular);
	float luminance = dot(lighting, vec3(0.2126, 0.7152, 0.0722));
	float lightingFactor = smoothstep(0.8, 0.9, luminance);
	
	return band * mask * vec3(0.8f) * luminance;
}

void main()
{
	vec4 surface = u_BaseColorFactor;
	float alpha = u_BaseColorFactor.a;

	vec3 N = normalize(v_Normal);
	if (!gl_FrontFacing)
		N = -N;
	vec3 T = normalize(v_Tangent.xyz);
	T = normalize(T - N * dot(N, T));
	vec3 B = normalize(cross(N, T)) * v_Tangent.w;

	mat3 TBN = mat3(T, B, N);

	vec3 normal = N;
	if (u_HasNormalTexture == 1)
	{
		vec3 normalMap = texture(u_NormalTexture, v_TexCoord).rgb;
		normalMap = normalMap * 2.0 - 1.0;
		normal = normalize(TBN * normalMap);
	}

	if (u_HasBaseColorTexture == 1)
	{
		surface *= texture(u_BaseColorTexture, v_TexCoord);
		alpha *= texture(u_BaseColorTexture, v_TexCoord).a;
	}

	if (u_HasOpacityTexture == 1)
		alpha *= texture(u_OpacityTexture, v_TexCoord).r;

	if (u_AlphaMode == 1 && alpha < u_AlphaCutoff)
		discard;

	vec3 specularColor = vec3(0.0);
	if (u_HasSpecularTexture == 1)
		specularColor = texture(u_SpecularTexture, v_TexCoord).rgb;

	vec3 baseColor = surface.rgb;
	vec3 viewDir = normalize(u_Camera.CameraPositionAndTime.xyz - v_WorldPos.xyz);
	vec3 ambient = u_AmbientColor * baseColor;


	vec3 lighting = CacLighting(vec3(1.0), vec3(0.0), normal, viewDir, v_WorldPos.xyz);
	vec3 color = ambient +
		 + lighting * baseColor + 
		SampleSphereLighting(N);

	float metal = texture(u_MetalMask, v_TexCoord).r;
	color += CalculateMetalSpecular(normal, viewDir, lighting, metal);

	if (u_HasEmissiveTexture == 1)
	{
		vec3 emissiveColor =
			u_EmissiveFactor * texture(u_EmissiveTexture, v_TexCoord).rgb;
		color += emissiveColor;
	}

	float fresnel = clamp(1.0 - max(dot(normal, viewDir), 0.0), 0.00004, 1.0);
	float rim = pow(fresnel, u_RimPower);
	vec3 rimColor = u_RimColor * rim * u_RimIntensity;

	vec3 mapped = color + rimColor;

	if (u_Selected == 1)
		mapped += vec3(0.2);

	if (u_DebugView == 1)
		mapped = baseColor;
	else if (u_DebugView == 2)
		mapped = normal * 0.5 + 0.5;
	else if (u_DebugView == 3)
	{
		if (u_HasEmissiveTexture == 1)
			mapped = u_EmissiveFactor *
				texture(u_EmissiveTexture, v_TexCoord).rgb;
		else
			mapped = vec3(0.0);
	}
	else if (u_DebugView == 4)
		mapped = vec3(0.0);

	o_Color = vec4(mapped, alpha);
	WriteCharacterNormalMask(normal);
	o_Velocity = GetVelocity(v_CurrentClip, v_PreviousClip);
}
