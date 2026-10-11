#include "Frontend.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static const ImU32 CLR_BG = IM_COL32(19, 19, 23, 255);
static const ImU32 CLR_HEADER_BG = IM_COL32(18, 18, 24, 240);
static const ImU32 CLR_CARD_BG = IM_COL32(26, 26, 34, 235);
static const ImU32 CLR_CARD_HDR = IM_COL32(30, 30, 38, 240);
static const ImU32 CLR_BORDER = IM_COL32(189, 176, 255, 70);
static const ImU32 CLR_BORDER_SOFT = IM_COL32(189, 176, 255, 40);
static const ImU32 CLR_ACCENT = IM_COL32(189, 176, 255, 255);
static const ImU32 CLR_ACCENT_DARK = IM_COL32(120, 105, 200, 255);
static const ImU32 CLR_TEXT_MAIN = IM_COL32(235, 228, 255, 255);
static const ImU32 CLR_TEXT_DIM = IM_COL32(200, 195, 235, 255);
static const ImU32 CLR_TEXT_MUTED = IM_COL32(150, 155, 185, 255);
static const ImU32 CLR_TEXT_LABEL = IM_COL32(160, 155, 185, 255);

static const ImU32 CLR_GREEN = IM_COL32(120, 220, 140, 255);
static const ImU32 CLR_YELLOW = IM_COL32(230, 210, 120, 255);
static const ImU32 CLR_RED = IM_COL32(230, 90, 90, 255);

static const int HEADER_H = 44;
static const int CORNER_RADIUS = 10;
static const int CLOSE_ZONE = 46;

static UINT g_ResizeWidth = 0;
static UINT g_ResizeHeight = 0;

static float Snap(float v) { return std::floor(v + 0.5f); }

static void DrawProgressBar(ImDrawList* dl, ImVec2 mn, ImVec2 mx,
    float progress, float time, float alpha) {
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    float w = mx.x - mn.x;
    float h = mx.y - mn.y;
    float rounding = h * 0.5f;

    int a8 = (int)(alpha * 255.0f);
    if (a8 <= 0) return;

    dl->AddRectFilled(mn, mx, IM_COL32(20, 20, 26, a8), rounding);
    dl->AddRect(mn, mx, IM_COL32(189, 176, 255, (int)(a8 * 0.20f)), rounding, 0, 1.0f);

    float filledW = w * progress;
    if (filledW <= 0.5f) return;

    float pad = 1.5f;
    ImVec2 fMin(mn.x + pad, mn.y + pad);
    ImVec2 fMax(mn.x + filledW - pad, mx.y - pad);
    if (fMax.x <= fMin.x + 0.5f) return;

    float fRound = (h - pad * 2.0f) * 0.5f;

    ImU32 topCol = IM_COL32(210, 200, 255, a8);
    ImU32 botCol = IM_COL32(140, 120, 220, a8);
    dl->AddRectFilledMultiColor(fMin, fMax, topCol, topCol, botCol, botCol);

    if (progress > 0.005f && progress < 0.999f) {
        float pulse = 0.5f + 0.5f * sinf(time * 6.0f);
        int glowA = (int)(a8 * (0.30f + 0.35f * pulse));
        ImU32 glowCol = IM_COL32(189, 176, 255, glowA);
        float gr = h * 1.8f;
        dl->AddCircleFilled(ImVec2(fMax.x, (mn.y + mx.y) * 0.5f), gr, glowCol, 16);
    }

    int dA = (int)(a8 * 0.95f);
    dl->AddCircleFilled(ImVec2(fMax.x, (mn.y + mx.y) * 0.5f),
        (h - pad * 2.0f) * 0.5f,
        IM_COL32(245, 240, 255, dA), 16);
}

