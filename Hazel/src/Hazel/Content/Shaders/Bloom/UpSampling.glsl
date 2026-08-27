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
uniform float u_Scatter;

const float kernel[9] = float[](
    1.0, 2.0, 1.0,
    2.0, 4.0, 2.0,
    1.0, 2.0, 1.0
    );


vec3 SampleSource(vec2 uv)
{
	return textureLod(u_SourceTexture, uv, u_SourceMip).rgb;
}

vec3 SampleTent3x3(sampler2D tex, vec2 uv, int mipLevel, vec2 texelSize)
{
    vec3 res = vec3(0.0);
    int index = 0;

    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            vec2 offset = vec2(x, y) * texelSize;
            res += textureLod(tex, uv + offset, mipLevel).rgb * kernel[index];
            index++;
        }
    }
    return res / 16.0;
}

void main()
{
    vec2 texelSize = 1.0 / vec2(u_Width, u_Hight);
	vec3 baseColor = u_Scatter * SampleTent3x3(
		u_SourceTexture, v_TexCoord, u_SourceMip, texelSize);
	o_Color = vec4(baseColor, 1.0);
}
