#include "Render.hpp"
#include "../GUI/Framework/Framework.hpp"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <cstdio>

static ID3D11Device* g_Device = nullptr;
static ID3D11DeviceContext* g_Context = nullptr;
static IDXGISwapChain* g_Swapchain = nullptr;
static ID3D11RenderTargetView* g_RTV = nullptr;
static bool g_Ready = false;
static ImFont* g_BoldFont = nullptr;

static UINT        g_BackbufferWidth = 0;
static UINT        g_BackbufferHeight = 0;
static DXGI_FORMAT g_BackbufferFormat = DXGI_FORMAT_UNKNOWN;

static void UnbindPipelineForResize() {
    if (!g_Context) return;

    ID3D11RenderTargetView* nullRTV[1] = { nullptr };
    ID3D11ShaderResourceView* nullSRV[8] = { nullptr, nullptr, nullptr, nullptr,
                                             nullptr, nullptr, nullptr, nullptr };
    ID3D11UnorderedAccessView* nullUAV[4] = { nullptr, nullptr, nullptr, nullptr };

    g_Context->OMSetRenderTargets(1, nullRTV, nullptr);
    g_Context->PSSetShaderResources(0, 8, nullSRV);
    g_Context->VSSetShaderResources(0, 8, nullSRV);
    g_Context->GSSetShaderResources(0, 8, nullSRV);
    g_Context->CSSetShaderResources(0, 8, nullSRV);
    g_Context->CSSetUnorderedAccessViews(0, 4, nullUAV, nullptr);
    g_Context->Flush();
}

static bool CreateRTV() {
    if (g_RTV) { g_RTV->Release(); g_RTV = nullptr; }
    if (!g_Swapchain || !g_Device || !g_Context) return false;

    ID3D11Texture2D* back = nullptr;
    if (FAILED(g_Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back)))
        return false;

    D3D11_TEXTURE2D_DESC d{};
    back->GetDesc(&d);

    HRESULT hr = g_Device->CreateRenderTargetView(back, nullptr, &g_RTV);
    back->Release();

    if (FAILED(hr)) {
        g_RTV = nullptr;
        return false;
    }

    g_BackbufferWidth = d.Width;
    g_BackbufferHeight = d.Height;
    g_BackbufferFormat = d.Format;

    D3D11_VIEWPORT vp{};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = (float)d.Width;
    vp.Height = (float)d.Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    g_Context->RSSetViewports(1, &vp);

    return true;
}

static void ResetBackbufferTargets() {
    UnbindPipelineForResize();

    if (g_RTV) { g_RTV->Release(); g_RTV = nullptr; }

    g_BackbufferWidth = 0;
    g_BackbufferHeight = 0;
    g_BackbufferFormat = DXGI_FORMAT_UNKNOWN;
}

bool Render::Init(IDXGISwapChain* swapChain) {
    if (!swapChain) return false;

    ImGui_ImplWin32_EnableDpiAwareness();

    g_Swapchain = swapChain;
    if (FAILED(swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_Device))) {
        g_Swapchain = nullptr;
        return false;
    }
    g_Device->GetImmediateContext(&g_Context);

    if (!CreateRTV()) {
        if (g_Context) { g_Context->Release(); g_Context = nullptr; }
        if (g_Device) { g_Device->Release(); g_Device = nullptr; }
        g_Swapchain = nullptr;
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    char winDir[MAX_PATH]{};
    GetWindowsDirectoryA(winDir, MAX_PATH);

    char boldPath[MAX_PATH]{};
    sprintf_s(boldPath, "%s\\Fonts\\arialbd.ttf", winDir);

    ImFontConfig cfg;
    cfg.OversampleH = 3;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = true;

    g_BoldFont = io.Fonts->AddFontFromFileTTF(boldPath, 26.0f, &cfg);
    if (!g_BoldFont) {
        char fallback[MAX_PATH]{};
        sprintf_s(fallback, "%s\\Fonts\\segoeuib.ttf", winDir);
        g_BoldFont = io.Fonts->AddFontFromFileTTF(fallback, 26.0f, &cfg);
    }
    if (!g_BoldFont) {
        io.Fonts->AddFontDefault();
        g_BoldFont = io.Fonts->Fonts[0];
    }

    DXGI_SWAP_CHAIN_DESC desc{};
    swapChain->GetDesc(&desc);
    if (!ImGui_ImplWin32_Init(desc.OutputWindow)) {
        ImGui::DestroyContext();
        if (g_RTV) { g_RTV->Release(); g_RTV = nullptr; }
        if (g_Context) { g_Context->Release(); g_Context = nullptr; }
        if (g_Device) { g_Device->Release(); g_Device = nullptr; }
        g_BoldFont = nullptr;
        g_Swapchain = nullptr;
        return false;
    }
    if (!ImGui_ImplDX11_Init(g_Device, g_Context)) {
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        if (g_RTV) { g_RTV->Release(); g_RTV = nullptr; }
        if (g_Context) { g_Context->Release(); g_Context = nullptr; }
        if (g_Device) { g_Device->Release(); g_Device = nullptr; }
        g_BoldFont = nullptr;
        g_Swapchain = nullptr;
        return false;
    }

    g_Ready = true;
    return true;
}

