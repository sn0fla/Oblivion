#include "Overlay.hpp"

#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <avrt.h>
#include <cstdio>
#include <atomic>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "avrt.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern double FrameTime();
extern double GetLastSnapshotPublishTime();
extern std::atomic<double> g_overlayPresentLatencyMs;
extern std::atomic<double> g_overlayFrameTimeMs;

static constexpr wchar_t kClassName[] = L"ExternalOverlayWnd";

bool Overlay::Fail(const char* what, HRESULT hr)
{
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s (hr=0x%08X)", what, static_cast<unsigned>(hr));
    lastError_ = buf;
    return false;
}

LRESULT CALLBACK Overlay::WndProcStatic(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(l);
        SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }

    auto* self = reinterpret_cast<Overlay*>(GetWindowLongPtrW(h, GWLP_USERDATA));

    if (self && self->imguiReady_ && ImGui_ImplWin32_WndProcHandler(h, m, w, l))
        return TRUE;

    switch (m)
    {
    case WM_MOUSEACTIVATE:
        return (self && self->interactive_) ? MA_ACTIVATE : MA_NOACTIVATE;
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

bool Overlay::CreateOverlayWindow()
{
    hInst_ = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProcStatic;
    wc.hInstance = hInst_;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kClassName;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return Fail("RegisterClassExW failed", HRESULT_FROM_WIN32(GetLastError()));
    classReg_ = true;

    RECT rc{};
    bool vis = true;
    QueryTarget(rc, vis);
    width_ = static_cast<UINT>(rc.right - rc.left);
    height_ = static_cast<UINT>(rc.bottom - rc.top);
    curRect_ = rc;

    const DWORD ex = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE |
        WS_EX_TOOLWINDOW | WS_EX_NOREDIRECTIONBITMAP;

    hwnd_ = CreateWindowExW(ex, kClassName, cfg_.windowTitle.c_str(), WS_POPUP,
        rc.left, rc.top, static_cast<int>(width_), static_cast<int>(height_),
        nullptr, nullptr, hInst_, this);
    if (!hwnd_)
        return Fail("CreateWindowExW failed", HRESULT_FROM_WIN32(GetLastError()));

    SetLayeredWindowAttributes(hwnd_, 0, 255, LWA_ALPHA);
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd_);
    visible_ = true;
    return true;
}

bool Overlay::CreateDevice()
{
    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL got{};

    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
        &device_, &got, &ctx_);
    if (FAILED(hr))
        return Fail("D3D11CreateDevice failed", hr);

    ComPtr<IDXGIDevice1> dxgiDev;
    if (SUCCEEDED(device_.As(&dxgiDev)))
    {
        dxgiDev->SetMaximumFrameLatency(1);
        dxgiDev->SetGPUThreadPriority(7);
    }
    return true;
}

bool Overlay::CreateRenderTarget()
{
    ComPtr<ID3D11Texture2D> back;
    HRESULT hr = swapchain_->GetBuffer(0, IID_PPV_ARGS(&back));
    if (FAILED(hr))
        return Fail("GetBuffer failed", hr);

    hr = device_->CreateRenderTargetView(back.Get(), nullptr, &rtv_);
    if (FAILED(hr))
        return Fail("CreateRenderTargetView failed", hr);
    return true;
}

bool Overlay::CreateSwapchainAndComposition()
{
    ComPtr<IDXGIDevice> dxgiDev;
    HRESULT hr = device_.As(&dxgiDev);
    if (FAILED(hr)) return Fail("QueryInterface(IDXGIDevice) failed", hr);

    ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDev->GetAdapter(&adapter);
    if (FAILED(hr)) return Fail("GetAdapter failed", hr);

    ComPtr<IDXGIFactory2> factory;
    hr = adapter->GetParent(IID_PPV_ARGS(&factory));
    if (FAILED(hr)) return Fail("GetParent(IDXGIFactory2) failed", hr);

    DXGI_SWAP_CHAIN_DESC1 sd{};
    sd.Width = width_;
    sd.Height = height_;
    sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.SampleDesc = { 1, 0 };
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.Scaling = DXGI_SCALING_STRETCH;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    ComPtr<IDXGISwapChain1> sc1;
    hr = factory->CreateSwapChainForComposition(device_.Get(), &sd, nullptr, &sc1);
    if (FAILED(hr)) return Fail("CreateSwapChainForComposition failed", hr);

    hr = sc1.As(&swapchain_);
    if (FAILED(hr)) return Fail("QueryInterface(IDXGISwapChain2) failed", hr);

    swapchain_->SetMaximumFrameLatency(1);
    waitable_ = swapchain_->GetFrameLatencyWaitableObject();

    hr = DCompositionCreateDevice(dxgiDev.Get(), IID_PPV_ARGS(&dcompDevice_));
    if (FAILED(hr)) return Fail("DCompositionCreateDevice failed", hr);

    hr = dcompDevice_->CreateTargetForHwnd(hwnd_, TRUE, &dcompTarget_);
    if (FAILED(hr)) return Fail("CreateTargetForHwnd failed", hr);

    hr = dcompDevice_->CreateVisual(&dcompVisual_);
    if (FAILED(hr)) return Fail("CreateVisual failed", hr);

    dcompVisual_->SetContent(swapchain_.Get());
    dcompTarget_->SetRoot(dcompVisual_.Get());
    hr = dcompDevice_->Commit();
    if (FAILED(hr)) return Fail("DComp Commit failed", hr);

    return CreateRenderTarget();
}