static void DrawStyledButton(ImDrawList* dl, ImVec2 wp,
    float x, float y, float w, float h,
    const char* label, ImFont* font, float fontSize,
    float hoverVal, bool enabled, float time, int iconType = 0) {
    ImVec2 bMin(Snap(wp.x + x), Snap(wp.y + y));
    ImVec2 bMax(bMin.x + w, bMin.y + h);
    float hv = hoverVal;
    float hvr = 8.0f;

    if (hv > 0.01f && enabled) {
        for (int i = 5; i >= 1; --i) {
            float e = (float)i * 2.2f;
            int a = (int)(22 * hv / (float)i);
            dl->AddRectFilled(
                ImVec2(bMin.x - e, bMin.y - e),
                ImVec2(bMax.x + e, bMax.y + e),
                IM_COL32(160, 140, 255, a), hvr + e * 0.5f);
        }
    }

    ImU32 baseTop, baseBot;
    if (enabled) {
        baseTop = IM_COL32(
            (int)(22 + (46 - 22) * hv),
            (int)(22 + (42 - 22) * hv),
            (int)(30 + (78 - 30) * hv), 255);
        baseBot = IM_COL32(
            (int)(14 + (32 - 14) * hv),
            (int)(14 + (30 - 14) * hv),
            (int)(20 + (58 - 20) * hv), 255);
    }
    else {
        baseTop = IM_COL32(28, 28, 34, 255);
        baseBot = IM_COL32(18, 18, 22, 255);
    }
    dl->AddRectFilledMultiColor(bMin, bMax, baseTop, baseTop, baseBot, baseBot);

    dl->PushClipRect(bMin, bMax, true);
    dl->AddRectFilledMultiColor(
        ImVec2(bMin.x + 1.0f, bMin.y + 1.0f),
        ImVec2(bMax.x - 1.0f, bMin.y + h * 0.5f),
        IM_COL32(255, 255, 255, enabled ? (int)(14 + 22 * hv) : 8),
        IM_COL32(255, 255, 255, enabled ? (int)(14 + 22 * hv) : 8),
        IM_COL32(255, 255, 255, 0),
        IM_COL32(255, 255, 255, 0));
    dl->PopClipRect();

    ImU32 borderCol = enabled
        ? IM_COL32(
            (int)(189 + (255 - 189) * hv * 0.3f),
            (int)(176 + (230 - 176) * hv * 0.3f),
            (int)(255), 255)
        : IM_COL32(80, 80, 90, 255);
    dl->AddRect(bMin, bMax, borderCol, hvr, 0, 1.5f);
    dl->AddRect(
        ImVec2(bMin.x + 1.0f, bMin.y + 1.0f),
        ImVec2(bMax.x - 1.0f, bMax.y - 1.0f),
        IM_COL32(255, 255, 255, 18), hvr - 1.0f, 0, 1.0f);

    ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label);

    float iconW = (iconType > 0) ? fontSize * 0.7f : 0.0f;
    float gap = (iconType > 0) ? 8.0f : 0.0f;
    float totalW = iconW + gap + ts.x;
    float startX = bMin.x + (w - totalW) * 0.5f;
    float cy = bMin.y + (h - ts.y) * 0.5f;
    float cx = startX;

    ImU32 labelCol = enabled
        ? IM_COL32(
            (int)(210 + (255 - 210) * hv),
            (int)(200 + (245 - 200) * hv),
            (int)(255), 255)
        : IM_COL32(110, 110, 125, 255);

    if (iconType == 1) {
        float iconH = fontSize * 0.7f;
        float iconCy = cy + ts.y * 0.5f;
        float triY = iconCy - iconH * 0.5f;
        dl->AddTriangleFilled(
            ImVec2(startX, triY),
            ImVec2(startX + iconH * 0.8f, iconCy),
            ImVec2(startX, triY + iconH),
            labelCol
        );
        cx += iconW + gap;
    }
    else if (iconType == 2) {
        float iconH = fontSize * 0.6f;
        float iconCy = cy + ts.y * 0.5f;
        float iconCx = startX + iconW * 0.5f;
        float d = iconH * 0.5f;
        dl->AddLine(ImVec2(iconCx - d, iconCy - d), ImVec2(iconCx + d, iconCy + d), labelCol, 2.0f);
        dl->AddLine(ImVec2(iconCx + d, iconCy - d), ImVec2(iconCx - d, iconCy + d), labelCol, 2.0f);
        cx += iconW + gap;
    }

    if (hv > 0.01f && enabled)
        dl->AddText(font, fontSize, ImVec2(cx + 1.0f, cy + 1.0f),
            IM_COL32(120, 100, 200, (int)(120 * hv)), label);
    dl->AddText(font, fontSize, ImVec2(cx, cy), labelCol, label);
}

