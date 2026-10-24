#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>

namespace Logging {

	bool Init();
	void Shutdown();

	void Info(const char* fmt, ...);
	void Warn(const char* fmt, ...);
	void Error(const char* fmt, ...);
	void Debug(const char* fmt, ...);

	void Sample(const char* name, double value);
	void Tick();

	void InstallCrashHandler();
	LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ep);

}