bool Overlay::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    ImGui::StyleColorsDark();

    if (!cfg_.fontPath.empty())
        io.Fonts->AddFontFromFileTTF(cfg_.fontPath.c_str(), cfg_.fontSize);

    if (!ImGui_ImplWin32_Init(hwnd_))
        return Fail("ImGui_ImplWin32_Init failed");
    if (!ImGui_ImplDX11_Init(device_.Get(), ctx_.Get()))
        return Fail("ImGui_ImplDX11_Init failed");

    imguiReady_ = true;
    return true;
}

bool Overlay::Create(const Config& cfg)
{
    cfg_ = cfg;

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    if (!CreateOverlayWindow())          return false;
    if (!CreateDevice())                 return false;
    if (!CreateSwapchainAndComposition()) return false;
    if (!InitImGui())                    return false;

    running_ = true;
    return true;
}

void Overlay::Shutdown()
{
    if (imguiReady_)
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        imguiReady_ = false;
    }

    if (ctx_) ctx_->ClearState();

    rtv_.Reset();
    dcompVisual_.Reset();
    dcompTarget_.Reset();
    dcompDevice_.Reset();
    swapchain_.Reset();
    waitable_ = nullptr;
    ctx_.Reset();
    device_.Reset();

    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
    if (classReg_) { UnregisterClassW(kClassName, hInst_); classReg_ = false; }
    running_ = false;
}

