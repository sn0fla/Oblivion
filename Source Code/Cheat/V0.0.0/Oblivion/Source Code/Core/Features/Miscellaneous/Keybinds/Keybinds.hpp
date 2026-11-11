#pragma once
#include <imgui.h>

namespace Keybinds {
    void Render(float s, ImVec2 menuPos, float menuW);

    bool Visible();
    void SetVisible(bool v);
    void Toggle();
}