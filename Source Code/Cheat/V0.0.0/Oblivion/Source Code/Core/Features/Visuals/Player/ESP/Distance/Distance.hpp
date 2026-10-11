#pragma once
#include "../../../../../../SDK/Entities/Entities.hpp"
#include <d3d11.h>

namespace Features::ESP {
    void RenderDistanceESP(const FrameSnapshot& frame,
        const ViewMatrix& viewMatrix,
        ID3D11Device* device,
        ID3D11DeviceContext* context,
        int width, int height);
}