#include "Overlay.hpp"

#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "../../Utilities/Logging/Logging.hpp"

#include <avrt.h>
#include <dwmapi.h>
#include <cstdio>
#include <cstdint>
#include <atomic>
#include <intrin.h>
#include <thread>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "winmm.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static constexpr wchar_t kClassName[] = L"ExternalOverlayWnd";

typedef HRESULT(WINAPI* DwmGetDxSharedSurfaceFn)(
    HWND hWnd, HANDLE* phSurface, LUID* pAdapterLuid,
    UINT* pFmtWindow, UINT* pPresentFlags, ULONGLONG* pWin32kUpdateId);

typedef HRESULT(WINAPI* DCompositionWaitForCompositorClockFn)(
    UINT count, const HANDLE* pHandles, DWORD dwTimeoutInMs);

static int64_t Qpc()
{
    LARGE_INTEGER v;
    QueryPerformanceCounter(&v);
    return v.QuadPart;
}

static DwmGetDxSharedSurfaceFn GetDwmGetDxSharedSurface()
{
    static DwmGetDxSharedSurfaceFn fn = nullptr;
    static bool tried = false;
    if (!tried)
    {
        tried = true;
        HMODULE h = GetModuleHandleW(L"dwmapi.dll");
        if (!h)
            h = LoadLibraryW(L"dwmapi.dll");
        if (h)
            fn = reinterpret_cast<DwmGetDxSharedSurfaceFn>(
                GetProcAddress(h, "DwmGetDxSharedSurface"));
    }
    return fn;
}

static DCompositionWaitForCompositorClockFn GetWaitForCompositorClock()
{
    static DCompositionWaitForCompositorClockFn fn = nullptr;
    static bool tried = false;
    if (!tried)
    {
        tried = true;
        HMODULE h = GetModuleHandleW(L"dcomp.dll");
        if (!h)
            h = LoadLibraryW(L"dcomp.dll");
        if (h)
            fn = reinterpret_cast<DCompositionWaitForCompositorClockFn>(
                GetProcAddress(h, "DCompositionWaitForCompositorClock"));
    }
    return fn;
}

static bool GetAdapterLuid(ID3D11Device* dev, LUID& out)
{
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    if (FAILED(dev->QueryInterface(IID_PPV_ARGS(&dxgiDevice))))
        return false;

    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgiDevice->GetAdapter(&adapter)))
        return false;

    DXGI_ADAPTER_DESC desc{};
    if (FAILED(adapter->GetDesc(&desc)))
        return false;

    out = desc.AdapterLuid;
    return true;
}

bool Overlay::Fail(const char* what, HRESULT hr)
{
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s (hr=0x%08X)", what, static_cast<unsigned>(hr));
    lastError_ = buf;
    Logging::Error("Overlay: %s", buf);
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

UINT Overlay::GetMonitorRefreshRate() const
{
    HMONITOR mon = nullptr;
    if (hwnd_ && IsWindow(hwnd_))
        mon = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTOPRIMARY);
    else if (target_ && IsWindow(target_))
        mon = MonitorFromWindow(target_, MONITOR_DEFAULTTOPRIMARY);
    else
        mon = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);

    MONITORINFOEXW mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(mon, &mi))
        return 60;

    DEVMODEW dm{};
    dm.dmSize = sizeof(dm);
    if (!EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm))
        return 60;

    if (dm.dmDisplayFrequency <= 1)
        return 60;

    return static_cast<UINT>(dm.dmDisplayFrequency);
}

void Overlay::RefreshMonitorHz()
{
    const ULONGLONG now = GetTickCount64();
    if (lastHzPollTick_ != 0 && now - lastHzPollTick_ < 1000)
        return;
    lastHzPollTick_ = now;

    const UINT hz = GetMonitorRefreshRate();
    if (hz == 0 || hz == monitorHz_)
        return;

    monitorHz_ = hz;
    cfg_.targetFps = static_cast<float>(hz);
    Logging::Info("Overlay: monitorHz=%u", hz);
}

