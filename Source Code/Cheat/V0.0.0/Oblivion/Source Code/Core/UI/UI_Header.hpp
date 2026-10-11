#pragma once
#include <imgui.h>

namespace UI {
    namespace Header {
        void Draw(ImDrawList* dl, ImVec2 pos, float menuW, float hdrH,
            float s, float a, float dt, bool& anyIconHover);
    }
}