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
uniform float u_Threshold;
uniform float u_Knee;


void main()
{
	vec3 color = texture(u_SceneColor, v_TexCoord).xyz;

	float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float knee = max(u_Knee, 0.00001);
	float soft = brightness - u_Threshold + knee;
	soft = clamp(soft, 0.0, 2.0 * knee);
	soft = soft * soft / (4.0 * knee + 0.00001);

	float contribution = max(brightness - u_Threshold, soft);
	contribution /= max(brightness, 0.00001);

	o_Color = vec4(color * contribution, 1.0);
}

