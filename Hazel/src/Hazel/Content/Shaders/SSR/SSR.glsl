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

#include "../Common/CameraUniform.glsl"

//m_Parameters.SetTexture("u_ScreenColor", buffer->GetColorAttachment(0));
//m_Parameters.SetTexture("u_Depth", buffer->GetDepthAttachment());
//m_Parameters.SetTexture("u_NormalRoughness", gbuffer->GetNormalRoughness());
//m_Parameters.SetTexture("u_EmissiveMetallic", gbuffer->GetEmissiveMetallic());

uniform sampler2D u_NormalRoughness;	
uniform sampler2D u_EmissiveMetallic;
uniform sampler2D u_ScreenColor;
uniform sampler2D u_AlbedoAlpha;
uniform sampler2D u_Depth;
uniform sampler2D u_MipDepth;
uniform int u_MipCount;
uniform int u_UseHiZ;
uniform sampler2D u_CharacterNormalMask;

const float maxRayDistance = 6.4;
const int maxSteps = 64;
const int binarySearchSteps = 5;

const float thickness = 0.03;
const float normalBias = 0.002;
const float rayBias = 0.02;

const float edgeFadeWidth = 0.08;
const float distanceFadeStart = 0.7;
const float roughnessFadeStart = 0.3;
const float roughnessFadeEnd = 0.7;
const float PI = 3.14159265359;


float LinearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0;
	return (2.0 * u_Camera.CameraClip.x * u_Camera.CameraClip.y) /
		(u_Camera.CameraClip.y + u_Camera.CameraClip.x -
			z * (u_Camera.CameraClip.y - u_Camera.CameraClip.x));
}

bool IsInsideScreen(vec2 uv)
{
	return all(greaterThanEqual(uv, vec2(0.0))) &&
		all(lessThan(uv, vec2(1.0)));
}

vec3 SafeNormalize(vec3 value)
{
	float lengthSquared = dot(value, value);
	return lengthSquared > 0.00000001
		? value * inversesqrt(lengthSquared)
		: vec3(0.0);
}

vec3 GetViewSpacePos(vec2 uv, float samplerDepth)
{
	float ndcZ = samplerDepth * 2.0 - 1.0;
	vec4 clip = u_Camera.InverseProjection *
		vec4(uv * 2.0 - 1.0, ndcZ, 1.0);
	vec3 posVS = clip.xyz / clip.w;

	return posVS;
}

bool ProjectToScreen(vec3 viewPos, out vec2 uv)
{
	vec4 clip = u_Camera.Projection * vec4(viewPos, 1.0);

	if (clip.w <= 0.0)
		return false;

	uv = clip.xy / clip.w * 0.5 + 0.5;
	return all(greaterThanEqual(uv, vec2(0.0))) &&
		all(lessThanEqual(uv, vec2(1.0)));
}

float GetTexelDepth(vec2 uv)
{
	ivec2 size = ivec2(u_Camera.ViewportSizeAndJittered.xy);
	ivec2 pixel = clamp(ivec2(uv * size), ivec2(0), size - ivec2(1));
	return texelFetch(u_Depth, pixel, 0).r;
}

bool SampleSceneDepthVS(vec2 uv, out float viewDepth)
{
	float depth = GetTexelDepth(uv);
	if (depth >= 1.0)
		return false;

	// vec3 viewPos = GetViewSpacePos(uv, depth);
	
	viewDepth = LinearizeDepth(depth);
	return true;
}

bool RefineSSRHit(vec3 left, vec3 right, out vec2 hitUV, out vec3 hitPos)
{
	vec3 mid;

	for (int i = 0; i < binarySearchSteps; i++)
	{
		mid = 0.5 * (left + right);

		vec2 sampleUV;
		if (!ProjectToScreen(mid, sampleUV))
		{
			return false;	// 越界了
		}

		float sceneDepth = -mid.z;
		float curDepth;

		float curViewDepth;
		if (!SampleSceneDepthVS(sampleUV, curViewDepth))
			return false;

		float curDiff = sceneDepth - curViewDepth;

		if (curDiff < 0.0)
			left = mid;
		else
			right = mid;
	}

	if (!ProjectToScreen(right, hitUV))
		return false;

	float finalDepth;
	if (!SampleSceneDepthVS(hitUV, finalDepth))
		return false;
	
	float curDepth = -right.z;
	hitPos = right;
	
	return abs(curDepth - finalDepth) <= thickness;
}

