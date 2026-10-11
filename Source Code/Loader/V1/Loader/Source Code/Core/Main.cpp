#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include "Frontend/Frontend.hpp"
#include "Backend/Backend.hpp"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    Backend::EnsureDirectories();
    Backend::EnsureClickSound();

    Frontend frontend;
    if (!frontend.Initialize(hInstance)) {
        MessageBoxA(nullptr,
            "Failed to initialize the loader window / D3D11 device.",
            "Oblivion - Error", MB_ICONERROR);
        return 1;
    }

    frontend.Run();
    frontend.Shutdown();
    return 0;
}