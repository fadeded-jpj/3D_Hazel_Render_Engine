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

uniform vec3 u_AmbientColor;

uniform sampler2D u_GBufferAlbedoAlpha;
uniform sampler2D u_GBufferNormalRoughness;		// Normal,  u_RimPower
uniform sampler2D u_GBufferEmissiveMetallic;	// u_RimColor * u_RimIntensity, u_RimMask
uniform sampler2D u_GBufferDepth;

uniform sampler2D u_GBufferRim;			// u_RimColor * u_RimIntensity, u_RimPower;

uniform int u_DebugView;

uniform int u_SSAOEnabled;
uniform sampler2D u_SSAOTexture;

#include "Common/GBufferToonLighting.glsl"



vec3 ReconstructWorldPosition(vec2 uv, float depth)
{
	vec2 ndcXY = uv * 2.0 - 1.0;
	vec4 clip = vec4(ndcXY, depth * 2 - 1.0, 1.0);	// xyz 都要是[-1, 1]的范围内

	vec4 world = u_Camera.InverseViewProj * clip;
	return world.xyz / world.w;
}



void main()
{
	float depth = texture(u_GBufferDepth, v_TexCoord).r;

	if (depth >= 1.0)
	{
		o_Color = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	//o_Color = vec4(1.0, 1.0, 1.0, 1.0);
	//return;

	vec4 albedoAlpha = texture(u_GBufferAlbedoAlpha, v_TexCoord);
	vec4 normalRoughness = texture(u_GBufferNormalRoughness, v_TexCoord);
	vec4 emissiveMetallic = texture(u_GBufferEmissiveMetallic, v_TexCoord);

	vec3 baseColor = albedoAlpha.rgb;


	vec3 normal = normalize(normalRoughness.rgb);
	vec3 worldPos = ReconstructWorldPosition(v_TexCoord, depth);

	vec3 L = normalize(-u_Lights[0].DirectionType.xyz);
	vec3 lightColor = u_Lights[0].ColorIntensity.rgb
		* u_Lights[0].ColorIntensity.a;
	vec3 viewDir = normalize(u_Camera.CameraPositionAndTime.xyz - worldPos);

	float NoL = max(dot(normal, L), 0.0);

	vec3 color = u_AmbientColor * baseColor;
	if (u_SSAOEnabled == 1)
		color *= texture(u_SSAOTexture, v_TexCoord).r;
	color += CacLighting(baseColor, vec3(0.0), normal, viewDir, worldPos);
	color += emissiveMetallic.rgb;

	// ---------------- rim light ----------------------
	vec4 rimColorPower = texture(u_GBufferRim, v_TexCoord);
	vec3 RimColor = rimColorPower.rgb;
	float RimPower = rimColorPower.a;

	float fresnel = clamp(1.0 - max(dot(normal, viewDir), 0.0), 0.00004, 1.0);
	float rim = pow(fresnel, RimPower);

	color += RimColor * rim;
	// -------------------------------------------------

	if (u_DebugView == 1)
		color = baseColor;
	else if (u_DebugView == 2)
		color = normal;
	else if (u_DebugView == 3)
		color = emissiveMetallic.rgb;
	else if (u_DebugView == 4)
		color = vec3(albedoAlpha.a);


	o_Color = vec4(color, 1.0);
}
