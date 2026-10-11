#pragma once
#include <imgui.h>

namespace UI {
    namespace Cards {

        constexpr int kEspItemCount = 24;

        struct Rect { ImVec2 min, max; };

        float RowAlpha(int idx, float expandBox, float expandFlags);
        float RowCenterY(ImVec2 cardMin, int idx, float expandBox, float expandFlags, float s);

        bool HasColor(int idx);

        Rect GetCheckboxRect(ImVec2 cardMin, ImVec2 cardMax, int idx,
            float expandBox, float expandFlags, float s);
        Rect GetColorRect(ImVec2 cardMin, ImVec2 cardMax, int idx,
            float expandBox, float expandFlags, float s);
        Rect GetExpanderRect(ImVec2 cardMin, ImVec2 cardMax, float s,
            float expandBox, float expandFlags, ImFont* font, float labelFs);
        Rect GetFlagsExpanderRect(ImVec2 cardMin, ImVec2 cardMax, float s,
            float expandBox, ImFont* font, float labelFs);

        void Draw(ImDrawList* dl, ImVec2 min, ImVec2 max, float alpha,
            const float* hover, const float* sel,
            const float* colorHover, const float* const* colors,
            float expandBox, float expanderBoxHover,
            float expandFlags, float expanderFlagsHover,
            ImFont* font, float labelFs, float s,
            bool drawCardBg = true);

    }
}