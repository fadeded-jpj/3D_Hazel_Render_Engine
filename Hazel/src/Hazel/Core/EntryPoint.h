#pragma once

#ifdef HZ_PLATFORM_WINDOWS

extern Engine::Application* Engine::CreateApplication();

int main(int argc, char** argv)
{
	Engine::Log::Init();
	HZ_CORE_WARN("warn");
	int a = 5;
	HZ_CORE_ERROR("INFO! var={0}", a);


	auto app = Engine::CreateApplication();
	app->Run();
	delete app;
}
#endif