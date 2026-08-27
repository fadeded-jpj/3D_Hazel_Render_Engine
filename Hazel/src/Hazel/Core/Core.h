#pragma once

#include <memory>

#ifdef HZ_PLATFORM_WINDOWS
#if HZ_DYNAMIC_LINK
	#ifdef HZ_BUILD_DLL
		#define HAZEL_API __declspec(dllexport)
	#else
		#define HAZEL_API __declspec(dllimport)
	#endif
#else
	#define HAZEL_API
#endif
#else
	#error Hazel only supports Windows!
#endif

#ifdef HZ_ENABLE_ASSERTS
	#define HZ_ASSERT(x, ...) { if(!(x)) { HZ_CLIENT_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define HZ_CORE_ASSERT(x, ...) { if(!(x)) { HZ_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define HZ_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define HZ_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

namespace Engine
{
	template<typename T>
	using Scope = std::unique_ptr<T>;
	
	template<typename T>
	using Ref = std::shared_ptr<T>;

	template<typename T>
	using WeakRef = std::weak_ptr<T>;
}

#ifdef HZ_PLATFORM_WINDOWS
#define NOMINMAX
#include <Windows.h>

static std::string WideToUtf8(const std::wstring& value)
{
    if (value.empty())
        return {};

    int size = WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        (int)value.size(),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    std::string result(size, 0);

    WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        (int)value.size(),
        result.data(),
        size,
        nullptr,
        nullptr
    );

    return result;
}

static std::wstring Utf8ToWide(const std::string& value)
{
    if (value.empty())
        return {};

    int size = MultiByteToWideChar(
        CP_UTF8,
        0,
        value.data(),
        (int)value.size(),
        nullptr,
        0
    );

    std::wstring result(size, L'\0');

    MultiByteToWideChar(
        CP_UTF8,
        0,
        value.data(),
        (int)value.size(),
        result.data(),
        size
    );

    return result;
}

#endif

