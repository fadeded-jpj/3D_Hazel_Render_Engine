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

out VS_OUT
{
	vec3 WorldPos;
} vs_out;


out vec3 v_Normal;
out vec2 v_TexCoord;
out vec4 v_WorldPos;
out vec4 v_Tangent;

uniform mat4 u_Model;
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

	vec3 N = normalize(normalMatrix * localNormal);
	vec3 T = normalize(modelMatrix3 * localTangent);
	T = normalize(T - N * dot(N, T));

	v_TexCoord = a_TexCoord;
	v_Tangent = vec4(T, a_Tangent.w);

	v_WorldPos = u_Model * localPos;
	v_Normal = N;

	vs_out.WorldPos = v_WorldPos.xyz;

	gl_Position = u_Camera.ViewProj * v_WorldPos;
}

#type geometry
#version 460 core

#include "Common/CameraUniform.glsl"

layout(lines_adjacency) in;
layout(triangle_strip, max_vertices = 4) out;

in VS_OUT
{
	vec3 WorldPos;
}gs_in[];

flat out vec4 g_EdgeColor;

uniform float u_EdgeSize;

uniform float u_OutlineWidthScale;
uniform float u_OutlineDepthBias;
uniform float u_CreaseCosThreshold;
vec2 NDC2Pixel(vec2 ndc)
{
	return (ndc * 0.5 + 0.5) * u_Camera.ViewportSizeAndJittered.xy;
}

void EmitOutlineVertex(vec4 clipPos, vec2 offsetNDC)
{
	clipPos.xy += offsetNDC * clipPos.w;

	clipPos.z -= u_OutlineDepthBias * clipPos.w;

	gl_Position = clipPos;
	EmitVertex();
}

void EmitWorldVertex(vec3 pos)
{
	vec4 clipPos = u_Camera.ViewProj * vec4(pos, 1.0);
	clipPos.z -= u_OutlineDepthBias * clipPos.w;
	gl_Position = clipPos;
	EmitVertex();
}

void main()
{
	vec3 opposite0 = gs_in[0].WorldPos;
	vec3 edgeStart = gs_in[1].WorldPos;
	vec3 edgeEnd = gs_in[2].WorldPos;
	vec3 opposite1 = gs_in[3].WorldPos;

	vec3 edge = edgeEnd - edgeStart;
	if (dot(edge, edge) < 1e-10)
		return;

	// 两个邻接三角形的法线
	vec3 n0 = normalize(cross(edgeEnd - edgeStart, opposite0 - edgeStart));
	bool boundary = distance(opposite1, edgeStart) < 1e-6;
	vec3 n1 = boundary ? n0 : 
		normalize(cross(edgeStart - edgeEnd, opposite1 - edgeEnd));

	vec3 midpoint = (edgeStart + edgeEnd) * 0.5;
	vec3 viewDir = normalize(u_Camera.CameraPositionAndTime.xyz - midpoint);


	bool front0 = dot(n0, viewDir) > 0.0;
	bool front1 = dot(n1, viewDir) > 0.0;

	// 轮廓边
	bool silhouette = !boundary && (front0 != front1);
	
	// 折缝边
	bool crease = !boundary && front0 && front1 &&
		dot(n0, n1) < u_CreaseCosThreshold;

	bool visibleBounry = boundary && front0;

	if (!silhouette && !crease && !visibleBounry)
		return;

	//bool crease = false;
	//bool visibleBoundary = false;

	//if (!silhouette)
	//	return;

	vec4 edgeColor;
	
	if (silhouette)
		edgeColor = vec4(1, 0, 0, 1);       // 红：轮廓
	else if (crease)
		edgeColor = vec4(0, 1, 0, 1);       // 绿：折痕
	else if (visibleBounry)
		edgeColor = vec4(0, 0, 1, 1);       // 蓝：边界
	else
		return;


	vec3 edgeDirection = normalize(edge);
	vec3 worldSide = cross(edgeDirection, viewDir);
	float worldSideLengthSq = dot(worldSide, worldSide);
	if (worldSideLengthSq < 1e-8)
		return;

	worldSide *= inversesqrt(worldSideLengthSq);

	float lineWidthWorld = max(u_EdgeSize * u_OutlineWidthScale, 0.0);
	if (lineWidthWorld <= 0.0)
		return;

	float halfWidthWorld = 0.5 * lineWidthWorld;

	vec4 startClip = u_Camera.ViewProj * vec4(edgeStart, 1.0);
	vec4 endClip = u_Camera.ViewProj * vec4(edgeEnd, 1.0);

	if (startClip.w <= 0.0 || endClip.w <= 0.0)
		return;


	vec2 startNDC = startClip.xy / startClip.w;
	vec2 endNDC = endClip.xy / endClip.w;

	vec2 startPixel = NDC2Pixel(startNDC);
	vec2 endPixel = NDC2Pixel(endNDC);

	vec2 edgePixelDirection = endPixel - startPixel;

	if (dot(edgePixelDirection, edgePixelDirection) < 1e-6)
		return;

	edgePixelDirection = normalize(edgePixelDirection);

	vec2 sidePixelDirection = vec2(
		-edgePixelDirection.y,
		edgePixelDirection.x);

	vec4 startWorldOffsetClip = u_Camera.ViewProj * vec4(
		edgeStart + worldSide * halfWidthWorld,
		1.0);

	vec4 endWorldOffsetClip = u_Camera.ViewProj * vec4(
		edgeEnd + worldSide * halfWidthWorld,
		1.0);
	
	vec2 startWorldOffsetPixel = NDC2Pixel(
		startWorldOffsetClip.xy /
		startWorldOffsetClip.w);

	vec2 endWorldOffsetPixel = NDC2Pixel(
		endWorldOffsetClip.xy /
		endWorldOffsetClip.w);

	float minHalfWidthPixels = 0.5;

	float startHalfWidthPixels = max(
		length(startWorldOffsetPixel - startPixel),
		minHalfWidthPixels);

	float endHalfWidthPixels = max(
		length(endWorldOffsetPixel - endPixel),
		minHalfWidthPixels);

	vec2 startOffsetNDC =
		sidePixelDirection *
		startHalfWidthPixels *
		2.0 / u_Camera.ViewportSizeAndJittered.xy;

	vec2 endOffsetNDC =
		sidePixelDirection *
		endHalfWidthPixels *
		2.0 / u_Camera.ViewportSizeAndJittered.xy;


	g_EdgeColor = edgeColor;

	EmitOutlineVertex(startClip, startOffsetNDC);
	EmitOutlineVertex(startClip, -startOffsetNDC);
	EmitOutlineVertex(endClip, endOffsetNDC);
	EmitOutlineVertex(endClip, -endOffsetNDC);

	EndPrimitive();
}

#type fragment
#version 460 core

layout(location = 0) out vec4 o_Color;

uniform vec4 u_EdgeColor;

flat in vec4 g_EdgeColor;

void main()
{
	o_Color = u_EdgeColor;
	// o_Color = g_EdgeColor;
}
