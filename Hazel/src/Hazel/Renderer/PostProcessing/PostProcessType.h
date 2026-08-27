#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Engine
{
	enum class ToneMappingOperator
	{
		ByPass = 0,
		Reinhard,
		ReinhardExtended,
		ACESFitted,
		PBRNeutral,
		Count
	};

	struct ExposureSettings
	{
		// One EV doubles or halves the scene exposure.
		float CompensationEV = 0.0f;

		// Reserved for automatic exposure.
		bool Automatic = false;
		float MinEV = -4.0f;
		float MaxEV = 4.0f;
		float AdaptationSpeedUp = 3.0f;
		float AdaptationSpeedDown = 1.0f;
	};

	struct ReinhardToneMappingSettings
	{
		float WhitePoint = 4.0f;
	};

	struct ToneMappingSettings
	{
		bool Enabled = true;
		ToneMappingOperator Operator = ToneMappingOperator::PBRNeutral;
		ExposureSettings Exposure;
		ReinhardToneMappingSettings Reinhard;
	};

	struct BloomSettings
	{
		bool Enabled = true;
		float Threshold = 1.0f;
		float Knee = 0.5f;
		float Intensity = 0.8f;
		float Scatter = 0.5f;
		uint32_t MaxMipLevels = 6;
	};

	struct ColorGradingSettings
	{
		float Contrast = 1.0f;
		float Saturation = 1.0f;
		glm::vec3 ColorFilter{ 1.0f };
	};

	struct PostProcessSettings
	{
		ToneMappingSettings ToneMapping;
		BloomSettings Bloom;
		ColorGradingSettings ColorGrading;
	};
}