bool traceSSR(vec3 P, vec3 direction, vec3 viewNormal, 
	out vec2 hitUV, out float hitDistance)
{
	vec3 origin = P + viewNormal * normalBias + direction * rayBias;	// 防止自遮挡

	bool hasPreviousSample = false;
	vec3 previousRayPosition;
	float previousDepthDiff = 0.0;

	float stepLength = maxRayDistance / float(maxSteps);
	float maxInitialOvershoot = stepLength * 2.0;

	for (int i = 1; i <= maxSteps; i++)
	{
		float rayDistance = stepLength * float(i);
		vec3 rayPosition = origin + rayDistance * direction;

		vec2 sampleUV;
		if (!ProjectToScreen(rayPosition, sampleUV))
		{
			return false;	// 越界了
		}

		float curViewDepth;
		if (!SampleSceneDepthVS(sampleUV, curViewDepth))
		{
			hasPreviousSample = false;
			continue;
		}

		float sceneDepth = -rayPosition.z;
		float depthDiff = sceneDepth - curViewDepth;

		if (hasPreviousSample && previousDepthDiff < 0.0 && depthDiff >= 0.0)
		{
			// 说明这段区间内有交点，二分获取更精细的位置
			vec2 refinedUV;
			vec3 hitPos;
			if (RefineSSRHit(previousRayPosition, rayPosition, refinedUV, hitPos))
			{
				hitUV = refinedUV;
				hitDistance = length(hitPos - origin);;
				return true;
			}

			hasPreviousSample = false;
			continue;
		}

		previousRayPosition = rayPosition;
		previousDepthDiff = depthDiff;
		hasPreviousSample = true;
	}

	return false;
}


bool Project2TextureSpace(vec3 pos, out vec3 positionTS)
{
	vec4 clip = u_Camera.Projection * vec4(pos, 1.0);
	if (clip.w <= 0.0)
		return false;

	vec3 ndc = clip.xyz / clip.w;
	positionTS = ndc * 0.5 + 0.5;

	return all(greaterThanEqual(positionTS.xy, vec2(0.0))) &&
		all(lessThan(positionTS.xy, vec2(1.0)));
}

vec3 TS2VS(vec3 posTS)
{
	vec3 ndc = posTS * 2.0 - 1.0;

	vec4 pos = u_Camera.InverseProjection * vec4(ndc, 1.0);

	return pos.xyz / pos.w;
}

ivec2 GetHizCell(vec2 uv, int level)
{
	ivec2 size = textureSize(u_MipDepth, level);
	ivec2 pixel = ivec2(floor(uv * vec2(size)));
	return clamp(pixel, ivec2(0), size - ivec2(1));
}

float SampleHiZDepth(ivec2 pixel, int mipLevel)
{
	return texelFetch(u_MipDepth, pixel, mipLevel).r;
}


bool CrossedCellBoundary(ivec2 oldCell, ivec2 newCell)
{
	return any(notEqual(oldCell, newCell));
}

vec3 IntersectCellBoundary(vec2 minUV, vec2 maxUV, vec3 originTS, vec3 rayDir)
{
	const float INF = 1e20;
	const float EPSILON = 1e-7;

	float boundaryU = rayDir.x > 0 ? maxUV.x : minUV.x;
	float boundaryV = rayDir.y > 0 ? maxUV.y : minUV.y;

	float tx = abs(rayDir.x) > EPSILON
		? (boundaryU - originTS.x) / rayDir.x
		: INF;

	float ty = abs(rayDir.y) > EPSILON
		? (boundaryV - originTS.y) / rayDir.y
		: INF;

	if (tx <= EPSILON)
		tx = INF;

	if (ty <= EPSILON)
		ty = INF;

	float t = min(tx, ty);

	vec3 result = originTS + t * rayDir;

	vec2 cellSize = maxUV - minUV;
	vec2 crossOffset = max(cellSize * 1e-4, vec2(EPSILON));

	if (tx < ty)
		result.x += sign(rayDir.x) * crossOffset.x;
	else
		result.y += sign(rayDir.y) * crossOffset.y;

	return result;
}

bool traceSSRHiZ(vec3 pos, vec3 direction, vec3 viewNormal,
	out vec2 hitUV, out float hitDistance)
{
	direction = normalize(direction);

	vec3 originVS = pos + viewNormal * normalBias + direction * rayBias;	// 防止自遮挡
	vec3 endVS = originVS + direction * maxRayDistance;

	int level = 0;

	vec3 originTS;
	vec3 endTS;
	Project2TextureSpace(endVS, endTS);
	if (!Project2TextureSpace(originVS, originTS))
		return false;
	
	vec3 ray = originTS;
	ivec2 rayCell = GetHizCell(ray.xy, level);

	vec3 rayDir = endTS - originTS;
	if (abs(rayDir.z) < 1e-6 || rayDir.z <= 0.0)
		return false;
	rayDir = rayDir.xyz / rayDir.z;

	// ray = IntersectCellBoundary()
	int iterations = 0;
	while (level >= 0 && iterations < 128
		&& ray.x >= 0 && ray.x < 1
		&& ray.y >= 0 && ray.y < 1
		&& ray.z > 0)
	{

		bool isSky = false;

		ivec2 size = textureSize(u_MipDepth, level);
		vec2 invSize = 1.0 / vec2(size);
		ivec2 cell = GetHizCell(ray.xy, level);	// oldCell

		vec2 minUV = vec2(cell) * invSize;
		vec2 maxUV = vec2(cell + ivec2(1)) * invSize;

		vec3 nextRay = ray;

		// 当前cell 的 depth
		float minZ = SampleHiZDepth(cell, level);

		float min_minus_ray = minZ - ray.z;

		nextRay = min_minus_ray > 0
			? nextRay + rayDir * min_minus_ray
			: ray;
		ivec2 newCell = GetHizCell(nextRay.xy, level);
		if (CrossedCellBoundary(cell, newCell))
		{
			nextRay = IntersectCellBoundary(minUV, maxUV, ray, rayDir);
			level = min(u_MipCount, level + 2);
		}
		else if (level == 0)
		{
			float minZOffset = (minZ + 0.0001);
			isSky = minZ == 1;

			if (isSky)
				break;

			if (nextRay.z > minZOffset)
			{
				nextRay = IntersectCellBoundary(minUV, maxUV, ray, rayDir);
				level = 1;
			}
		}
		level--;
		ray = nextRay;
		iterations++;
	}

	hitUV = ray.xy;
	hitDistance = length(originVS - TS2VS(ray));

	return level < 0 && iterations > 0;
}

