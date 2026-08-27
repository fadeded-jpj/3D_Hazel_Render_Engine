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

uniform sampler2D u_SourceTexture;
uniform int u_SourceMip;
uniform float u_Width;
uniform float u_Hight;

const float kernel[25] = float[](
    1.0, 4.0, 6.0, 4.0, 1.0,
    4.0, 16.0, 24.0, 16.0, 4.0,
    6.0, 24.0, 36.0, 24.0, 6.0,
    4.0, 16.0, 24.0, 16.0, 4.0,
    1.0, 4.0, 6.0, 4.0, 1.0
    );

vec3 SampleSource(vec2 uv)
{
    return textureLod(u_SourceTexture, uv, u_SourceMip).rgb;
}

vec3 GaussianBlur5x5(sampler2D tex, vec2 uv, int mipLevel, vec2 texelSize)
{
    vec3 res = vec3(0.0);
    int index = 0;

    for (int y = -2; y <= 2; y++)
    {
        for (int x = -2; x <= 2; x++)
        {
            vec2 offset = vec2(x, y) * texelSize;
            res += textureLod(tex, uv + offset, mipLevel).rgb * kernel[index];
            index++;
        }
    }
    return res / 256.0;
}


void main()
{
    vec2 texelSize = 1.0 / vec2(u_Width, u_Hight);
    o_Color = vec4(GaussianBlur5x5(u_SourceTexture, v_TexCoord, u_SourceMip, texelSize), 1.0);
	// o_Color = vec4(SampleSource(v_TexCoord), 1.0);
}
