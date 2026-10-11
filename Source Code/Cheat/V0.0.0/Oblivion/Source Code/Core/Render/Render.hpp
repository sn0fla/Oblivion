#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <imgui.h>

namespace Render {
    bool Init(IDXGISwapChain* swapChain);
    void Shutdown();
    void CheckResize(IDXGISwapChain* swapChain);
    void BeginFrame();
    void EndFrame();
    void Present();
    void UpdateSwapchain(IDXGISwapChain* swapChain);
    void SyncDisplaySize(IDXGISwapChain* swapChain);
    void OnPreResize();
    void OnPostResize(IDXGISwapChain* swapChain);
    ID3D11Device* Device();
    ID3D11DeviceContext* Context();
    IDXGISwapChain* Swapchain();
    ImFont* BoldFont();
    bool IsReady();
}