#pragma once

#include "Core.h"
#include "spdlog/spdlog.h"


namespace Engine
{

	class HAZEL_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	
	private:

		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

}

// core log macros
#define HZ_CORE_ERROR(...)		::Engine::Log::GetCoreLogger()->error(__VA_ARGS__)
#define HZ_CORE_WARN(...)		::Engine::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define HZ_CORE_TRACE(...)		::Engine::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define HZ_CORE_INFO(...)		::Engine::Log::GetCoreLogger()->info(__VA_ARGS__)
#define HZ_CORE_FATAL(...)		::Engine::Log::GetCoreLogger()->fatal(__VA_ARGS__)
//#define HZ_CORE_ASSERT(x, ...) { if(!(x)) { HZ_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }

//client log macros
#define HZ_CLIENT_ERROR(...)	::Engine::Log::GetClientLogger()->error(__VA_ARGS__)
#define HZ_CLIENT_WARN(...)		::Engine::Log::GetClientLogger()->warn(__VA_ARGS__)
#define HZ_CLIENT_TRACE(...)	::Engine::Log::GetClientLogger()->trace(__VA_ARGS__)
#define HZ_CLIENT_INFO(...)		::Engine::Log::GetClientLogger()->info(__VA_ARGS__)
#define HZ_CLIENT_FATAL(...)	::Engine::Log::GetClientLogger()->fatal(__VA_ARGS__)

#ifdef HZ_DIST
	#define HZ_CORE_INFO
#else
#endif