vec3 FresnelSchlick(float cosineTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosineTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float alpha = roughness * roughness;
	float a2 = alpha * alpha;

	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float denom = NdotH2 * (a2 - 1.0) + 1.0;

	return a2 / (PI * denom * denom);
}

float GeoSchlickGGX(float NdotX, float roughness)
{
	float r = roughness + 1.0;
	float k = r * r / 8.0;

	return NdotX / (NdotX * (1.0 - k) + k);
}

float GeoSchlick(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	float G1NV = GeoSchlickGGX(NdotV, roughness);
	float G1NL = GeoSchlickGGX(NdotL, roughness);

	return G1NV * G1NL;
}

float CaculateEdgeFade(vec2 hitUV)
{
	vec2 distanceToEdge = min(hitUV, 1.0 - hitUV);

	float minDistance = min(distanceToEdge.x, distanceToEdge.y);

	return smoothstep(0.0, edgeFadeWidth, minDistance);
}

float CaculateDistanceFade(float distance)
{
	return 1.0 - smoothstep(maxRayDistance * distanceFadeStart,
		maxRayDistance, distance);
}

float CaculateRoughness(float roughness)
{
	return 1.0 - smoothstep(roughnessFadeStart, roughnessFadeEnd, roughness);
}

void main()
{
	vec3 baseColor = texture(u_ScreenColor, v_TexCoord).rgb;
	vec4 AS = texture(u_AlbedoAlpha, v_TexCoord);
	vec3 albedo = AS.rgb;
	float ssrStrength = AS.a;

	if (ssrStrength <= 0.001 || texture(u_CharacterNormalMask, v_TexCoord).a > 0.5)
	{
		o_Color = vec4(baseColor, 1.0f);
		return;
	}

	vec4 NR = texture(u_NormalRoughness, v_TexCoord);
	vec3 worldNormal = normalize(NR.xyz);
	vec3 viewNormal = normalize(mat3(u_Camera.View) * worldNormal);
	float roughness = clamp(NR.a, 0.04, 1.0);
	float metallic = clamp(texture(u_EmissiveMetallic, v_TexCoord).a, 0.0, 1.0);
	
	float depth = GetTexelDepth(v_TexCoord);
	vec3 posVS = GetViewSpacePos(v_TexCoord, depth);
	vec3 viewDir = normalize(-posVS);

	vec3 R = reflect(-viewDir, viewNormal);
	vec2 hitUV;
	float hitDistance;
	vec3 color = baseColor;

	//o_Color = vec4(color, 1.0f);
	//return;

	bool hit = u_UseHiZ != 0
		? traceSSRHiZ(posVS, R, viewNormal, hitUV, hitDistance)
		: traceSSR(posVS, R, viewNormal, hitUV, hitDistance);

	vec4 characterData = texture(u_CharacterNormalMask, hitUV);

	vec3 hitWorldNormal;

	if (characterData.a > 0.5)
	{
		hitWorldNormal = normalize(characterData.xyz * 2.0 - 1.0);
	}
	else
	{
		hitWorldNormal = normalize(texture(u_NormalRoughness, hitUV).xyz);
	}

	vec3 hitNormalVS =
		normalize(mat3(u_Camera.View) * hitWorldNormal);

	float hitFacing = dot(hitNormalVS, -R);

	if (hitFacing <= 0.0)
		hit = false;

	if (hit)
	{
		//o_Color = vec4(1.0f);
		//return;

		vec3 reflectColor = texture(u_ScreenColor, hitUV).rgb;

		vec3 F0 = mix(vec3(0.04), albedo, metallic);
		float NdotV = max(dot(viewNormal, viewDir), 0.0);

		// specular
		vec3 F = FresnelSchlick(NdotV, F0);

		float roughnessFade = CaculateRoughness(roughness);
		float edgeFade = CaculateEdgeFade(hitUV);
		float distanceFade = CaculateDistanceFade(hitDistance);
		float confidence = edgeFade * distanceFade * roughnessFade;
		// float confidence = 1.0;

		// color = mix(baseColor, reflectColor, reflectionWeight * confidence);
		color = baseColor + reflectColor * F * confidence * ssrStrength;
	}
	// o_Color = vec4(0.0f);
	o_Color = vec4(color, 1.0f);
}
