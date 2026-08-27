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

uniform sampler2D u_SceneColor;
uniform sampler2D u_BloomTexture;
uniform int u_BloomEnabled;
uniform float u_BloomIntensity;
uniform float u_Exp;
uniform float u_ReinhardWhitePoint;
uniform int u_ToneMappingEnabled;
uniform int u_ToneMappingOperator;


vec3 Reinhard(vec3 color)
{
	return color / (vec3(1.0) + color);
}

vec3 ReinhardExtended(vec3 color)
{
	float whiteSquared =
		max(u_ReinhardWhitePoint * u_ReinhardWhitePoint, 0.0001);

	return color * (vec3(1.0) + color / whiteSquared) / (vec3(1.0) + color);
}

vec3 ACESFitted(vec3 color)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;

	return clamp(
		color * (a * color + b) /
		(color * (c * color + d) + e),
		0.0,
		1.0);
}

vec3 PBRNeutral(vec3 color)
{
	const float startCompression = 0.76;
	const float desaturation = 0.15;

	float minChannel = min(color.r, min(color.g, color.b));
	float offset = minChannel < 0.08
		? minChannel - 6.25 * minChannel * minChannel
		: 0.04;

	color -= offset;

	float peak = max(color.r, max(color.g, color.b));
	if (peak < startCompression)
		return color;

	float range = 1.0 - startCompression;
	float compressedPeak =
		1.0 - range * range /
		(peak + range - startCompression);

	float desaturationAmount =
		1.0 /
		(desaturation * (peak - compressedPeak) + 1.0);

	return mix(
		vec3(compressedPeak),
		color * compressedPeak / peak,
		desaturationAmount);
}

vec3 LinearToRGB(vec3 color)
{
	return pow(color, vec3(1.0 / 2.2));
}

void main()
{
	vec3 color = texture(u_SceneColor, v_TexCoord).xyz;
	if (u_BloomEnabled != 0)
		color += texture(u_BloomTexture, v_TexCoord).rgb * u_BloomIntensity;

	color *= exp2(u_Exp);

	if (u_ToneMappingEnabled != 0)
	{
		switch (u_ToneMappingOperator)
		{
		case 1: color = Reinhard(color);			break;
		case 2: color = ReinhardExtended(color);	break;
		case 3: color = ACESFitted(color);			break;
		case 4: color = PBRNeutral(color);			break;
		default:									break;
		}
	}

	//color = clamp(color, 0.0, 1.0);
	color = LinearToRGB(color);

	o_Color = vec4(color, 1.0);
}
