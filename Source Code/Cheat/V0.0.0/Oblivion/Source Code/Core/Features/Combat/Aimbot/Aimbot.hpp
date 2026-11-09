#pragma once
#include "../../../../SDK/Entities/Entities.hpp"

struct AimSettings;

namespace Aimbot {
    void Tick(const FrameSnapshot& frame);
    void RenderFOV(const ViewMatrix& vm, const AimSettings& cfg);
}