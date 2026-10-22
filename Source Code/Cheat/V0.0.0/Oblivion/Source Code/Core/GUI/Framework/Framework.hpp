#pragma once
#include <imgui.h>
#include <cmath>
#include <algorithm>

namespace Framework {

    inline float Snap(float v) {
        return std::floor(v + 0.5f);
    }

    inline float LerpFactor(float dt, float base = 0.0001f) {
        return 1.0f - powf(base, dt);
    }

    inline void Approach(float& current, float target, float step) {
        if (current < target) current = (std::min)(target, current + step);
        else if (current > target) current = (std::max)(target, current - step);
    }

    inline int EasedAlpha(float eased) {
        int a = (int)(eased * 255.0f);
        return a < 0 ? 0 : (a > 255 ? 255 : a);
    }

    inline int MixGray(int base, float hover) {
        int v = (int)(base + hover * 50);
        return v > 255 ? 255 : v;
    }

    ImFont* IconFont();
    void ResetCachedFonts();

    ImU32 ScaleAlpha(ImU32 col, float mul);
    float Hash01(unsigned int x);
    void PlayClickSound();
    ImFont* GetIconFont();
    void DrawFlowLine(ImDrawList* dl, ImVec2 a, ImVec2 b,
        ImU32 baseCol, ImU32 glowCol, ImU32 coreCol,
        float thickness, float pulsePos, float pulseWidth);
    void DrawIconGlyph(ImDrawList* dl, int type, ImVec2 c,
        float boxSz, float fontSz, ImU32 col);
}