static void DrawBrandText(ImDrawList* dl, ImFont* font, ImVec2 wp, ImVec2 ds,
    float y, float fontSize, int alphaOverride = -1) {
    const char* text = "OBLIVION";
    ImVec2 sz = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
    float textX = wp.x + (ds.x - sz.x) * 0.5f;
    int a = (alphaOverride >= 0) ? alphaOverride : 255;
    ImU32 col = IM_COL32(235, 228, 255, a);
    dl->AddText(font, fontSize, ImVec2(Snap(textX), Snap(y)), col, text);
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_NCHITTEST: {
        POINT pt{ (LONG)(short)LOWORD(lParam), (LONG)(short)HIWORD(lParam) };
        ScreenToClient(hWnd, &pt);
        if (pt.y >= 0 && pt.y <= HEADER_H && pt.x >= 0 && pt.x < 400) {
            return HTCAPTION;
        }
        return HTCLIENT;
    }
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool Frontend::Initialize(HINSTANCE hInstance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OBLIVION_LOADER_WND";
    RegisterClassExW(&wc);

    int sx = GetSystemMetrics(SM_CXSCREEN);
    int sy = GetSystemMetrics(SM_CYSCREEN);
    int wx = (sx - m_winW) / 2;
    int wy = (sy - m_winH) / 2;

    m_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_APPWINDOW,
        wc.lpszClassName, L"Oblivion Loader",
        WS_POPUP, wx, wy, m_winW, m_winH,
        nullptr, nullptr, hInstance, nullptr);

    if (!m_hwnd) return false;

    SetLayeredWindowAttributes(m_hwnd, 0, 255, LWA_ALPHA);
    ApplyRoundRegion();

    if (!CreateDeviceD3D()) return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    auto loadFont = [&](const char* path, float size) -> ImFont* {
        if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) return nullptr;
        ImFontConfig cfg;
        cfg.OversampleH = 3;
        cfg.OversampleV = 3;
        cfg.PixelSnapH = true;
        return io.Fonts->AddFontFromFileTTF(path, size, &cfg);
        };

    m_bodyFont = loadFont("C:\\Windows\\Fonts\\segoeui.ttf", 15.0f);
    m_titleFont = loadFont("C:\\Windows\\Fonts\\segoeuib.ttf", 26.0f);
    m_smallFont = loadFont("C:\\Windows\\Fonts\\segoeui.ttf", 14.0f);
    m_tinyFont = loadFont("C:\\Windows\\Fonts\\segoeui.ttf", 13.0f);
    if (!m_bodyFont)  m_bodyFont = io.Fonts->AddFontDefault();
    if (!m_titleFont) m_titleFont = m_bodyFont;
    if (!m_smallFont) m_smallFont = m_bodyFont;
    if (!m_tinyFont)  m_tinyFont = m_bodyFont;
    io.FontDefault = m_bodyFont;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.ScrollbarSize = 10.0f;
    style.ScrollbarRounding = 5.0f;
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.74f, 0.69f, 1.0f, 0.4f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.74f, 0.69f, 1.0f, 0.8f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.74f, 0.69f, 1.0f, 1.0f);

    ImGui_ImplWin32_Init(m_hwnd);
    ImGui_ImplDX11_Init(m_pd3dDevice, m_pd3dDeviceContext);

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    {
        RECT rc{};
        GetClientRect(m_hwnd, &rc);
        g_ResizeWidth = (UINT)(rc.right - rc.left);
        g_ResizeHeight = (UINT)(rc.bottom - rc.top);
    }
    return true;
}

void Frontend::Run() {
    const float clear[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    MSG msg{};

    while (m_running) {
        while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) m_running = false;
        }
        if (!m_running) break;

        int desiredH = m_winH;
        switch (m_state) {
        case FrontendState::Splash:   desiredH = 300; break;
        case FrontendState::Checking: desiredH = 260; break;
        case FrontendState::Main:     desiredH = 340; break;
        case FrontendState::Loading:  desiredH = 300; break;
        case FrontendState::Error:    desiredH = 260; break;
        default: break;
        }

        if (desiredH != m_winH) {
            RECT r; GetWindowRect(m_hwnd, &r);
            int cx = (r.left + r.right) / 2;
            int cy = (r.top + r.bottom) / 2;
            m_winH = desiredH;
            SetWindowPos(m_hwnd, nullptr, cx - m_winW / 2, cy - m_winH / 2,
                m_winW, m_winH, SWP_NOZORDER | SWP_NOACTIVATE);
            ApplyRoundRegion();
        }

        if (m_swapChainOccluded &&
            m_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            Sleep(10);
            continue;
        }
        m_swapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            m_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = 0;
            g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderFrame();

        ImGui::Render();
        m_pd3dDeviceContext->OMSetRenderTargets(1, &m_mainRenderTargetView, nullptr);
        m_pd3dDeviceContext->ClearRenderTargetView(m_mainRenderTargetView, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = m_pSwapChain->Present(1, 0);
        m_swapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);

        if (m_state == FrontendState::Done) {
            m_running = false;
            PostQuitMessage(0);
        }
    }
}

