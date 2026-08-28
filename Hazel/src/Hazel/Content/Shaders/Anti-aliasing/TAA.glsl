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

uniform sampler2D u_CurrentColor;
uniform sampler2D u_HistoryColor;
uniform sampler2D u_PreviousDepth;
uniform int u_HistoryValid;
uniform sampler2D u_CurrentDepth;
uniform sampler2D u_VelocityBuffer;
uniform sampler2D u_PreviousVelocityBuffer;
uniform int u_HighQualitySampling;

vec2 GetSkyVelocity(vec2 uv)
{
	vec2 ndc = uv * 2.0 - 1.0;

	vec4 worldFar = u_Camera.InverseViewProj * vec4(ndc, 1.0, 1.0);

	vec3 directionH =
		worldFar.xyz -
		u_Camera.CameraPositionAndTime.xyz * worldFar.w;

	float lengthSquared = dot(directionH, directionH);

	if (lengthSquared < 1e-10)
		return vec2(0.0);

	vec3 worldDir =
		directionH * inversesqrt(lengthSquared);

	vec4 currentClip = u_Camera.StabledViewProj * vec4(worldDir, 0.0);
	vec4 previousClip = u_Camera.PreViewProj * vec4(worldDir, 0.0);

	if (currentClip.w <= 0.0 || previousClip.w <= 0.0)
		return vec2(0.0);

	vec2 currentUV = currentClip.xy / currentClip.w * 0.5 + 0.5;
	vec2 previousUV = previousClip.xy / previousClip.w * 0.5 + 0.5;

	return currentUV - previousUV;
}

const ivec2 samplePoints[] =
{
	ivec2(-1, -1), ivec2(-1, 1), ivec2(1, -1), ivec2(1, 1), ivec2(0, 0)
};


float LinearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0;
	return (2.0 * u_Camera.CameraClip.x * u_Camera.CameraClip.y) /
		(u_Camera.CameraClip.y + u_Camera.CameraClip.x -
			z * (u_Camera.CameraClip.y - u_Camera.CameraClip.x));
}

vec3 RGB2YCoCgR(vec3 rgbColor)
{
	vec3 YCoCgRColor;

	YCoCgRColor.y = rgbColor.r - rgbColor.b;
	float temp = rgbColor.b + YCoCgRColor.y / 2;
	YCoCgRColor.z = rgbColor.g - temp;
	YCoCgRColor.x = temp + YCoCgRColor.z / 2;

	return YCoCgRColor;
}

vec3 YCoCgR2RGB(vec3 YCoCgRColor)
{
	vec3 rgbColor;

	float temp = YCoCgRColor.x - YCoCgRColor.z / 2;
	rgbColor.g = YCoCgRColor.z + temp;
	rgbColor.b = temp - YCoCgRColor.y / 2;
	rgbColor.r = rgbColor.b + YCoCgRColor.y;

	return rgbColor;
}

float Luminance(vec3 color)
{
	return 0.25 * color.r + 0.5 * color.g + 0.25 * color.b;
}

vec3 ToneMap(vec3 color)
{
	return color / (1 + Luminance(color));
}

vec3 UnToneMap(vec3 color)
{
	return color / (1 - Luminance(color));

	//float luminance = clamp(
	//	Luminance(color),
	//	0.0,
	//	1.0 - 1e-4);

	//return color / max(1.0 - luminance, 1e-4);
}



vec2 GetClosestCurrentDepthUV(vec2 uv)
{
	ivec2 size = textureSize(u_CurrentDepth, 0);
	ivec2 center = ivec2(uv * vec2(size));

	float closestDepth = 1.0;
	ivec2 closestPixel = center;

	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			ivec2 p = clamp(center + ivec2(x, y),
				ivec2(0), size - ivec2(1));

			float currentDepth = texelFetch(u_CurrentDepth, p, 0).r;
			if (currentDepth < closestDepth)
			{
				closestDepth = currentDepth;
				closestPixel = p;
			}
		}
	}
	return (vec2(closestPixel) + 0.5) / vec2(size);
}

vec2 GetClosestCurrentDepthUVQuad(vec2 uv)
{
	ivec2 size = textureSize(u_CurrentDepth, 0);
	ivec2 center = ivec2(uv * vec2(size));

	float closestDepth = 1.0;
	ivec2 closestPixel = center;

	for (int i = 0; i < 4; i++)
	{
		ivec2 p = clamp(center + samplePoints[i],
			ivec2(0), size - ivec2(1));

		float currentDepth = texelFetch(u_CurrentDepth, p, 0).r;
		if (currentDepth < closestDepth)
		{
			closestDepth = currentDepth;
			closestPixel = p;
		}
	}
	return (vec2(closestPixel) + 0.5) / vec2(size);
 }

