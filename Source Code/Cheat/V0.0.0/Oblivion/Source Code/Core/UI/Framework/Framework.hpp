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
}