void Frontend::Shutdown() {
    if (m_hCheckThread) {
        WaitForSingleObject(m_hCheckThread, 3000);
        CloseHandle(m_hCheckThread);
        m_hCheckThread = nullptr;
    }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        UnregisterClassW(L"OBLIVION_LOADER_WND", GetModuleHandleW(nullptr));
        m_hwnd = nullptr;
    }
}

bool Frontend::CreateDeviceD3D() {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;
    D3D_FEATURE_LEVEL level;
    const D3D_FEATURE_LEVEL levels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        levels, 2, D3D11_SDK_VERSION, &sd,
        &m_pSwapChain, &m_pd3dDevice, &level, &m_pd3dDeviceContext);

    if (res == DXGI_ERROR_UNSUPPORTED) {
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags,
            levels, 2, D3D11_SDK_VERSION, &sd,
            &m_pSwapChain, &m_pd3dDevice, &level, &m_pd3dDeviceContext);
    }
    if (res != S_OK) return false;

    CreateRenderTarget();
    return true;
}

void Frontend::CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (m_pSwapChain) { m_pSwapChain->Release();        m_pSwapChain = nullptr; }
    if (m_pd3dDeviceContext) { m_pd3dDeviceContext->Release(); m_pd3dDeviceContext = nullptr; }
    if (m_pd3dDevice) { m_pd3dDevice->Release();        m_pd3dDevice = nullptr; }
}

void Frontend::CreateRenderTarget() {
    ID3D11Texture2D* back = nullptr;
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) {
        m_pd3dDevice->CreateRenderTargetView(back, nullptr, &m_mainRenderTargetView);
        back->Release();
    }
}

void Frontend::CleanupRenderTarget() {
    if (m_mainRenderTargetView) {
        m_mainRenderTargetView->Release();
        m_mainRenderTargetView = nullptr;
    }
}

void Frontend::ApplyRoundRegion() {
    HRGN rgn = CreateRoundRectRgn(0, 0, m_winW + 1, m_winH + 1,
        CORNER_RADIUS * 2, CORNER_RADIUS * 2);
    SetWindowRgn(m_hwnd, rgn, TRUE);
}

DWORD WINAPI Frontend::CheckThreadFunc(LPVOID param) {
    Frontend* self = static_cast<Frontend*>(param);

    bool dlOk = Backend::RefreshInfo();
    bool verOk = false;
    if (dlOk) {
        self->m_info = Backend::ParseInfo();
        verOk = Backend::IsVersionUpToDate();
    }
    self->SetCheckResult(dlOk && self->m_info.valid, verOk);
    self->m_checkComplete.store(true);
    return 0;
}