vec4 SampleHistoryColor(vec2 uv, vec3 backColor, out vec2 velocity, out vec2 historyUV)
{
	if (u_HistoryValid == 0)
		return vec4(backColor, 1.0);

	float centerDepth = texture(u_CurrentDepth, uv).r;
	if (centerDepth >= 0.99999)
	{
		velocity = GetSkyVelocity(uv);
	}
	else
	{
		vec2 closetUV = u_HighQualitySampling != 0
			? GetClosestCurrentDepthUV(uv)
			: GetClosestCurrentDepthUVQuad(uv);
		// vec2 closetUV = uv;

		velocity = texture(u_VelocityBuffer, closetUV).rg;
	}

	historyUV = uv - velocity;

	if (historyUV.x < 0.0 || historyUV.x > 1.0 || historyUV.y < 0.0 || historyUV.y > 1.0)
		return vec4(backColor, 1.0);

	// previousUV = uv;
	return texture(u_HistoryColor, historyUV);
}

// 3x3 相邻候选 clamp
void GetNeighborhoodRange(vec2 uv, vec3 centerColor, out vec3 minColor, out vec3 maxColor)
{
	ivec2 size = textureSize(u_CurrentColor, 0);
	ivec2 center = ivec2(uv * vec2(size));

	minColor = vec3(0);
	maxColor = vec3(0);

	if (u_HighQualitySampling != 0)
	{
		for (int y = -1; y <= 1; y++)
		{
			for (int x = -1; x <= 1; x++)
			{
				ivec2 p = clamp(center + ivec2(x, y), ivec2(0), size - ivec2(1));
				vec3 c = y == 0 && x == 0
					? RGB2YCoCgR(ToneMap(centerColor))
					: RGB2YCoCgR(ToneMap(texelFetch(u_CurrentColor, p, 0).rgb));

				minColor += c;
				maxColor += c * c;
			}
		}
	}
	else
	{
		for (int i = 0; i < 5; ++i)
		{
			ivec2 p = clamp(center + samplePoints[i], ivec2(0), size - ivec2(1));
			vec3 c = all(equal(samplePoints[i], ivec2(0)))
				? RGB2YCoCgR(ToneMap(centerColor))
				: RGB2YCoCgR(ToneMap(texelFetch(u_CurrentColor, p, 0).rgb));

			minColor += c;
			maxColor += c * c;
		}
	}
}


vec3 ClampHistoryColor(vec3 historyColor, vec3 centerColor, vec2 uv)
{
	vec3 aabbMin;
	vec3 aabbMax;

	vec3 m1 = vec3(0);
	vec3 m2 = vec3(0);

	GetNeighborhoodRange(v_TexCoord, centerColor, m1, m2);

	// Variance clip
	int N = u_HighQualitySampling != 0 ? 9 : 5;
	float  VarianceClipGamma = 1.5;
	vec3 mu = m1 / N;								// 均值
	vec3 sigma = sqrt(abs(m2 / N - mu * mu));		// 标准差

	// 置信区间
	aabbMin = mu - VarianceClipGamma * sigma;
	aabbMax = mu + VarianceClipGamma * sigma;


	// clip
	vec3 center = 0.5 * (aabbMin + aabbMax);	// AABB 中心
	vec3 extent = 0.5 * (aabbMax - aabbMin);	// AABB 半径

	vec3 v_clip = historyColor - center;	// history 到中心的向量
	vec3 v_unit = v_clip.xyz / max(extent, vec3(1e-5));	// AABB 归一化到[-1, 1]
	vec3 a_unit = abs(v_unit);		// 用于判断是否在AABB包围盒内

	float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

	if (ma_unit > 1.0)				// 在包围盒外， 计算交点
		return center + v_clip / ma_unit;
	else                            // 在包围盒内， 本身就可信
		return historyColor;

	// return clamp(historyColor, aabbMin, aabbMax);
}

float VelocityLengthInPixels(vec2 velocity)
{
	ivec2 size = textureSize(u_VelocityBuffer, 0);
	return length(velocity * vec2(size)); //	velocity 像素距离
}

float MaxPreviousVelocity3x3(sampler2D velocitybuffer, vec2 uv)
{
	ivec2 size = textureSize(velocitybuffer, 0);
	vec2 previousUV = uv + 0.5 *
		(u_Camera.ViewportSizeAndJittered.zw - u_Camera.CameraClip.zw);
	ivec2 pixel = ivec2(previousUV * size);
	float maxVelocity = 0.0f;

	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++)
		{
			ivec2 p = clamp(pixel + ivec2(x, y), ivec2(0),
				size - ivec2(1));

			vec2 velocity = texelFetch(velocitybuffer, p, 0).rg * size;
			
			maxVelocity = max(maxVelocity, dot(velocity, velocity));
		}
	}
	return sqrt(maxVelocity);
}

