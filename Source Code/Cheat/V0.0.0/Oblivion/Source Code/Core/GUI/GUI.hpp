#pragma once
#include <d3d11.h>

namespace GUI {
    void Render(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height);
    bool IsOpen();
    void Toggle();
}