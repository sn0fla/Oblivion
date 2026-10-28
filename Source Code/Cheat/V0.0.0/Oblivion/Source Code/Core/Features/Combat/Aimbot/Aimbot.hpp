#pragma once
#include "../../../../SDK/Entities/Entities.hpp"

namespace Aimbot {
    void Tick(const FrameSnapshot& frame);
    void RenderFOV(const ViewMatrix& vm);
}