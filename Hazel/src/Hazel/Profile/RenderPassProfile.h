#pragma once

#include <chrono>

namespace Engine
{
	struct RenderPassItemStatistics
	{
		unsigned int InputItems = 0;
		unsigned int VisibleItems = 0;
		unsigned int CulledItems = 0;
	};

    struct RenderPassProfile
    {
        unsigned int DrawItems = 0;
        unsigned int DrawCalls = 0;

        unsigned int ShaderBinds = 0;
        unsigned int ParameterApplyCalls = 0;
        unsigned int UniformUploads = 0;
        unsigned int TextureBinds = 0;

        bool Executed = false;
        double CpuTimeMs = 0.0;

		bool GPUExecuted = false;
		double GPUTimeMs = 0.0;

		RenderPassItemStatistics ItemStatistics;
    };

    class RenderPassCPUTimer
    {
    public:
        explicit RenderPassCPUTimer(RenderPassProfile& profile)
            : m_Profile(profile), m_Begin(std::chrono::steady_clock::now())
        {
            m_Profile.Executed = true;
        }

        ~RenderPassCPUTimer()
        {
            const auto end = std::chrono::steady_clock::now();
            m_Profile.CpuTimeMs +=
                std::chrono::duration<double, std::milli>(end - m_Begin).count();
        }

        RenderPassCPUTimer(const RenderPassCPUTimer&) = delete;
        RenderPassCPUTimer& operator=(const RenderPassCPUTimer&) = delete;

    private:
        RenderPassProfile& m_Profile;
        std::chrono::steady_clock::time_point m_Begin;
    };
}