DWORD Overlay::GetSwapchainWaitMs() const
{
    UINT hz = monitorHz_ ? monitorHz_ : GetMonitorRefreshRate();
    if (hz < 30) hz = 60;
    const float periodMs = 1000.0f / static_cast<float>(hz);
    DWORD ms = static_cast<DWORD>(periodMs * 2.0f + 1.0f);
    if (ms < 4) ms = 4;
    return ms;
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

    const DWORD ex = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT |
        WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;

    hwnd_ = CreateWindowExW(ex, kClassName, cfg_.windowTitle.c_str(), WS_POPUP,
        rc.left, rc.top, static_cast<int>(width_), static_cast<int>(height_),
        nullptr, nullptr, hInst_, this);
    if (!hwnd_)
        return Fail("CreateWindowExW failed", HRESULT_FROM_WIN32(GetLastError()));

    SetLayeredWindowAttributes(hwnd_, 0, 0, LWA_ALPHA);
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd_);
    visible_ = true;
    return true;
}

bool Overlay::CreateDevice(LUID* preferLuid)
{
    ComPtr<IDXGIAdapter1> adapter;
    if (preferLuid)
    {
        ComPtr<IDXGIFactory1> factory;
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
            return Fail("CreateDXGIFactory1 failed");

        for (UINT i = 0;; ++i)
        {
            ComPtr<IDXGIAdapter1> a;
            if (factory->EnumAdapters1(i, &a) == DXGI_ERROR_NOT_FOUND)
                break;

            DXGI_ADAPTER_DESC1 desc{};
            a->GetDesc1(&desc);
            if (desc.AdapterLuid.LowPart == preferLuid->LowPart &&
                desc.AdapterLuid.HighPart == preferLuid->HighPart)
            {
                adapter = a;
                break;
            }
        }
        if (!adapter)
            return Fail("Requested adapter not found");
    }

    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL got{};

    HRESULT hr = D3D11CreateDevice(
        adapter.Get(),
        adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
        &device_, &got, &ctx_);
    if (FAILED(hr))
        return Fail("D3D11CreateDevice failed", hr);

    ComPtr<IDXGIDevice1> dxgiDev;
    if (SUCCEEDED(device_.As(&dxgiDev)))
    {
        dxgiDev->SetMaximumFrameLatency(1);
    }
    return true;
}

bool Overlay::CreateSwapChain()
{
    ComPtr<IDXGIDevice> dxgiDev;
    if (FAILED(device_.As(&dxgiDev)))
        return Fail("QueryInterface(IDXGIDevice) failed");

    ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgiDev->GetAdapter(&adapter)))
        return Fail("GetAdapter failed");

    ComPtr<IDXGIFactory2> factory;
    if (FAILED(adapter->GetParent(IID_PPV_ARGS(&factory))))
        return Fail("GetParent(IDXGIFactory2) failed");

    bool tearingOk = false;
    {
        ComPtr<IDXGIFactory5> f5;
        if (SUCCEEDED(factory.As(&f5)))
        {
            BOOL allow = FALSE;
            if (SUCCEEDED(f5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow, sizeof(allow))))
                tearingOk = (allow != FALSE);
        }
    }

    allowTearing_ = tearingOk && !cfg_.vsync;

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

    if (allowTearing_)
    {
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        if (FAILED(factory->CreateSwapChainForComposition(device_.Get(), &sd, nullptr, &swapChain_)))
        {
            allowTearing_ = false;
            swapChain_.Reset();
        }
    }

    if (!swapChain_)
    {
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        if (FAILED(factory->CreateSwapChainForComposition(device_.Get(), &sd, nullptr, &swapChain_)))
            return Fail("CreateSwapChainForComposition failed");
    }

    if (SUCCEEDED(swapChain_.As(&swapChain2_)))
        swapChain2_->SetMaximumFrameLatency(1);

    if (!allowTearing_)
    {
        if (!swapChain2_)
            return Fail("QueryInterface(IDXGISwapChain2) failed");
        waitable_ = swapChain2_->GetFrameLatencyWaitableObject();
        if (!waitable_)
            return Fail("GetFrameLatencyWaitableObject failed");
    }

    return true;
}

bool Overlay::CreateComposition()
{
    ComPtr<IDXGIDevice> dxgiDev;
    if (FAILED(device_.As(&dxgiDev)))
        return Fail("QueryInterface(IDXGIDevice) failed");

    if (FAILED(DCompositionCreateDevice(dxgiDev.Get(), IID_PPV_ARGS(&dcompDevice_))))
        return Fail("DCompositionCreateDevice failed");
    if (FAILED(dcompDevice_->CreateTargetForHwnd(hwnd_, TRUE, &dcompTarget_)))
        return Fail("CreateTargetForHwnd failed");
    if (FAILED(dcompDevice_->CreateVisual(&dcompVisual_)))
        return Fail("CreateVisual failed");
    if (FAILED(dcompVisual_->SetContent(swapChain_.Get())))
        return Fail("SetContent failed");
    if (FAILED(dcompTarget_->SetRoot(dcompVisual_.Get())))
        return Fail("SetRoot failed");
    if (FAILED(dcompDevice_->Commit()))
        return Fail("DComp Commit failed");
    return true;
}