float MaxPreviousVelocityQuad(sampler2D velocitybuffer, vec2 uv)
{
	ivec2 size = textureSize(velocitybuffer, 0);
	vec2 previousUV = uv + 0.5 *
		(u_Camera.ViewportSizeAndJittered.zw - u_Camera.CameraClip.zw);
	ivec2 pixel = ivec2(previousUV * size);
	float maxVelocity = 0.0f;

	for (int i = 0; i < 5; i++)
	{
		ivec2 p = clamp(pixel + samplePoints[i], ivec2(0),
			size - ivec2(1));

		vec2 velocity = texelFetch(velocitybuffer, p, 0).rg * size;

		maxVelocity = max(maxVelocity, dot(velocity, velocity));
	}
			

	return sqrt(maxVelocity);
}

float GetHistoryWeight(vec2 velocity)
{
	float v = VelocityLengthInPixels(velocity); 

	float band = smoothstep(1.0, 2.0, v);

	return 0.8;

	return mix(0.9, 0.5, band);
}

float GetDepthAdaptiveForce(vec2 currentUV, vec2 historyUV)
{
	ivec2 size = textureSize(u_CurrentDepth, 0);
	float currentDepth = texelFetch(u_CurrentDepth, ivec2(currentUV * size), 0).r;
	
	vec2 previousDepthUV = clamp(
		historyUV + 0.5 *
			(u_Camera.ViewportSizeAndJittered.zw - u_Camera.CameraClip.zw),
		vec2(0), vec2(1.0));
	float previousDepth = texelFetch(u_PreviousDepth, ivec2(previousDepthUV * size), 0).r;

	float currentZ = LinearizeDepth(currentDepth);
	float previousZ = LinearizeDepth(previousDepth);

	return abs(currentZ - previousZ) < 0.05 ? 1.0 : 0.3;
}

void main()
{
	vec3 currentColorRaw = texture(u_CurrentColor, v_TexCoord).rgb;
	if (u_HistoryValid == 0)
	{
		o_Color = vec4(currentColorRaw, 1.0);
		return;
	}
	vec3 currentColor = RGB2YCoCgR(ToneMap(currentColorRaw));
	vec2 velocity;
	vec2 historyUV;
	vec4 historyColorSampler = SampleHistoryColor(v_TexCoord, currentColorRaw, velocity, historyUV);
	vec3 rawHistoryColor = RGB2YCoCgR(ToneMap(historyColorSampler.rgb));

	bool historyUVValid =
		all(greaterThanEqual(historyUV, vec2(0.0))) &&
		all(lessThanEqual(historyUV, vec2(1.0)));

	if (!historyUVValid)
	{
		o_Color = vec4(currentColorRaw, 1.0);
		return;
	}

	vec3 clipHistoryColor = ClampHistoryColor(rawHistoryColor, currentColorRaw, v_TexCoord);

	// clip confidence
	float currentMotion = VelocityLengthInPixels(velocity);
	float previousMotion = u_HighQualitySampling != 0
		? MaxPreviousVelocity3x3(u_PreviousVelocityBuffer, historyUV)
		: MaxPreviousVelocityQuad(u_PreviousVelocityBuffer, historyUV);
	float motionFactor = max(currentMotion, previousMotion);
	float depthAdaptiveForce = GetDepthAdaptiveForce(v_TexCoord, historyUV);
	float previousConfidence = historyColorSampler.a;
	depthAdaptiveForce = min(depthAdaptiveForce, previousConfidence);
	float clampConfidence;

	float currentVelocityWeight =
		clamp(currentMotion * 0.5, 0.0, 1.0);

	float previousVelocityWeight =
		clamp(previousMotion * 0.5, 0.0, 1.0);
	
	// 当前帧运动时，清除黑历史
	depthAdaptiveForce = mix(depthAdaptiveForce, 1.0, currentVelocityWeight);

	// 上一帧运动时， 也清除黑历史
	depthAdaptiveForce = mix(depthAdaptiveForce, 1.0, previousVelocityWeight);


	clampConfidence = depthAdaptiveForce;
	vec3 historyColor = mix(rawHistoryColor, clipHistoryColor, clampConfidence);

	float historyWeight = GetHistoryWeight(velocity);
	
	vec3 color = mix(UnToneMap(YCoCgR2RGB(currentColor)),
		UnToneMap(YCoCgR2RGB(historyColor)), historyWeight);

	if (u_HistoryValid == 0)
		color = UnToneMap(YCoCgR2RGB(currentColor));

	o_Color = vec4(color, clampConfidence);
}
