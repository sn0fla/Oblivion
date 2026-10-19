#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>

namespace Logging {

	bool Init(const std::string& dir = "C:\\Oblivion\\Logs");
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