bool Overlay::CreateSwapChainTarget()
{
    ComPtr<ID3D11Texture2D> back;
    if (FAILED(swapChain_->GetBuffer(0, IID_PPV_ARGS(&back))))
        return Fail("GetBuffer failed");

    if (FAILED(device_->CreateRenderTargetView(back.Get(), nullptr, &rtv_)))
        return Fail("CreateRenderTargetView failed");

    targetTex_ = back;
    if (FAILED(targetTex_.As(&targetSurface_)))
        return Fail("QueryInterface(IDXGISurface) failed");
    return true;
}

bool Overlay::CreateDwmTarget()
{
    ComPtr<ID3D11Texture2D> sharedTex;
    if (FAILED(device_->OpenSharedResource(reinterpret_cast<HANDLE>(targetTex_.Get()),
        IID_PPV_ARGS(&sharedTex))))
    {
        return false;
    }

    if (FAILED(device_->CreateRenderTargetView(sharedTex.Get(), nullptr, &rtv_)))
        return Fail("CreateRenderTargetView failed");

    targetTex_ = sharedTex;
    if (FAILED(targetTex_.As(&targetSurface_)))
        return Fail("QueryInterface(IDXGISurface) failed");
    return true;
}

bool Overlay::TryInitDwm()
{
    auto fn = GetDwmGetDxSharedSurface();
    if (!fn)
        return false;

    HANDLE shared = nullptr;
    LUID luid{};
    UINT fmt = 0;
    UINT flags = 0;
    ULONGLONG updateId = 0;

    if (FAILED(fn(hwnd_, &shared, &luid, &fmt, &flags, &updateId)) || !shared)
        return false;

    if (!CreateDevice(&luid))
    {
        CloseHandle(shared);
        return false;
    }

    if (FAILED(device_->OpenSharedResource(shared, IID_PPV_ARGS(&targetTex_))))
    {
        CloseHandle(shared);
        return false;
    }
    CloseHandle(shared);

    if (FAILED(targetTex_.As(&targetSurface_)))
        return false;

    if (FAILED(device_->CreateRenderTargetView(targetTex_.Get(), nullptr, &rtv_)))
        return false;

    useDwm_ = true;
    return true;
}

bool Overlay::InitSwapChainPath()
{
    if (!CreateDevice(nullptr))
        return false;
    if (!CreateSwapChain())
        return false;
    if (!CreateComposition())
        return false;
    if (!CreateSwapChainTarget())
        return false;
    return true;
}

bool Overlay::ReacquireDwm()
{
    if (!useDwm_)
        return false;

    rtv_.Reset();
    targetSurface_.Reset();
    targetTex_.Reset();

    auto fn = GetDwmGetDxSharedSurface();
    if (!fn)
        return false;

    HANDLE shared = nullptr;
    LUID luid{};
    UINT fmt = 0;
    UINT flags = 0;
    ULONGLONG updateId = 0;

    if (FAILED(fn(hwnd_, &shared, &luid, &fmt, &flags, &updateId)) || !shared)
        return false;

    LUID mine{};
    if (!GetAdapterLuid(device_.Get(), mine) ||
        mine.LowPart != luid.LowPart || mine.HighPart != luid.HighPart)
    {
        CloseHandle(shared);
        return false;
    }

    if (FAILED(device_->OpenSharedResource(shared, IID_PPV_ARGS(&targetTex_))))
    {
        CloseHandle(shared);
        return false;
    }
    CloseHandle(shared);

    if (FAILED(targetTex_.As(&targetSurface_)))
        return false;

    if (FAILED(device_->CreateRenderTargetView(targetTex_.Get(), nullptr, &rtv_)))
        return false;

    return true;
}

void Overlay::ReleaseTargets()
{
    rtv_.Reset();
    targetSurface_.Reset();
    targetTex_.Reset();
}

