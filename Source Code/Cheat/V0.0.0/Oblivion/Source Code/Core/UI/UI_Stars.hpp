#pragma once
#include <imgui.h>

namespace UI {
    namespace Stars {
        void Draw(ImDrawList* dl, ImVec2 min, ImVec2 max, float time, float alpha);
    }
}