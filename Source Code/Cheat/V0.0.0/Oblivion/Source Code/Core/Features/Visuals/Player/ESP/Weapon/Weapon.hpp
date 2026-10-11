#pragma once
#include <cstdint>
#include "../../../../../../SDK/Entities/Entities.hpp"
#include <d3d11.h>

const char* GetWeaponNameFromPawn(uintptr_t pawn, char* outName, size_t outSize);

namespace Features::ESP {
    void RenderWeaponESP(const FrameSnapshot& frame,
        const ViewMatrix& viewMatrix,
        ID3D11Device* device,
        ID3D11DeviceContext* context,
        int width, int height);
}