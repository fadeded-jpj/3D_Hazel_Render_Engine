#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "Hazel/Profile/RenderProfile.h"
#include "Hazel/Renderer/RHI/RenderCommand.h"

namespace Engine
{
	class RenderGPUProfiler
	{
	public:
		static void Init()
		{
			if (s_Initialized)
				return;

			for (auto& frame : s_Frames)
			{
				for (auto& query : frame.Queries)
				{
					query.Begin = RenderCommand::CreateTimestampQuery();
					query.End = RenderCommand::CreateTimestampQuery();
				}
			}

			s_Initialized = true;
		}

		static void Shutdown()
		{
			if (!s_Initialized)
				return;

			for (auto& frame : s_Frames)
			{
				for (auto& query : frame.Queries)
				{
					RenderCommand::DeleteTimestampQuery(query.Begin);
					RenderCommand::DeleteTimestampQuery(query.End);
					query = {};
				}
				frame.Pending = false;
			}

			s_CurrentFrame = nullptr;
			s_FrameIndex = 0;
			s_Initialized = false;
		}

		static void SetEnabled(bool enabled) { s_Enabled = enabled; }
		static bool IsEnabled() { return s_Enabled; }

		static void BeginFrame(RenderProfile& output)
		{
			s_CurrentFrame = nullptr;
			if (!s_Initialized || !s_Enabled)
				return;

			auto& frame = s_Frames[s_FrameIndex % FramesInFlight];
			if (frame.Pending && !TryResolve(frame, output))
				return;

			for (auto& query : frame.Queries)
				query.Issued = false;

			s_CurrentFrame = &frame;
			BeginPass(RenderProfilePass::Frame);
		}

		static void EndFrame()
		{
			if (!s_CurrentFrame)
				return;

			EndPass(RenderProfilePass::Frame);
			s_CurrentFrame->Pending = true;
			s_CurrentFrame = nullptr;
			++s_FrameIndex;
		}

		static void BeginPass(RenderProfilePass pass)
		{
			if (!s_CurrentFrame)
				return;

			auto& query = s_CurrentFrame->Queries[ToIndex(pass)];
			HZ_CORE_ASSERT(!query.Issued, "GPU profile pass was submitted more than once in one frame");
			query.Issued = true;
			RenderCommand::WriteTimestamp(query.Begin);
		}

		static void EndPass(RenderProfilePass pass)
		{
			if (!s_CurrentFrame)
				return;

			auto& query = s_CurrentFrame->Queries[ToIndex(pass)];
			HZ_CORE_ASSERT(query.Issued, "GPU profile pass ended before it began");
			RenderCommand::WriteTimestamp(query.End);
		}

	private:
		static constexpr size_t PassCount = static_cast<size_t>(RenderProfilePass::Count);
		static constexpr size_t FramesInFlight = 4;

		struct TimestampQuery
		{
			uint32_t Begin = 0;
			uint32_t End = 0;
			bool Issued = false;
		};

		struct FrameQueries
		{
			std::array<TimestampQuery, PassCount> Queries{};
			bool Pending = false;
		};

		static constexpr size_t ToIndex(RenderProfilePass pass)
		{
			return static_cast<size_t>(pass);
		}

		static bool TryResolve(FrameQueries& frame, RenderProfile& output)
		{
			for (const auto& query : frame.Queries)
			{
				if (query.Issued && !RenderCommand::IsTimestampAvailable(query.End))
					return false;
			}

			for (size_t index = 0; index < PassCount; ++index)
			{
				const auto& query = frame.Queries[index];
				if (!query.Issued)
					continue;

				const uint64_t begin = RenderCommand::GetTimestamp(query.Begin);
				const uint64_t end = RenderCommand::GetTimestamp(query.End);
				auto& profile = GetPassProfile(output, static_cast<RenderProfilePass>(index));
				profile.GPUExecuted = true;
				profile.GPUTimeMs = static_cast<double>(end - begin) / 1'000'000.0;
			}

			frame.Pending = false;
			return true;
		}

		static RenderPassProfile& GetPassProfile(RenderProfile& profile, RenderProfilePass pass)
		{
			switch (pass)
			{
			case RenderProfilePass::Frame: return profile.Frame;
			case RenderProfilePass::SceneCSMShadow: return profile.SceneCSMShadow;
			case RenderProfilePass::SceneShadow2D: return profile.SceneShadow2D;
			case RenderProfilePass::ScenePointShadow: return profile.ScenePointShadow;
			case RenderProfilePass::CharacterShadow: return profile.CharacterShadow;
			case RenderProfilePass::GBuffer: return profile.GBuffer;
			case RenderProfilePass::SSAO: return profile.SSAO;
			case RenderProfilePass::Lighting: return profile.Lighting;
			case RenderProfilePass::ScreenSpaceOutline: return profile.ScreenSpaceOutline;
			case RenderProfilePass::InvertedHullOutline: return profile.InvertedHullOutline;
			case RenderProfilePass::Character: return profile.Character;
			case RenderProfilePass::GeometryOutline: return profile.GeometryOutline;
			case RenderProfilePass::SSR: return profile.SSR;
			case RenderProfilePass::Transparent: return profile.Transparent;
			case RenderProfilePass::TAA: return profile.TAA;
			case RenderProfilePass::Bloom: return profile.Bloom;
			case RenderProfilePass::ToneMapping: return profile.ToneMapping;
			default:
				HZ_CORE_ASSERT(false, "Unknown render profile pass");
				return profile.Frame;
			}
		}

		inline static std::array<FrameQueries, FramesInFlight> s_Frames{};
		inline static FrameQueries* s_CurrentFrame = nullptr;
		inline static uint64_t s_FrameIndex = 0;
		inline static bool s_Initialized = false;
		inline static bool s_Enabled = true;
	};

	class RenderPassGPUTimer
	{
	public:
		explicit RenderPassGPUTimer(RenderProfilePass pass)
			: m_Pass(pass)
		{
			RenderGPUProfiler::BeginPass(m_Pass);
		}

		~RenderPassGPUTimer()
		{
			RenderGPUProfiler::EndPass(m_Pass);
		}

		RenderPassGPUTimer(const RenderPassGPUTimer&) = delete;
		RenderPassGPUTimer& operator=(const RenderPassGPUTimer&) = delete;

	private:
		RenderProfilePass m_Pass;
	};
}
