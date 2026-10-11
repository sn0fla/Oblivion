#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <d3d11.h>
#include <string>
#include <cmath>
#include <algorithm>
#include <atomic>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include "../Backend/Backend.hpp"

enum class FrontendState {
    Splash,
    Checking,
    Main,
    Loading,
    Error,
    Done
};

enum class LoadStage {
    Connecting,
    CreatingDirectories,
    DownloadingFiles,
    LaunchingExe,
    Done
};

class Frontend {
public:
    Frontend() = default;
    ~Frontend() = default;

    bool Initialize(HINSTANCE hInstance);
    void Run();
    void Shutdown();

    void SetLoaderInfo(const LoaderInfo& info) { m_info = info; }
    void SetCheckResult(bool ok, bool versionOk) {
        m_checkSuccess.store(ok);
        m_versionOk.store(versionOk);
    }

private:
    HWND                     m_hwnd = nullptr;
    int                      m_winW = 480;
    int                      m_winH = 340;

    ID3D11Device* m_pd3dDevice = nullptr;
    ID3D11DeviceContext* m_pd3dDeviceContext = nullptr;
    IDXGISwapChain* m_pSwapChain = nullptr;
    ID3D11RenderTargetView* m_mainRenderTargetView = nullptr;
    bool                     m_swapChainOccluded = false;
    bool                     m_running = true;

    FrontendState            m_state = FrontendState::Splash;
    LoaderInfo               m_info;
    std::string              m_errorMsg;
    std::string              m_errorDetail;

    std::atomic<bool>        m_checkStarted{ false };
    std::atomic<bool>        m_checkComplete{ false };
    std::atomic<bool>        m_checkSuccess{ false };
    std::atomic<bool>        m_versionOk{ false };
    HANDLE                   m_hCheckThread = nullptr;

    float                    m_splashAlpha = 0.0f;
    bool                     m_splashFadingIn = true;
    bool                     m_splashFadingOut = false;
    float                    m_splashHoldTimer = 0.0f;
    float                    m_checkTimer = 0.0f;

    float                    m_btnHover = 0.0f;
    float                    m_exitHover = 0.0f;
    float                    m_okHover = 0.0f;
    float                    m_closeHover = 0.0f;

    LoadStage                m_loadStage = LoadStage::Connecting;
    float                    m_loadStageTimer = 0.0f;
    bool                     m_stageActionDone = false;
    float                    m_progress = 0.0f;
    float                    m_prepareTimer = 0.0f;

    ImFont* m_titleFont = nullptr;
    ImFont* m_bodyFont = nullptr;
    ImFont* m_smallFont = nullptr;
    ImFont* m_tinyFont = nullptr;

    bool  CreateDeviceD3D();
    void  CleanupDeviceD3D();
    void  CreateRenderTarget();
    void  CleanupRenderTarget();
    void  ApplyRoundRegion();

    void  RenderFrame();
    void  RenderTitleBar(ImDrawList* dl, ImVec2 wp, ImVec2 ds);
    void  RenderSplash(ImDrawList* dl, ImVec2 wp, ImVec2 ds);
    void  RenderChecking(ImDrawList* dl, ImVec2 wp, ImVec2 ds);
    void  RenderMain(ImDrawList* dl, ImVec2 wp, ImVec2 ds);
    void  RenderLoading(ImDrawList* dl, ImVec2 wp, ImVec2 ds);
    void  RenderError(ImDrawList* dl, ImVec2 wp, ImVec2 ds);
    void  UpdateAnimations(float dt);

    static DWORD WINAPI CheckThreadFunc(LPVOID param);
};