void Overlay::ResetDevice()
{
    ReleaseTargets();
    dcompVisual_.Reset();
    dcompTarget_.Reset();
    dcompDevice_.Reset();
    swapChain2_.Reset();
    swapChain_.Reset();
    ctx_.Reset();
    device_.Reset();

    waitable_ = nullptr;
    allowTearing_ = false;
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

void Overlay::Warmup()
{
    if (!hwnd_)
        return;

    SetLayeredWindowAttributes(hwnd_, 0, 0, LWA_ALPHA);

    for (int i = 0; i < 3; ++i)
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::Render();

        if (rtv_ && ctx_)
        {
            ID3D11RenderTargetView* rtv = rtv_.Get();
            ctx_->OMSetRenderTargets(1, &rtv, nullptr);
            const float clear[4] = { 0.f, 0.f, 0.f, 0.f };
            ctx_->ClearRenderTargetView(rtv, clear);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            if (swapChain_)
            {
                UINT pf = 0;
                UINT sync = 1;
                if (waitable_) { sync = 0; }
                else if (allowTearing_ && !cfg_.vsync)
                {
                    sync = 0;
                    pf = DXGI_PRESENT_ALLOW_TEARING | DXGI_PRESENT_DO_NOT_WAIT;
                }
                swapChain_->Present(sync, pf);
            }
            else if (useDwm_)
            {
                DwmFlush();
            }
        }

        if (waitable_)
            WaitForSingleObject(waitable_, 50);
    }

    SetLayeredWindowAttributes(hwnd_, 0, 255, LWA_ALPHA);
}

bool Overlay::Create(const Config& cfg)
{
    cfg_ = cfg;

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    if (!CreateOverlayWindow())
        return false;

    bool ok = TryInitDwm();
    if (!ok)
    {
        ResetDevice();
        ok = InitSwapChainPath();
    }
    if (!ok)
        return false;

    if (!InitImGui())
        return false;

    Warmup();

    monitorHz_ = 0;
    lastHzPollTick_ = 0;
    RefreshMonitorHz();

    Logging::Info("Overlay: init ok (DWM=%d, monitorHz=%u, tearing=%d, waitable=%d)",
        useDwm_ ? 1 : 0, monitorHz_, allowTearing_ ? 1 : 0, waitable_ ? 1 : 0);

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

    ResetDevice();
    useDwm_ = false;
    waitable_ = nullptr;

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
    if (!swapChain_ || !ctx_)
        return;

    ctx_->OMSetRenderTargets(0, nullptr, nullptr);
    rtv_.Reset();

    const UINT flags = allowTearing_
        ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
        : DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    HRESULT hr = swapChain_->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, flags);
    if (FAILED(hr)) { Fail("ResizeBuffers failed", hr); running_ = false; return; }

    width_ = w;
    height_ = h;
    if (!CreateSwapChainTarget()) running_ = false;

    if (!allowTearing_ && swapChain2_)
        waitable_ = swapChain2_->GetFrameLatencyWaitableObject();
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

        if (sized)
        {
            if (useDwm_)
                ReacquireDwm();
            else
                ResizeSwapchain(w, h);
        }
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
    const bool quit = (cfg_.exitKey != 0) && ((GetAsyncKeyState(cfg_.exitKey) & 0x8000) != 0);

    if (interact && !prevInteractKey_) SetInteractive(!interactive_);
    if (quit && !prevExitKey_)         running_ = false;

    prevInteractKey_ = interact;
    prevExitKey_ = quit;
}