void Render::Shutdown() {
    if (g_Ready) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    if (g_RTV) { g_RTV->Release(); g_RTV = nullptr; }
    if (g_Context) { g_Context->Release(); g_Context = nullptr; }
    if (g_Device) { g_Device->Release(); g_Device = nullptr; }

    g_Swapchain = nullptr;
    g_BoldFont = nullptr;
    g_Ready = false;

    Framework::ResetCachedFonts();
}

void Render::UpdateSwapchain(IDXGISwapChain* s) {
    if (s == g_Swapchain) return;

    if (g_Ready)
        ImGui_ImplDX11_InvalidateDeviceObjects();

    ResetBackbufferTargets();

    g_Swapchain = s;

    CreateRTV();

    if (g_Ready)
        ImGui_ImplDX11_CreateDeviceObjects();
}

void Render::OnPreResize() {
    if (g_Ready)
        ImGui_ImplDX11_InvalidateDeviceObjects();
    ResetBackbufferTargets();
}

void Render::OnPostResize(IDXGISwapChain* swapChain) {
    if (!swapChain || !g_Device || !g_Context) return;
    g_Swapchain = swapChain;
    CreateRTV();
    if (g_Ready)
        ImGui_ImplDX11_CreateDeviceObjects();
}

void Render::CheckResize(IDXGISwapChain* swapChain) {
    if (!swapChain || !g_Ready) return;

    ID3D11Texture2D* back = nullptr;
    if (FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back)))
        return;

    D3D11_TEXTURE2D_DESC d{};
    back->GetDesc(&d);
    back->Release();

    if (d.Width == g_BackbufferWidth &&
        d.Height == g_BackbufferHeight &&
        d.Format == g_BackbufferFormat)
        return;

    OnPreResize();
    if (swapChain != g_Swapchain) g_Swapchain = swapChain;
    CreateRTV();
    if (g_Ready)
        ImGui_ImplDX11_CreateDeviceObjects();
}

void Render::SyncDisplaySize(IDXGISwapChain*) {
    if (g_BackbufferWidth == 0 || g_BackbufferHeight == 0) return;

    ImGuiIO& io = ImGui::GetIO();
    const float bbW = (float)g_BackbufferWidth;
    const float bbH = (float)g_BackbufferHeight;

    if (io.DisplaySize.x != bbW || io.DisplaySize.y != bbH)
        io.DisplaySize = ImVec2(bbW, bbH);
}

void Render::BeginFrame() {
    ImGui_ImplWin32_NewFrame();
    ImGui_ImplDX11_NewFrame();
    ImGui::NewFrame();
}

void Render::EndFrame() { ImGui::EndFrame(); }

void Render::Present() {
    if (!g_RTV || !g_Ready || !g_Device || !g_Context) return;

    ID3D11RenderTargetView* rtv = g_RTV;
    g_Context->OMSetRenderTargets(1, &rtv, nullptr);

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    ID3D11RenderTargetView* nullRTV = nullptr;
    g_Context->OMSetRenderTargets(1, &nullRTV, nullptr);
}

ID3D11Device* Render::Device() { return g_Device; }
ID3D11DeviceContext* Render::Context() { return g_Context; }
IDXGISwapChain* Render::Swapchain() { return g_Swapchain; }
ImFont* Render::BoldFont() { return g_BoldFont; }
bool Render::IsReady() { return g_Ready; }