void Frontend::UpdateAnimations(float dt) {
    if (m_state == FrontendState::Splash) {
        if (m_splashFadingIn) {
            m_splashAlpha += dt * 1.25f;
            if (m_splashAlpha >= 1.0f) {
                m_splashAlpha = 1.0f;
                m_splashFadingIn = false;
                m_splashHoldTimer = 0.0f;
            }
        }
        else if (!m_splashFadingOut) {
            m_splashHoldTimer += dt;
            if (m_splashHoldTimer >= 1.5f) m_splashFadingOut = true;
        }
        else {
            m_splashAlpha -= dt * 2.0f;
            if (m_splashAlpha <= 0.0f) {
                m_splashAlpha = 0.0f;
                m_state = FrontendState::Checking;
                m_checkTimer = 0.0f;
            }
        }
    }

    if (m_state == FrontendState::Checking) {
        m_checkTimer += dt;

        if (!m_checkStarted.load()) {
            m_checkStarted.store(true);
            m_hCheckThread = CreateThread(nullptr, 0, CheckThreadFunc, this, 0, nullptr);
        }

        if (m_checkComplete.load()) {
            if (!m_checkSuccess.load()) {
                m_errorMsg = "Could not reach update server";
                m_errorDetail = "Check your internet connection and try again.";
                m_state = FrontendState::Error;
            }
            else if (!m_versionOk.load()) {
                std::string remote = Backend::GetRemoteVersion();
                m_errorMsg = "Loader is outdated";
                m_errorDetail = "Local: " + std::string(Backend::LOCAL_VERSION) +
                    "   Server: " + (remote.empty() ? "unknown" : remote);
                m_state = FrontendState::Error;
            }
            else {
                m_state = FrontendState::Main;
            }
        }
    }

    if (m_state == FrontendState::Loading) {
        m_prepareTimer += dt;
        m_loadStageTimer += dt;

        switch (m_loadStage) {
        case LoadStage::Connecting:
            if (m_loadStageTimer >= 1.0f) {
                m_loadStage = LoadStage::CreatingDirectories;
                m_loadStageTimer = 0.0f;
                m_stageActionDone = false;
            }
            break;

        case LoadStage::CreatingDirectories:
            if (!m_stageActionDone) {
                Backend::EnsureDirectories();
                Backend::EnsureClickSound();
                m_stageActionDone = true;
            }
            if (m_loadStageTimer >= 0.8f) {
                m_loadStage = LoadStage::DownloadingFiles;
                m_loadStageTimer = 0.0f;
                m_stageActionDone = false;
            }
            break;

        case LoadStage::DownloadingFiles:
            if (!m_stageActionDone) {
                Backend::EnsureOblivionExe();
                m_stageActionDone = true;
            }
            if (m_loadStageTimer >= 1.5f) {
                m_loadStage = LoadStage::LaunchingExe;
                m_loadStageTimer = 0.0f;
                m_stageActionDone = false;
            }
            break;

        case LoadStage::LaunchingExe:
            if (!m_stageActionDone) {
                bool ok = Backend::LaunchOblivionExe();
                m_stageActionDone = true;
                if (!ok) {
                    m_errorMsg = "Failed to launch Oblivion.exe";
                    m_errorDetail = "Check that you accepted the UAC prompt.";
                    m_state = FrontendState::Error;
                    break;
                }
            }
            if (m_loadStageTimer >= 2.0f) {
                m_loadStage = LoadStage::Done;
                m_state = FrontendState::Done;
            }
            break;

        default: break;
        }

        float target = 0.0f;
        switch (m_loadStage) {
        case LoadStage::Connecting:          target = 0.10f * (m_loadStageTimer / 1.0f); break;
        case LoadStage::CreatingDirectories: target = 0.10f + 0.25f * (m_loadStageTimer / 0.8f); break;
        case LoadStage::DownloadingFiles:    target = 0.35f + 0.45f * (m_loadStageTimer / 1.5f); break;
        case LoadStage::LaunchingExe:        target = 0.80f + 0.20f * (m_loadStageTimer / 2.0f); break;
        default:                             target = 1.0f;
        }
        target = (std::min)(target, 1.0f);
        float lerp = 1.0f - powf(0.0001f, dt);
        m_progress += (target - m_progress) * lerp;
    }
}

void Frontend::RenderFrame() {
    float dt = ImGui::GetIO().DeltaTime;
    if (dt > 0.05f) dt = 0.05f;
    if (dt < 0.0f)  dt = 0.0f;

    UpdateAnimations(dt);

    ImVec2 ds = ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ds);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

    ImGui::Begin("##oblivion_root", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNavFocus);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2      wp = ImGui::GetWindowPos();

    dl->AddRectFilled(wp, ImVec2(wp.x + ds.x, wp.y + ds.y),
        CLR_BG, (float)CORNER_RADIUS);

    dl->AddRectFilledMultiColor(
        ImVec2(wp.x, wp.y),
        ImVec2(wp.x + ds.x, wp.y + 140.0f),
        IM_COL32(40, 34, 70, 60),
        IM_COL32(40, 34, 70, 60),
        IM_COL32(19, 19, 23, 0),
        IM_COL32(19, 19, 23, 0));

    if (m_state != FrontendState::Splash)
        RenderTitleBar(dl, wp, ds);

    switch (m_state) {
    case FrontendState::Splash:   RenderSplash(dl, wp, ds); break;
    case FrontendState::Checking: RenderChecking(dl, wp, ds); break;
    case FrontendState::Main:     RenderMain(dl, wp, ds); break;
    case FrontendState::Loading:  RenderLoading(dl, wp, ds); break;
    case FrontendState::Error:    RenderError(dl, wp, ds); break;
    default: break;
    }

    dl->AddRect(wp, ImVec2(wp.x + ds.x, wp.y + ds.y),
        IM_COL32(189, 176, 255, 90), (float)CORNER_RADIUS, 0, 1.0f);

    ImGui::End();
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(1);
}

