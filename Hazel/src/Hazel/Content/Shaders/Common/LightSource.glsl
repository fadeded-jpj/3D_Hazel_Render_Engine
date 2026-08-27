const int MAX_LIGHTS = 16;

struct RenderLight
{
	vec4 ColorIntensity;	// (rgb, instensity)
	vec4 PositionRange;		// (world postion, range)
	vec4 DirectionType;		// (light direction, lightType)
	vec4 SpotAngles;
};

uniform int u_LightCount;
uniform RenderLight u_Lights[MAX_LIGHTS];

