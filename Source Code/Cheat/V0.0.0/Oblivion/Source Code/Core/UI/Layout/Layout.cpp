#include "Layout.hpp"
#include <cmath>

namespace Layout {

    static void DrawAimbotIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float r = s * 0.38f;
        dl->AddCircle(c, r, col, 32, 1.8f);
        dl->AddCircleFilled(c, s * 0.10f, col, 12);
        float g0 = r + 2.5f;
        float g1 = s * 0.55f;
        dl->AddLine(ImVec2(c.x - g1, c.y), ImVec2(c.x - g0, c.y), col, 1.8f);
        dl->AddLine(ImVec2(c.x + g0, c.y), ImVec2(c.x + g1, c.y), col, 1.8f);
        dl->AddLine(ImVec2(c.x, c.y - g1), ImVec2(c.x, c.y - g0), col, 1.8f);
        dl->AddLine(ImVec2(c.x, c.y + g0), ImVec2(c.x, c.y + g1), col, 1.8f);
    }

    static void DrawEyeIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float w = s * 0.50f;
        float h = s * 0.40f;
        dl->AddBezierCubic(
            ImVec2(c.x - w, c.y),
            ImVec2(c.x - w * 0.35f, c.y - h),
            ImVec2(c.x + w * 0.35f, c.y - h),
            ImVec2(c.x + w, c.y), col, 1.8f, 24);
        dl->AddBezierCubic(
            ImVec2(c.x - w, c.y),
            ImVec2(c.x - w * 0.35f, c.y + h),
            ImVec2(c.x + w * 0.35f, c.y + h),
            ImVec2(c.x + w, c.y), col, 1.8f, 24);
        dl->AddCircleFilled(c, s * 0.14f, col, 16);
    }

    static void DrawGearIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float r = s * 0.28f;
        float outer = s * 0.48f;
        int teeth = 8;
        for (int i = 0; i < teeth; ++i) {
            float a = i * (6.2831853f / teeth);
            ImVec2 p0(c.x + cosf(a) * (r - 1.0f), c.y + sinf(a) * (r - 1.0f));
            ImVec2 p1(c.x + cosf(a) * outer, c.y + sinf(a) * outer);
            dl->AddLine(p0, p1, col, 2.4f);
        }
        dl->AddCircle(c, r, col, 32, 1.8f);
        dl->AddCircleFilled(c, r * 0.40f, col, 12);
    }

    static void DrawSlidersIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float w = s * 0.48f;
        float h1 = -s * 0.24f;
        float h2 = 0.0f;
        float h3 = s * 0.24f;

        dl->AddLine(ImVec2(c.x - w, c.y + h1), ImVec2(c.x + w, c.y + h1), col, 1.6f);
        dl->AddCircleFilled(ImVec2(c.x + w * 0.30f, c.y + h1), 2.6f, col, 12);

        dl->AddLine(ImVec2(c.x - w, c.y + h2), ImVec2(c.x + w, c.y + h2), col, 1.6f);
        dl->AddCircleFilled(ImVec2(c.x - w * 0.30f, c.y + h2), 2.6f, col, 12);

        dl->AddLine(ImVec2(c.x - w, c.y + h3), ImVec2(c.x + w, c.y + h3), col, 1.6f);
        dl->AddCircleFilled(ImVec2(c.x + w * 0.10f, c.y + h3), 2.6f, col, 12);
    }

    void DrawCategoryIcon(ImDrawList* draw, int index, ImVec2 center, float size, ImU32 color) {
        switch (index) {
        case 0: DrawAimbotIcon(draw, center, size, color);  break;
        case 1: DrawEyeIcon(draw, center, size, color);     break;
        case 2: DrawGearIcon(draw, center, size, color);    break;
        case 3: DrawSlidersIcon(draw, center, size, color); break;
        }
    }

    static void DrawEnemyIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float r = s * 0.36f;
        dl->AddCircle(c, r, col, 32, 1.5f);
        dl->AddCircleFilled(c, s * 0.09f, col, 12);
        float g0 = r * 0.55f;
        float g1 = r * 1.55f;
        dl->AddLine(ImVec2(c.x - g1, c.y), ImVec2(c.x - g0, c.y), col, 1.4f);
        dl->AddLine(ImVec2(c.x + g0, c.y), ImVec2(c.x + g1, c.y), col, 1.4f);
        dl->AddLine(ImVec2(c.x, c.y - g1), ImVec2(c.x, c.y - g0), col, 1.4f);
        dl->AddLine(ImVec2(c.x, c.y + g0), ImVec2(c.x, c.y + g1), col, 1.4f);
    }

    static void DrawTeamIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float headR = s * 0.20f;
        ImVec2 h1(c.x - s * 0.18f, c.y - s * 0.12f);
        ImVec2 h2(c.x + s * 0.18f, c.y - s * 0.12f);
        dl->AddCircle(h1, headR, col, 20, 1.5f);
        dl->AddCircle(h2, headR, col, 20, 1.5f);

        dl->AddBezierCubic(
            ImVec2(c.x - s * 0.42f, c.y + s * 0.42f),
            ImVec2(c.x - s * 0.38f, c.y + s * 0.10f),
            ImVec2(c.x + s * 0.05f, c.y + s * 0.10f),
            ImVec2(c.x + s * 0.02f, c.y + s * 0.42f),
            col, 1.5f, 20);

        dl->AddBezierCubic(
            ImVec2(c.x - s * 0.02f, c.y + s * 0.42f),
            ImVec2(c.x - s * 0.05f, c.y + s * 0.10f),
            ImVec2(c.x + s * 0.38f, c.y + s * 0.10f),
            ImVec2(c.x + s * 0.42f, c.y + s * 0.42f),
            col, 1.5f, 20);
    }

    static void DrawLocalIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float headR = s * 0.15f;
        ImVec2 head(c.x, c.y - s * 0.16f);
        dl->AddCircle(head, headR, col, 24, 1.5f);

        dl->AddBezierCubic(
            ImVec2(c.x - s * 0.32f, c.y + s * 0.38f),
            ImVec2(c.x - s * 0.30f, c.y + s * 0.05f),
            ImVec2(c.x + s * 0.30f, c.y + s * 0.05f),
            ImVec2(c.x + s * 0.32f, c.y + s * 0.38f),
            col, 1.5f, 24);
    }

    static void DrawWorldIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float r = s * 0.38f;
        dl->AddCircle(c, r, col, 36, 1.5f);
        dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x + r, c.y), col, 1.3f);

        dl->AddBezierCubic(
            ImVec2(c.x, c.y - r),
            ImVec2(c.x + r * 0.55f, c.y - r * 0.40f),
            ImVec2(c.x + r * 0.55f, c.y + r * 0.40f),
            ImVec2(c.x, c.y + r),
            col, 1.3f, 24);

        dl->AddBezierCubic(
            ImVec2(c.x, c.y - r),
            ImVec2(c.x - r * 0.55f, c.y - r * 0.40f),
            ImVec2(c.x - r * 0.55f, c.y + r * 0.40f),
            ImVec2(c.x, c.y + r),
            col, 1.3f, 24);

        dl->AddBezierCubic(
            ImVec2(c.x - r, c.y - r * 0.30f),
            ImVec2(c.x - r * 0.20f, c.y - r * 0.18f),
            ImVec2(c.x + r * 0.20f, c.y - r * 0.18f),
            ImVec2(c.x + r, c.y - r * 0.30f),
            col, 1.3f, 24);

        dl->AddBezierCubic(
            ImVec2(c.x - r, c.y + r * 0.30f),
            ImVec2(c.x - r * 0.20f, c.y + r * 0.18f),
            ImVec2(c.x + r * 0.20f, c.y + r * 0.18f),
            ImVec2(c.x + r, c.y + r * 0.30f),
            col, 1.3f, 24);
    }

    void DrawSubIcon(ImDrawList* draw, int index, ImVec2 center, float size, ImU32 color) {
        switch (index) {
        case 0: DrawEnemyIcon(draw, center, size, color); break;
        case 1: DrawTeamIcon(draw, center, size, color);  break;
        case 2: DrawLocalIcon(draw, center, size, color); break;
        case 3: DrawWorldIcon(draw, center, size, color); break;
        }
    }

    void DrawCheckbox(ImDrawList* draw, const Box& box, float fillAnim,
        ImU32 accent, ImU32 border, int alpha)
    {
        draw->AddRectFilled(box.min, box.max, IM_COL32(16, 16, 16, alpha), 0.0f);
        draw->AddRect(box.min, box.max, border, 0.0f, 0, 1.0f);

        if (fillAnim > 0.001f) {
            const float fullSize = 8.0f;
            const float s = fullSize * fillAnim;
            const float cx = (box.min.x + box.max.x) * 0.5f;
            const float cy = (box.min.y + box.max.y) * 0.5f;
            const float half = s * 0.5f;
            draw->AddRectFilled(
                ImVec2(cx - half, cy - half),
                ImVec2(cx + half, cy + half),
                accent, 0.0f);
        }
    }

}