void Frontend::RenderTitleBar(ImDrawList* dl, ImVec2 wp, ImVec2 ds) {
    dl->AddRectFilled(wp, ImVec2(wp.x + ds.x, wp.y + HEADER_H),
        CLR_HEADER_BG, (float)CORNER_RADIUS,
        ImDrawFlags_RoundCornersTop);

    DrawBrandText(dl, m_titleFont, wp, ds, wp.y + 9.0f, 26.0f);

    dl->AddLine(ImVec2(wp.x, wp.y + HEADER_H),
        ImVec2(wp.x + ds.x, wp.y + HEADER_H),
        CLR_BORDER_SOFT, 1.0f);
}

void Frontend::RenderSplash(ImDrawList* dl, ImVec2 wp, ImVec2 ds) {
    if (m_splashAlpha <= 0.001f) return;

    int a = (int)(m_splashAlpha * 255.0f);
    const float fs = 54.0f;
    const char* text = "OBLIVION";

    ImVec2 sz = m_titleFont->CalcTextSizeA(fs, FLT_MAX, 0.0f, text);
    float textX = wp.x + (ds.x - sz.x) * 0.5f;
    float textY = wp.y + (ds.y - sz.y) * 0.5f;

    int glowA = (int)(a * 0.35f);
    ImU32 glow = IM_COL32(140, 120, 255, glowA);
    for (int oy = -1; oy <= 1; ++oy) {
        for (int ox = -1; ox <= 1; ++ox) {
            if (ox == 0 && oy == 0) continue;
            dl->AddText(m_titleFont, fs,
                ImVec2(textX + ox * 1.2f, textY + oy * 1.2f),
                glow, text);
        }
    }

    ImU32 white = IM_COL32(235, 228, 255, a);
    dl->AddText(m_titleFont, fs, ImVec2(Snap(textX), Snap(textY)), white, text);
}

void Frontend::RenderChecking(ImDrawList* dl, ImVec2 wp, ImVec2 ds) {
    const char* txt = "Checking for updates...";
    ImVec2 ts = m_bodyFont->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, txt);
    dl->AddText(m_bodyFont, 15.0f,
        ImVec2(Snap(wp.x + (ds.x - ts.x) * 0.5f),
            Snap(wp.y + ds.y * 0.5f - ts.y * 0.5f)),
        CLR_TEXT_DIM, txt);
}

