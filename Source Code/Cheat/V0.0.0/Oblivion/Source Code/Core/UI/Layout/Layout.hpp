#pragma once
#include <imgui.h>

namespace Layout {

    struct Box { ImVec2 min, max; };

    void DrawCategoryIcon(ImDrawList* draw, int index, ImVec2 center, float size, ImU32 color);
    void DrawSubIcon(ImDrawList* draw, int index, ImVec2 center, float size, ImU32 color);

    void DrawCheckbox(ImDrawList* draw, const Box& box, float fillAnim,
        ImU32 accent, ImU32 border, int alpha);

}