void Overlay::Run(const RenderFn& render)
{
    if (!running_ || !hwnd_)
        return;

    DWORD mmcssTask = 0;
    HANDLE mmcss = AvSetMmThreadCharacteristicsW(L"Games", &mmcssTask);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    const unsigned cores = std::thread::hardware_concurrency();
    if (cores >= 4)
    {
        SetThreadAffinityMask(GetCurrentThread(), static_cast<DWORD_PTR>(0x3));
    }

    static const float kClear[4] = { 0.f, 0.f, 0.f, 0.f };

    LARGE_INTEGER perfFreq;
    QueryPerformanceFrequency(&perfFreq);

    Logging::Info("Overlay: Run loop entered (DWM=%d, tearing=%d, waitable=%d, monitorHz=%u)",
        useDwm_ ? 1 : 0, allowTearing_ ? 1 : 0, waitable_ ? 1 : 0, monitorHz_);

    while (running_)
    {
        RefreshMonitorHz();

        LARGE_INTEGER frameStart;
        QueryPerformanceCounter(&frameStart);

        if (useDwm_)
        {
            auto fn = GetWaitForCompositorClock();
            if (fn)
                fn(0, nullptr, 1000);
            else
                DwmFlush();
        }
        else if (waitable_)
        {
            uint32_t spins = 0;
            while (running_)
            {
                if (WaitForSingleObject(waitable_, 0) == WAIT_OBJECT_0)
                    break;
                YieldProcessor();
                if (++spins > 200000)
                {
                    if (WaitForSingleObject(waitable_, 1) == WAIT_OBJECT_0)
                        break;
                    spins = 0;
                }
            }
        }
        else if (allowTearing_)
        {
            YieldProcessor();
        }
        else if (!visible_)
        {
            Sleep(1);
        }
        else
        {
            YieldProcessor();
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

        if (!rtv_ || !ctx_)
            continue;

        ID3D11RenderTargetView* rtv = rtv_.Get();
        ctx_->OMSetRenderTargets(1, &rtv, nullptr);
        ctx_->ClearRenderTargetView(rtv, kClear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (useDwm_)
        {
            if (FAILED(DwmFlush()))
            {
                Logging::Warn("Overlay: DwmFlush failed, reacquiring");
                if (!ReacquireDwm())
                {
                    Logging::Error("Overlay: ReacquireDwm failed, stopping");
                    running_ = false;
                    break;
                }
            }
        }
        else if (swapChain_)
        {
            UINT syncInterval = 1;
            UINT pf = 0;
            if (waitable_)
            {
                syncInterval = 0;
            }
            else if (allowTearing_ && !cfg_.vsync)
            {
                syncInterval = 0;
                pf = DXGI_PRESENT_ALLOW_TEARING | DXGI_PRESENT_DO_NOT_WAIT;
            }

            const HRESULT hr = swapChain_->Present(syncInterval, pf);

            if (hr == DXGI_ERROR_WAS_STILL_DRAWING)
            {
            }
            else if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                Fail("GPU device lost", hr);
                running_ = false;
                break;
            }
            else if (FAILED(hr))
            {
                Fail("Present failed", hr);
                running_ = false;
                break;
            }
        }

        LARGE_INTEGER presentEnd;
        QueryPerformanceCounter(&presentEnd);

        const double freq = static_cast<double>(perfFreq.QuadPart);
        const double presentEndSec = static_cast<double>(presentEnd.QuadPart) / freq;
        const double frameStartSec = static_cast<double>(frameStart.QuadPart) / freq;

        Logging::Sample("overlay_frame_ms", (presentEndSec - frameStartSec) * 1000.0);

        if (lastPublishSec_ > 0.0)
            Logging::Sample("present_latency_ms", (presentEndSec - lastPublishSec_) * 1000.0);

        if (useDwm_) {
            DWM_TIMING_INFO ti{};
            ti.cbSize = sizeof(ti);
            if (SUCCEEDED(DwmGetCompositionTimingInfo(nullptr, &ti)) && ti.qpcVBlank > 0) {
                const int64_t nowQpc = Qpc();
                const int64_t vblankQpc = static_cast<int64_t>(ti.qpcVBlank);
                const int64_t periodQpc = static_cast<int64_t>(ti.qpcRefreshPeriod);
                if (periodQpc > 0) {
                    const int64_t delta = nowQpc - vblankQpc;
                    const int64_t n = (delta >= 0) ? (delta / periodQpc + 1) : -((-delta) / periodQpc);
                    const int64_t nextVBlank = vblankQpc + n * periodQpc;
                    Logging::Sample("scanout_ms", static_cast<double>(nextVBlank - nowQpc) * 1000.0 / freq);
                }
                Logging::Sample("present_queue", static_cast<double>(ti.cFramesPending));
            }
        }

        const int64_t minPeriodTicks = perfFreq.QuadPart / 1000;
        const int64_t deadline = frameStart.QuadPart + minPeriodTicks;
        uint32_t spinAccum = 0;
        while (running_)
        {
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            if (now.QuadPart >= deadline) break;

            if (++spinAccum >= 50)
            {
                SwitchToThread();
                spinAccum = 0;
            }
            else
            {
                YieldProcessor();
            }
        }
    }

    Logging::Info("Overlay: Run loop exited");

    if (mmcss) AvRevertMmThreadCharacteristics(mmcss);
}