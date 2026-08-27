#include "hzpch.h"
#include "OpenGLContext.h"

#include "Hazel/Core/Core.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <chrono>

namespace Engine
{
	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		:m_WindowHandle(windowHandle)
	{
		HZ_CORE_ASSERT(windowHandle, "Window handle is null!");
	}
	void OpenGLContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		HZ_CORE_ASSERT(status, "Failed to initailize Glad!");

		HZ_CORE_INFO("OpenGL Vendor: {0}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
		HZ_CORE_INFO("OpenGL Renderer: {0}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
		HZ_CORE_INFO("OpenGL Version: {0}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
	}
	void OpenGLContext::SwapBuffers()
	{
		// auto t0 = std::chrono::steady_clock::now();

		// glFinish();

		// auto t1 = std::chrono::steady_clock::now();
		glfwSwapBuffers(m_WindowHandle);
		// auto t2 = std::chrono::steady_clock::now();

		//const double finishTime =Rend
		//	std::chrono::duration<double, std::milli>(
		//		t1 - t0).count();

		//const double swapTime =
		//	std::chrono::duration<double, std::milli>(
		//		t2 - t1).count();

		// HZ_CORE_INFO("Finish Time:{0} ms, Swap Time:{1} ms", finishTime, swapTime);
	}
}