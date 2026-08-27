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
layout(location = 0) out vec2 o_ShadowClass;

in vec2 v_TexCoord;

#include "CameraUniform.glsl"

uniform sampler2D u_GBufferDepth;
uniform sampler2D u_GBufferNormalRoughness;

// for scene shadow
uniform int u_ShadowEnabled;
uniform int u_CSMEnabled;
uniform mat4 u_CSMViewProj[4];
uniform vec4 u_CSMSplits;
uniform float u_OverlapRatio;
uniform float u_ShadowBias;

// for character shadow
uniform int u_CharacterShadowEnabled;
uniform mat4 u_CharacterShadowViewProj;
uniform float u_CharacterShadowBias;

// DirectionType.xyz stores the direction in which the directional light points.
uniform vec3 u_MainLightDirection;

// Global shadow resources reserve slots 1-4; the scene CSM is always slot 3.
layout(binding = 3) uniform sampler2DArray u_CSMShadowMap;
layout(binding = 4) uniform sampler2D u_CharacterShadowMap;

// 0.0 = fully shadowed
// 0.5 = uncertain; evaluate PCF in the lighting pass
// 1.0 = fully lit

const ivec2 SAMPLE_OFFSETS[6] = ivec2[](
    ivec2(0, 0),
    ivec2(3, 0),
    ivec2(0, 3),
    ivec2(3, 3),
    ivec2(1, 1),
    ivec2(2, 2)
    );

int SelectMaskCascade(float viewDepth)
{
    if (viewDepth <= u_CSMSplits.x) return 0;
    if (viewDepth <= u_CSMSplits.y) return 1;
    if (viewDepth <= u_CSMSplits.z) return 2;
    if (viewDepth <= u_CSMSplits.w) return 3;
    return -1;
}

vec3 ReconstructWorldPosition(ivec2 pixel, float depth)
{
    ivec2 size = textureSize(u_GBufferDepth, 0);
    vec2 uv = (vec2(pixel) + 0.5) / vec2(size);

    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 world = u_Camera.InverseViewProj * clip;
    return world.xyz / world.w;
}

float EveluateHardCSM(vec3 worldPos, vec3 normal, vec3 lightDir, out int cascade, out bool uncertain)
{
    float viewDepth = -(u_Camera.View * vec4(worldPos, 1.0)).z;

    cascade = SelectMaskCascade(viewDepth);
    uncertain = false;

    if (cascade < 0)
        return 1.0;

    float cascadeNear = cascade == 0
        ? u_Camera.CameraClip.x
        : u_CSMSplits[cascade - 1];
    float cascadeFar = u_CSMSplits[cascade];

    if (cascade < 3)
    {
        float overlapStart = cascadeFar -
            (cascadeFar - cascadeNear) * u_OverlapRatio;
        uncertain = viewDepth >= overlapStart;
    }

    vec4 clip = u_CSMViewProj[cascade] * vec4(worldPos, 1.0);
    if (abs(clip.w) < 1e-6)
        return 1.0;

    vec3 shadowCoord = clip.xyz / clip.w;
    shadowCoord = shadowCoord * 0.5 + 0.5;

    if (any(lessThan(shadowCoord, vec3(0.0))) ||
        any(greaterThan(shadowCoord, vec3(1.0))))
    {
        return 1.0;
    }

    float closestDepth = texture(u_CSMShadowMap, vec3(shadowCoord.xy, cascade)).r;
    float NdotL = max(dot(normal, lightDir), 0.0);

    float bias = max(u_ShadowBias * (1.0 - NdotL), 0.0001);
    return shadowCoord.z - bias <= closestDepth ? 1.0 : 0.0;
}

float EveluateHardCharacterShadow(vec3 worldPos, vec3 normal, vec3 lightDir)
{
    vec4 clip = u_CharacterShadowViewProj * vec4(worldPos, 1.0);
    if (abs(clip.w) <= 1e-6)
        return 1.0;
    vec3 shadowCoord = clip.xyz / clip.w;
    shadowCoord = shadowCoord * 0.5 + 0.5;

    if (any(lessThan(shadowCoord, vec3(0.0))) ||
        any(greaterThan(shadowCoord, vec3(1.0))))
    {
        return 1.0;
    }
    
    float closetDepth = texture(u_CharacterShadowMap, shadowCoord.xy).r;
    float NdotL = max(dot(normal, lightDir), 0.0);

    float bias = max(u_CharacterShadowBias * (1.0 - NdotL), 0.0001);
    return shadowCoord.z - bias <= closetDepth ? 1.0 : 0.0;
}

void main()
{
    float sceneMask;
    float characterMask;

    if (u_ShadowEnabled == 0 || u_CSMEnabled == 0)
    {
        sceneMask = 1.0;
    }

    if (u_CharacterShadowEnabled == 0)
        characterMask = 1.0;

    if ((u_ShadowEnabled == 0 || u_CSMEnabled == 0) && u_CharacterShadowEnabled == 0)
    {
        o_ShadowClass = vec2(1.0);
        return;
    }

    ivec2 fullSize = textureSize(u_GBufferDepth, 0);
    ivec2 maskPixel = ivec2(gl_FragCoord.xy);

    ivec2 blockOrigin = maskPixel * 4;

    vec3 lightDir = normalize(-u_MainLightDirection);

    bool hasLit = false;
    bool hasShadow = false;
    bool uncertain = false;
    bool hasReceiver = false;
    int firstCascade = -1;

    bool characterHasLit = false;
    bool characterHasShadow = false;
    bool characterUncertain = false;

    for (int i = 0; i < 6; i++)
    {
        ivec2 pixel = clamp(blockOrigin + SAMPLE_OFFSETS[i],
            ivec2(0), fullSize - ivec2(1));

        float depth = texelFetch(u_GBufferDepth, pixel, 0).r;

        if (depth >= 1.0 - 0.00001)
            continue;

        vec3 normal = normalize(texelFetch(u_GBufferNormalRoughness, pixel, 0).xyz);

        if (dot(normal, lightDir) <= 0.0)
            continue;

        hasReceiver = true;

        vec3 worldPos = ReconstructWorldPosition(pixel, depth);
        int cascade;
        bool sampleUncertain;
        float sceneVisibility = EveluateHardCSM(worldPos, normal, lightDir, cascade, sampleUncertain);
        float characterVisibility = EveluateHardCharacterShadow(worldPos, normal, lightDir);
        
        hasLit = hasLit || (sceneVisibility > 0.5);
        hasShadow = hasShadow || (sceneVisibility <= 0.5);
        uncertain = uncertain || sampleUncertain;
        if (firstCascade < 0)
            firstCascade = cascade;
        else if (firstCascade != cascade)
            uncertain = true;

        characterHasLit = characterHasLit || (characterVisibility > 0.5);
        characterHasShadow = characterHasLit || (characterVisibility <= 0.5);
    }

    if (!hasReceiver)
        sceneMask = 1.0;
    else if (uncertain || (hasLit && hasShadow))
        sceneMask = 0.5;
    else if (hasShadow)
        sceneMask = 0.0;
    else
        sceneMask = 1.0;

    if (characterHasLit && characterHasShadow)
        characterMask = 0.5;
    else if (characterHasLit)
        characterMask = 1.0;
    else
        characterMask = 0.0;

    o_ShadowClass = vec2(sceneMask, characterMask);
}

