layout(location = 1) out vec4 o_CharacterNormalMask;

void WriteCharacterNormalMask(vec3 worldNormal)
{
	o_CharacterNormalMask = vec4(
		normalize(worldNormal) * 0.5 + 0.5,
		1.0);
}