void Frontend::RenderMain(ImDrawList* dl, ImVec2 wp, ImVec2 ds) {
    float dt = ImGui::GetIO().DeltaTime;
    ImVec2 mouse = ImGui::GetIO().MousePos;
    bool clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

    const float pad = 20.0f;
    const float gap = 10.0f;
    const float leftW = 240.0f;
    const float rightW = ds.x - pad * 2.0f - leftW - gap;
    const float colY = 60.0f;
    const float colH = 260.0f;

    ImVec2 clMin(Snap(wp.x + pad), Snap(wp.y + colY));
    ImVec2 clMax(clMin.x + leftW, clMin.y + colH);
    dl->AddRectFilled(clMin, clMax, CLR_CARD_BG, 8.0f);
    dl->AddRect(clMin, clMax, CLR_BORDER, 8.0f, 0, 1.0f);

    const float hdrH = 34.0f;
    dl->AddRectFilled(clMin, ImVec2(clMax.x, clMin.y + hdrH), CLR_CARD_HDR, 8.0f, ImDrawFlags_RoundCornersTop);
    dl->AddLine(ImVec2(clMin.x + 1.0f, clMin.y + hdrH), ImVec2(clMax.x - 1.0f, clMin.y + hdrH), CLR_BORDER_SOFT, 1.0f);
    dl->AddText(m_bodyFont, 14.0f, ImVec2(Snap(clMin.x + 14.0f), Snap(clMin.y + 8.0f)), CLR_TEXT_MAIN, "CHANGELOGS");

    const float innerPad = 10.0f;
    ImVec2 innerMin(clMin.x + innerPad, clMin.y + hdrH + innerPad);
    float innerW = leftW - innerPad * 2.0f;
    float innerH = colH - hdrH - innerPad * 2.0f;

    ImGui::SetCursorPos(ImVec2(innerMin.x - wp.x, innerMin.y - wp.y));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 6));
    ImGui::BeginChild("##changelogs", ImVec2(innerW, innerH), false, ImGuiWindowFlags_NoBackground);
    ImGui::PushFont(m_bodyFont);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);

    if (!m_info.changelogs.empty()) {
        std::istringstream ss(m_info.changelogs);
        std::string line;
        while (std::getline(ss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.78f, 0.92f, 1.0f));
            ImGui::TextWrapped("%s", line.c_str());
            ImGui::PopStyleColor();
        }
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.53f, 0.68f, 1.0f));
        ImGui::TextWrapped("No changelogs available.");
        ImGui::PopStyleColor();
    }
    ImGui::PopFont();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    const float statH = 130.0f;
    ImVec2 statMin(Snap(wp.x + pad + leftW + gap), Snap(wp.y + colY));
    ImVec2 statMax(statMin.x + rightW, statMin.y + statH);

    dl->AddRectFilled(statMin, statMax, CLR_CARD_BG, 8.0f);
    dl->AddRect(statMin, statMax, CLR_BORDER, 8.0f, 0, 1.0f);
    dl->AddRectFilled(statMin, ImVec2(statMax.x, statMin.y + hdrH), CLR_CARD_HDR, 8.0f, ImDrawFlags_RoundCornersTop);
    dl->AddLine(ImVec2(statMin.x + 1.0f, statMin.y + hdrH), ImVec2(statMax.x - 1.0f, statMin.y + hdrH), CLR_BORDER_SOFT, 1.0f);
    dl->AddText(m_bodyFont, 14.0f, ImVec2(Snap(statMin.x + 14.0f), Snap(statMin.y + 8.0f)), CLR_TEXT_MAIN, "INFORMATION");

    auto DrawRow = [&](float rowY, const char* label, const char* value, ImU32 valCol) {
        dl->AddText(m_bodyFont, 14.0f, ImVec2(Snap(statMin.x + 14.0f), Snap(rowY)), CLR_TEXT_LABEL, label);
        ImVec2 vs = m_bodyFont->CalcTextSizeA(14.0f, FLT_MAX, 0.0f, value);
        dl->AddText(m_bodyFont, 14.0f, ImVec2(Snap(statMax.x - 14.0f - vs.x), Snap(rowY)), valCol, value);
        };

    ImU32 statusCol = CLR_ACCENT;
    {
        std::string sl = m_info.status;
        std::transform(sl.begin(), sl.end(), sl.begin(), ::tolower);
        if (sl.find("undetect") != std::string::npos) statusCol = CLR_GREEN;
        else if (sl.find("detect") != std::string::npos) statusCol = CLR_RED;
    }

    DrawRow(statMin.y + hdrH + 16.0f, "Status", m_info.status.c_str(), statusCol);
    DrawRow(statMin.y + hdrH + 42.0f, "Version", m_info.version.c_str(), CLR_TEXT_MAIN);
    DrawRow(statMin.y + hdrH + 68.0f, "Updated", m_info.updated.c_str(), CLR_TEXT_MAIN);

    const float exitH = 50.0f;
    float exitY = statMax.y + 15.0f;
    float exitX = statMin.x;
    float exitW = rightW;

    ImGui::SetCursorPos(ImVec2(exitX, exitY));
    ImGui::PushID("exit");
    ImGui::InvisibleButton("##exitbtn", ImVec2(exitW, exitH));
    bool exitHovered = ImGui::IsItemHovered();
    bool exitClicked = ImGui::IsItemClicked();
    ImGui::PopID();

    float lerp = 1.0f - powf(0.0001f, dt);
    m_exitHover += ((exitHovered ? 1.0f : 0.0f) - m_exitHover) * lerp;

    DrawStyledButton(dl, wp, exitX, exitY, exitW, exitH,
        "EXIT", m_titleFont, 16.0f, m_exitHover, true, (float)ImGui::GetTime(), 2);

    if (exitClicked) {
        m_running = false;
        PostQuitMessage(0);
    }

    const float btnH = 50.0f;
    float btnY = exitY + exitH + 15.0f;
    float btnX = statMin.x;
    float btnW = rightW;

    ImGui::SetCursorPos(ImVec2(btnX, btnY));
    ImGui::PushID("load");
    ImGui::InvisibleButton("##loadbtn", ImVec2(btnW, btnH));
    bool hovered = ImGui::IsItemHovered();
    bool clickedBtn = ImGui::IsItemClicked();
    ImGui::PopID();

    m_btnHover += ((hovered ? 1.0f : 0.0f) - m_btnHover) * lerp;

    bool canLoad = m_info.allowLoad;

    DrawStyledButton(dl, wp, btnX, btnY, btnW, btnH,
        canLoad ? "LOAD" : "UNAVAILABLE",
        m_titleFont, 17.0f,
        m_btnHover, canLoad,
        (float)ImGui::GetTime(), 1);

    if (clickedBtn && canLoad) {
        m_state = FrontendState::Loading;
        m_loadStage = LoadStage::Connecting;
        m_loadStageTimer = 0.0f;
        m_stageActionDone = false;
        m_prepareTimer = 0.0f;
        m_progress = 0.0f;
    }
}

