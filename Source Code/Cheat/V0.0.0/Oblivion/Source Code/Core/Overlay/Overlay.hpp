#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_5.h>
#include <dcomp.h>
#include <wrl/client.h>

#include <functional>
#include <string>
#include <cstdint>

#include "imgui.h"

class Overlay
{
public:
    struct Config
    {
        std::wstring windowTitle = L"Overlay";
        std::wstring targetTitle;
        bool hideWhenTargetInactive = true;
        bool vsync = true;
        bool allow_tearing = true;
        float targetFps = 360.0f;
        int interactKey = VK_INSERT;
        int exitKey = VK_END;
        std::string fontPath;
        float fontSize = 16.0f;
    };

    using RenderFn = std::function<void(Overlay&)>;

    Overlay() = default;
    ~Overlay() { Shutdown(); }
    Overlay(const Overlay&) = delete;
    Overlay& operator=(const Overlay&) = delete;

    static bool HasUIAccess();
    static bool TryElevateToUIAccess();

    bool Create(const Config& cfg);
    void Run(const RenderFn& render);
    void Shutdown();

    void RequestExit() { running_ = false; }
    void SetInteractive(bool on);
    void SetLastPublishSec(double s) { lastPublishSec_ = s; }
    void SetVsync(bool on) { cfg_.vsync = on; }
    void SetAllowTearing(bool on) { cfg_.allow_tearing = on; }

    bool               IsInteractive() const { return interactive_; }
    HWND               Hwnd()          const { return hwnd_; }
    ImVec2             Size()          const { return ImVec2(static_cast<float>(width_), static_cast<float>(height_)); }
    const std::string& LastError()     const { return lastError_; }
    UINT               MonitorHz()     const { return monitorHz_; }

    ID3D11Device* Device()  const { return device_.Get(); }
    ID3D11DeviceContext* Context() const { return ctx_.Get(); }

private:
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    bool CreateOverlayWindow();
    bool CreateDevice(LUID* preferLuid = nullptr);
    bool InitSwapChainPath();
    bool CreateSwapChain();
    bool CreateComposition();
    bool CreateSwapChainTarget();
    bool InitImGui();
    void Warmup();
    bool Fail(const char* what, HRESULT hr = S_OK);

    void ReleaseTargets();
    void ResetDevice();

    UINT GetMonitorRefreshRate() const;
    DWORD GetSwapchainWaitMs() const;
    void RefreshMonitorHz();

    void QueryTarget(RECT& outRect, bool& outVisible);
    void ApplyGeometry(const RECT& rc, bool visible);
    void ResizeSwapchain(UINT w, UINT h);
    void PollHotkeys();
    void PumpInput();

    static LRESULT CALLBACK WndProcStatic(HWND h, UINT m, WPARAM w, LPARAM l);

private:
    Config cfg_{};

    HWND      hwnd_ = nullptr;
    HWND      target_ = nullptr;
    HINSTANCE hInst_ = nullptr;
    bool      classReg_ = false;

    ComPtr<ID3D11Device>           device_;
    ComPtr<ID3D11DeviceContext>    ctx_;

    ComPtr<IDXGISwapChain1>        swapChain_;
    ComPtr<IDXGISwapChain2>        swapChain2_;
    ComPtr<IDCompositionDevice>    dcompDevice_;
    ComPtr<IDCompositionTarget>    dcompTarget_;
    ComPtr<IDCompositionVisual>    dcompVisual_;

    ComPtr<ID3D11Texture2D>        targetTex_;
    ComPtr<IDXGISurface>           targetSurface_;
    ComPtr<ID3D11RenderTargetView> rtv_;

    bool   hwTearingOk_ = false;
    HANDLE waitable_ = nullptr;

    UINT width_ = 1;
    UINT height_ = 1;
    RECT curRect_{};

    bool imguiReady_ = false;
    bool running_ = false;
    bool visible_ = true;
    bool interactive_ = false;

    ULONGLONG lastFindTick_ = 0;
    ULONGLONG lastTopTick_ = 0;
    bool      prevInteractKey_ = false;
    bool      prevExitKey_ = false;

    bool prevKeys_[256] = {};
    bool prevMouse_[5] = {};

    UINT      monitorHz_ = 60;
    ULONGLONG lastHzPollTick_ = 0;

    double lastPublishSec_ = 0.0;

    std::string lastError_;
};