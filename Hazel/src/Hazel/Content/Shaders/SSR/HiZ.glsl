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
layout(location = 0) out float o_Color;

in vec2 v_TexCoord;

#include "../Common/CameraUniform.glsl"

uniform sampler2D u_SourceTexture;
uniform sampler2D u_RawDepth;
uniform int u_CurrentMipLevel;
ivec2 GetSamplePixel(ivec2 pixel, ivec2 offset, ivec2 size)
{
    return clamp(pixel + offset, ivec2(0), ivec2(size) - ivec2(1));
}

float Sample2x2(sampler2D tex, vec2 uv, int mipLevel)
{
    ivec2 srcSize = textureSize(tex, mipLevel);
    ivec2 dstSize = textureSize(tex, mipLevel + 1);
    ivec2 dstPixel = ivec2(gl_FragCoord.xy);    // 当前 mip 的像素坐标

    ivec2 srcBegin = dstPixel * srcSize / dstSize;
    ivec2 srcEnd =
        ((dstPixel + ivec2(1)) * srcSize
            + dstSize - ivec2(1))
        / dstSize;

    srcEnd = min(srcEnd, srcSize);

    float minDepth = u_Camera.CameraClip.y;

    for (int y = srcBegin.y; y < srcEnd.y; ++y)
    {
        for (int x = srcBegin.x; x < srcEnd.x; ++x)
        {
            minDepth = min(minDepth, texelFetch(tex, ivec2(x, y), mipLevel).r);
        }
    }

    return minDepth;
}


float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * u_Camera.CameraClip.x * u_Camera.CameraClip.y) /
        (u_Camera.CameraClip.y + u_Camera.CameraClip.x -
            z * (u_Camera.CameraClip.y - u_Camera.CameraClip.x));
}

void main()
{
    if (u_CurrentMipLevel == 0)
    {
        ivec2 size = textureSize(u_RawDepth, 0);
        ivec2 pixel = ivec2(size * v_TexCoord);
        float depth = texelFetch(u_RawDepth, pixel, 0).r;
        // o_Color = LinearizeDepth(depth);
        o_Color = depth;
        return;
    }

    o_Color = Sample2x2(u_SourceTexture, v_TexCoord, u_CurrentMipLevel - 1);
    // o_Color = vec4(SampleSource(v_TexCoord), 1.0);
}