void Frontend::RenderLoading(ImDrawList* dl, ImVec2 wp, ImVec2 ds) {
    float dt = ImGui::GetIO().DeltaTime;
    if (dt > 0.05f) dt = 0.05f;

    DrawBrandText(dl, m_titleFont, wp, ds, wp.y + 80.0f, 34.0f);

    const float pbX = wp.x + 40.0f;
    const float pbW = ds.x - 80.0f;
    const float pbY = wp.y + 150.0f;
    const float pbH = 8.0f;

    DrawProgressBar(dl,
        ImVec2(Snap(pbX), Snap(pbY)),
        ImVec2(Snap(pbX + pbW), Snap(pbY + pbH)),
        m_progress, m_prepareTimer, 1.0f);

    char pct[16];
    int p = (int)(m_progress * 100.0f + 0.5f);
    if (p < 0) p = 0;
    if (p > 100) p = 100;
    snprintf(pct, sizeof(pct), "%d%%", p);

    ImVec2 ps = m_tinyFont->CalcTextSizeA(14.0f, FLT_MAX, 0.0f, pct);
    dl->AddText(m_tinyFont, 14.0f,
        ImVec2(Snap(wp.x + (ds.x - ps.x) * 0.5f), Snap(pbY + 24.0f)),
        CLR_TEXT_MUTED, pct);

    const char* stageText = "";
    switch (m_loadStage) {
    case LoadStage::Connecting:          stageText = "Connecting..."; break;
    case LoadStage::CreatingDirectories: stageText = "Creating directories..."; break;
    case LoadStage::DownloadingFiles:    stageText = "Downloading files..."; break;
    case LoadStage::LaunchingExe:        stageText = "Launching Oblivion..."; break;
    default:                             stageText = "Done"; break;
    }

    ImVec2 ss = m_smallFont->CalcTextSizeA(14.0f, FLT_MAX, 0.0f, stageText);
    dl->AddText(m_smallFont, 14.0f,
        ImVec2(Snap(wp.x + (ds.x - ss.x) * 0.5f), Snap(pbY + 48.0f)),
        CLR_TEXT_DIM, stageText);
}

void Frontend::RenderError(ImDrawList* dl, ImVec2 wp, ImVec2 ds) {
    const char* title = "ERROR";
    ImVec2 ts = m_titleFont->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, title);
    dl->AddText(m_titleFont, 20.0f,
        ImVec2(Snap(wp.x + (ds.x - ts.x) * 0.5f), Snap(wp.y + 80.0f)),
        CLR_RED, title);

    ImVec2 ms = m_bodyFont->CalcTextSizeA(16.0f, FLT_MAX, 0.0f, m_errorMsg.c_str());
    dl->AddText(m_bodyFont, 16.0f,
        ImVec2(Snap(wp.x + (ds.x - ms.x) * 0.5f), Snap(wp.y + 120.0f)),
        CLR_TEXT_MAIN, m_errorMsg.c_str());

    if (!m_errorDetail.empty()) {
        ImVec2 ds2 = m_tinyFont->CalcTextSizeA(13.0f, FLT_MAX, 0.0f, m_errorDetail.c_str());
        dl->AddText(m_tinyFont, 13.0f,
            ImVec2(Snap(wp.x + (ds.x - ds2.x) * 0.5f), Snap(wp.y + 148.0f)),
            CLR_TEXT_MUTED, m_errorDetail.c_str());
    }
}