void Overlay::SetInteractive(bool on)
{
    if (!hwnd_ || on == interactive_)
        return;
    interactive_ = on;

    LONG_PTR ex = GetWindowLongPtrW(hwnd_, GWL_EXSTYLE);
    if (on) ex &= ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
    else    ex |= static_cast<LONG_PTR>(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
    SetWindowLongPtrW(hwnd_, GWL_EXSTYLE, ex);
    SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    ImGuiIO& io = ImGui::GetIO();

    if (on)
    {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

        const HWND fg = GetForegroundWindow();
        const DWORD fgThread = fg ? GetWindowThreadProcessId(fg, nullptr) : 0;
        const DWORD myThread = GetCurrentThreadId();
        const bool attached = (fgThread != 0 && fgThread != myThread);

        if (attached)
            AttachThreadInput(fgThread, myThread, TRUE);

        BringWindowToTop(hwnd_);
        SetForegroundWindow(hwnd_);
        SetFocus(hwnd_);

        if (attached)
            AttachThreadInput(fgThread, myThread, FALSE);

        SetActiveWindow(hwnd_);
    }
    else
    {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;

        if (target_ && IsWindow(target_))
        {
            DWORD fgThread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
            DWORD myThread = GetCurrentThreadId();

            if (fgThread != myThread)
                AttachThreadInput(fgThread, myThread, TRUE);

            SetForegroundWindow(target_);
            SetFocus(target_);

            if (fgThread != myThread)
                AttachThreadInput(fgThread, myThread, FALSE);

            ShowCursor(TRUE);
            ClipCursor(nullptr);
        }
    }
}

void Overlay::QueryTarget(RECT& out, bool& visible)
{
    visible = true;

    if (cfg_.targetTitle.empty())
    {
        out = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
        return;
    }

    if (!target_ || !IsWindow(target_))
    {
        target_ = nullptr;
        const ULONGLONG now = GetTickCount64();
        if (now - lastFindTick_ > 250)
        {
            lastFindTick_ = now;
            target_ = FindWindowW(nullptr, cfg_.targetTitle.c_str());
        }
    }

    if (!target_)
    {
        out = curRect_.right > curRect_.left ? curRect_
            : RECT{ 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
        visible = false;
        return;
    }

    if (IsIconic(target_))
    {
        out = curRect_;
        visible = false;
        return;
    }

    RECT client{};
    GetClientRect(target_, &client);
    POINT tl{ 0, 0 };
    ClientToScreen(target_, &tl);

    out.left = tl.x;
    out.top = tl.y;
    out.right = tl.x + (client.right - client.left);
    out.bottom = tl.y + (client.bottom - client.top);

    if (cfg_.hideWhenTargetInactive)
    {
        const HWND fg = GetForegroundWindow();
        visible = (fg == target_ || fg == hwnd_);
    }
}

void Overlay::ResizeSwapchain(UINT w, UINT h)
{
    if (!swapchain_) return;

    ctx_->OMSetRenderTargets(0, nullptr, nullptr);
    rtv_.Reset();

    HRESULT hr = swapchain_->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN,
        DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
    if (FAILED(hr)) { Fail("ResizeBuffers failed", hr); running_ = false; return; }

    width_ = w;
    height_ = h;
    if (!CreateRenderTarget()) running_ = false;
}

void Overlay::ApplyGeometry(const RECT& rcIn, bool visible)
{
    RECT rc = rcIn;
    if (rc.right <= rc.left) rc.right = rc.left + 1;
    if (rc.bottom <= rc.top)  rc.bottom = rc.top + 1;

    const UINT w = static_cast<UINT>(rc.right - rc.left);
    const UINT h = static_cast<UINT>(rc.bottom - rc.top);

    if (visible != visible_)
    {
        visible_ = visible;
        ShowWindow(hwnd_, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    }

    const bool moved = rc.left != curRect_.left || rc.top != curRect_.top;
    const bool sized = w != width_ || h != height_;

    if (moved || sized)
    {
        SetWindowPos(hwnd_, HWND_TOPMOST, rc.left, rc.top, static_cast<int>(w), static_cast<int>(h),
            SWP_NOACTIVATE);
        if (sized) ResizeSwapchain(w, h);
        curRect_ = rc;
    }

    if (visible_)
    {
        const ULONGLONG now = GetTickCount64();
        if (now - lastTopTick_ > 1000)
        {
            lastTopTick_ = now;
            SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }
}

void Overlay::PollHotkeys()
{
    const bool interact = (GetAsyncKeyState(cfg_.interactKey) & 0x8000) != 0;
    const bool quit = (GetAsyncKeyState(cfg_.exitKey) & 0x8000) != 0;

    if (interact && !prevInteractKey_) SetInteractive(!interactive_);
    if (quit && !prevExitKey_)         running_ = false;

    prevInteractKey_ = interact;
    prevExitKey_ = quit;
}

void Overlay::Run(const RenderFn& render)
{
    if (!running_ || !hwnd_) return;

    DWORD mmcssTask = 0;
    HANDLE mmcss = AvSetMmThreadCharacteristicsW(L"Games", &mmcssTask);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    static const float kClear[4] = { 0.f, 0.f, 0.f, 0.f };
    const bool hasWaitable = (waitable_ != nullptr);

    LARGE_INTEGER perfFreq;
    QueryPerformanceFrequency(&perfFreq);

    while (running_)
    {
        LARGE_INTEGER frameStart;
        QueryPerformanceCounter(&frameStart);

        if (hasWaitable)
        {
            WaitForSingleObject(waitable_, 100);
        }
        else if (!visible_)
        {
            Sleep(8);
        }

        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) running_ = false;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!running_) break;

        PollHotkeys();

        RECT rc{};
        bool vis = true;
        QueryTarget(rc, vis);
        ApplyGeometry(rc, vis);
        if (!running_) break;
        if (!visible_) continue;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (render) render(*this);

        ImGui::Render();

        ID3D11RenderTargetView* rtv = rtv_.Get();
        ctx_->OMSetRenderTargets(1, &rtv, nullptr);
        ctx_->ClearRenderTargetView(rtv, kClear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        const HRESULT hr = swapchain_->Present(cfg_.vsync ? 1 : 0, 0);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            Fail("GPU device lost", hr);
            running_ = false;
            break;
        }

        {
            LARGE_INTEGER presentEnd;
            QueryPerformanceCounter(&presentEnd);

            const double presentEndSec =
                static_cast<double>(presentEnd.QuadPart) /
                static_cast<double>(perfFreq.QuadPart);

            const double frameStartSec =
                static_cast<double>(frameStart.QuadPart) /
                static_cast<double>(perfFreq.QuadPart);

            g_overlayFrameTimeMs.store(
                (presentEndSec - frameStartSec) * 1000.0,
                std::memory_order_relaxed);

            const double pub = GetLastSnapshotPublishTime();
            if (pub > 0.0)
            {
                g_overlayPresentLatencyMs.store(
                    (presentEndSec - pub) * 1000.0,
                    std::memory_order_relaxed);
            }
        }
    }

    if (mmcss) AvRevertMmThreadCharacteristics(mmcss);
}