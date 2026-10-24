#pragma once
#include <imgui.h>

namespace Widgets {

    constexpr int kEspItemCount = 35;

    struct Rect { ImVec2 min, max; };

    void DrawCategoryIcon(ImDrawList* draw, int index, ImVec2 center, float size, ImU32 color);
    void DrawSubIcon(ImDrawList* draw, int index, ImVec2 center, float size, ImU32 color);

    void DrawCheckbox(ImDrawList* draw, const Rect& box, float fillAnim,
        ImU32 accent, ImU32 border, int alpha);

    float RowAlpha(int idx, float expandBox, float expandSkel, float expandFlags, float expandOffscreen);
    float RowOffsetY(int idx, float expandBox, float expandSkel, float expandFlags, float expandOffscreen);
    float RowCenterY(ImVec2 cardMin, int idx, float expandBox, float expandSkel, float expandFlags, float expandOffscreen, float s);
    float ContentHeight(float expandBox, float expandSkel, float expandFlags, float expandOffscreen);

    bool HasColor(int idx);
    bool IsSliderRow(int idx);
    bool IsSkeletonJoint(int idx);

    Rect GetCheckboxRect(ImVec2 cardMin, ImVec2 cardMax, int idx,
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen, float s);
    Rect GetColorRect(ImVec2 cardMin, ImVec2 cardMax, int idx,
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen, float s);
    Rect GetSliderRect(ImVec2 cardMin, ImVec2 cardMax, int idx,
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen, float s);
    Rect GetExpanderRect(ImVec2 cardMin, ImVec2 cardMax, float s,
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen, ImFont* font, float labelFs);
    Rect GetSkelExpanderRect(ImVec2 cardMin, ImVec2 cardMax, float s,
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen, ImFont* font, float labelFs);
    Rect GetFlagsExpanderRect(ImVec2 cardMin, ImVec2 cardMax, float s,
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen, ImFont* font, float labelFs);
    Rect GetOffscreenExpanderRect(ImVec2 cardMin, ImVec2 cardMax, float s,
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen, ImFont* font, float labelFs);

    void DrawEspCards(ImDrawList* dl, ImVec2 min, ImVec2 max, float alpha,
        const float* hover, const float* sel,
        const float* colorHover, const float* const* colors,
        float expandBox, float expanderBoxHover,
        float expandSkel, float expanderSkelHover,
        float expandFlags, float expanderFlagsHover,
        float expandOffscreen, float expanderOffscreenHover,
        ImFont* font, float labelFs, float s,
        bool drawCardBg);

    void DrawDashedSlider(ImDrawList* dl, const Rect& r, float* val,
        float vmin, float vmax, float alpha, float s, const char* id);
}